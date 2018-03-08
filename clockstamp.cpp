#include <chrono>
#include <iostream>
#include <map>
#include <mutex>
#include <node/node.h>
#include <sstream>
#include <thread>

#define STAMP 10
namespace Chronomancer {

    struct MasterNodeKeeper {
        std::map<unsigned, int> activeNodes;

        void notifyAdded(unsigned id) {
            ++activeNodes[id];
        }

        void notifyEroded(unsigned id) {
            --activeNodes[id];
        }

        bool isActive(unsigned id) {
            bool returno = false;

            auto finder = activeNodes.find(id);
            returno = finder != activeNodes.end() && finder->second > 0;
            return returno;
        }

        std::vector<unsigned> getActives() {
            std::vector<unsigned> openOp;
            for (auto &el : activeNodes) {
                if (el.second > 0) {
                    openOp.push_back(el.first);
                }
            }
            return std::move(openOp);
        }
    };

    struct ClockKeeper {
        MasterNodeKeeper *masterClock;
        std::thread tickerThread;
        std::mutex m_clockstamp;
        bool working = true;
        int clockstamp;
        int duration_clockstamp;
        std::map<unsigned, int> clocks;
        std::map<unsigned, int>::iterator pivot;

        ClockKeeper(MasterNodeKeeper *clocker) {
            this->masterClock = clocker;
            this->pivot = clocks.end(); //volatile
            this->duration_clockstamp = clockstamp = 0;
            this->tickerThread = std::thread(&ClockKeeper::tick, this);
            this->tickerThread.detach();
        }

        ~ClockKeeper() {
            this->m_clockstamp.lock();

            this->working = false;

            for (auto &online : this->clocks) {
                this->masterClock->notifyEroded(online.first);
            }
            clocks.clear();

            this->m_clockstamp.unlock();
        }

        void refreshAll() {
#ifdef DEBUG
            std::cout << "refreshAll" << std::endl;
#endif
            if (!clocks.empty()) { // have I deleted the last pivot? nope!
                int interval = this->timeInterval();
                this->clockstamp = std::numeric_limits<int>::max(); // set the current general clockstamp to maximum possible

                for (auto it = this->clocks.begin(); it != this->clocks.end();) { // for each clock

                    it->second -= interval; // decrement the clockstamp by the duration (notice that there's no modification on this variable yet)

                    if (it->second < 0) {
                        this->masterClock->notifyEroded(it->first);
                        it = this->clocks.erase(it);
                        continue;
                    }

                    if (it->second < this->clockstamp) { // if the current stamp is less than the pivot, use this as general
                        this->pivot = it;
                        this->clockstamp = it->second;
                    }

                    ++it;
                }

                if (!this->clocks.empty()) {
                    this->duration_clockstamp = this->clockstamp;
                } else { // removed everybody
                    this->reset();
                }

            } else {
                this->reset();
            }
        }

        inline int timeInterval() {
            return (this->duration_clockstamp - this->clockstamp);
        }

        void reset() {
            this->duration_clockstamp = this->clockstamp = 0;
            this->pivot = this->clocks.end();
        }

        void tick() {
            while (this->working) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                this->m_clockstamp.lock(); //get the lock

                if (!this->clocks.empty()) { // are there clocks?
#ifdef DEBUG
                    std::cout << "CLOCKS : ";
                    for (auto& el : clocks) {
                        std::cout << '{' << el.first << ',' << el.second << '}';
                    }

                    std::cout << std::endl;
#endif
                    this->clockstamp--;
#ifdef DEBUG
                    if (clockstamp >= 0) { // can it tick?
                        std::cout << clockstamp << std::endl;
                    } else { // end of tick
                        refreshAll();
                    }
#else
                    if (this->clockstamp < 0) {
                        this->refreshAll();
                    }
#endif
                }
                this->m_clockstamp.unlock();
            }
            std::cout << "Thread Killed" << std::endl;
        }

        void refresh(unsigned id) {
            this->m_clockstamp.lock();

            if (this->pivot != this->clocks.end()) { // there's pivot, so it isn't empty

                if (this->pivot->first == id) { // if the pivot is the current dude
                    if (this->clocks.size() == 1) {
                        this->clocks[id] = STAMP;
                        this->duration_clockstamp = this->clockstamp = STAMP;
                    } else {
                        this->clocks[id] = STAMP + timeInterval();
                        this->refreshAll(); // update everything
                    }
                } else {
                    if (this->clocks.emplace(id, STAMP + this->timeInterval()).second) {
                        this->masterClock->notifyAdded(id);
                    }
                }

            } else {
                this->reset();
                this->clocks[id] = STAMP;
                this->masterClock->notifyAdded(id);
                this->refreshAll();
            }

            this->m_clockstamp.unlock();
        }

        bool isActive(unsigned id) {
            bool active = false;

            this->m_clockstamp.lock();
            active = this->clocks.find(id) != this->clocks.end();
            this->m_clockstamp.unlock();

            return active;
        }

        std::vector<unsigned int> getActives() {

            std::vector<unsigned int> ids;

            m_clockstamp.lock();

            ids.reserve(clocks.size());

            for (auto &el : clocks) {
                ids.push_back(el.first);
            }

            m_clockstamp.unlock();

            return std::move(ids);
        }
    };
}

std::map<unsigned, Chronomancer::ClockKeeper> keeper;
Chronomancer::MasterNodeKeeper master;

void refresh(const v8::FunctionCallbackInfo<v8::Value> &args) {
    v8::Isolate *isolate = args.GetIsolate();

    if (args.Length() != 2) {
        isolate->ThrowException(v8::Exception::TypeError(
                v8::String::NewFromUtf8(isolate, "This function requires two parameters")));
        return;
    }

    unsigned int nodeId = args[0]->Uint32Value();
    unsigned int clockId = args[1]->Uint32Value();

    try {
        auto finder = keeper.find(nodeId);
        if (finder != keeper.end()) {
            finder->second.refresh(clockId);
        } else {
            auto emplaced = keeper.emplace(std::piecewise_construct, std::forward_as_tuple(nodeId), std::forward_as_tuple(&master));
            emplaced.first->second.refresh(clockId);

        }
    } catch (const char *e) {
        isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, e)));
    }
}

void getActives(const v8::FunctionCallbackInfo<v8::Value> &args) {
    v8::Isolate *isolate = args.GetIsolate();

    if (args.Length() != 0) {
        isolate->ThrowException(
                v8::String::NewFromUtf8(isolate, "This function requires no parameters"));
        return;
    }

    try {
        v8::Local<v8::Array> nodes = v8::Array::New(isolate);

        int count = 0;
        for (auto &id : master.getActives()) {
            nodes->Set(count, v8::Integer::New(isolate, id));
            ++count;
        }
        args.GetReturnValue().Set(nodes);

    } catch (const char *err) {
        isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, err)));
    }
}

void getActivesOnNode(const v8::FunctionCallbackInfo<v8::Value> &args) {
    v8::Isolate *isolate = args.GetIsolate();

    if (args.Length() != 1) {
        isolate->ThrowException(
                v8::String::NewFromUtf8(isolate, "This function requires only one parameter"));
        return;
    }

    try {
        v8::Local<v8::Array> nodes = v8::Array::New(isolate);

        int count = 0;
        auto nodeId = args[0]->IntegerValue();
        auto finder = keeper.find(nodeId);
        if (finder != keeper.end()) {
            for (auto &id : finder->second.getActives()) {
                nodes->Set(count, v8::Integer::New(isolate, id));
                ++count;
            }
        }
        args.GetReturnValue().Set(nodes);

    } catch (const char *err) {
        isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, err)));
    }
}

void isActive(const v8::FunctionCallbackInfo<v8::Value> &args) {
    v8::Isolate *isolate = args.GetIsolate();

    if (args.Length() != 1) {
        isolate->ThrowException(
                v8::String::NewFromUtf8(isolate, "This function requires only one parameters"));
        return;
    }

    try {
        auto id = args[0]->IntegerValue();
        args.GetReturnValue().Set(v8::Boolean::New(isolate, master.isActive(id)));
    } catch (const char *err) {
        isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, err)));
    }
}

void isActiveOnNode(const v8::FunctionCallbackInfo<v8::Value> &args) {
    v8::Isolate *isolate = args.GetIsolate();

    if (args.Length() != 2) {
        isolate->ThrowException(
                v8::String::NewFromUtf8(isolate, "This function requires two parameters"));
        return;
    }

    try {
        unsigned int nodeId = args[0]->Uint32Value();
        unsigned int clockId = args[1]->Uint32Value();

        auto finder = keeper.find(nodeId);

        if (finder != keeper.end()) {
            args.GetReturnValue().Set(v8::Boolean::New(isolate, finder->second.isActive(clockId)));
        } else {
            args.GetReturnValue().Set(v8::Boolean::New(isolate, false));
        }

    } catch (const char *err) {
        isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, err)));
    }
}

void Init(v8::Local<v8::Object> exports) {
    NODE_SET_METHOD(exports, "refresh", refresh);
    NODE_SET_METHOD(exports, "getActives", getActives);
    NODE_SET_METHOD(exports, "getActivesOnNode", getActivesOnNode);
    NODE_SET_METHOD(exports, "isActive", isActive);
    NODE_SET_METHOD(exports, "isActiveOnNode", isActiveOnNode);
}

NODE_MODULE(module_name, Init);