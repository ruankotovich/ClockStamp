#include <node/node.h>
#include <sstream>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#include <chrono>

#define STAMP 5
namespace Chronomancer {
    struct ClockKeeper {
        std::thread tickerThread;
        std::mutex m_clockstamp;
        int clockstamp;
        int duration_clockstamp;
        std::map<unsigned, int> clocks;
        std::map<unsigned, int>::iterator pivot;

        ClockKeeper() {
            pivot = clocks.end(); //volatile
            duration_clockstamp = clockstamp = 0;
            tickerThread = std::thread(&ClockKeeper::tick, this);
            tickerThread.detach();
        }

        void refreshAll() {
#ifdef DEBUG
            std::cout << "refreshAll" << std::endl;
#endif
            if (!clocks.empty()) { // have I deleted the last pivot? nope!
                int interval = timeInterval();
                clockstamp = std::numeric_limits<int>::max(); // set the current general clockstamp to maximum possible

                for (auto it = clocks.begin(); it != clocks.end();) {// for each clock

                    it->second -= interval; // decrement the clockstamp by the duration (notice that there's no modification on this variable yet)

                    if (it->second < 0) {
                        it = clocks.erase(it);
                        continue;
                    }

                    if (it->second < clockstamp) { // if the current stamp is less than the pivot, use this as general
                        pivot = it;
                        clockstamp = it->second;
                    }

                    ++it;
                }

                if (!clocks.empty()) {
                    duration_clockstamp = clockstamp;
                } else { // removed everybody
                    reset();
                }

            } else {
                reset();
            }
        }

        inline int timeInterval() {
            return (duration_clockstamp - clockstamp);
        }

        void reset() {
            duration_clockstamp = clockstamp = 0;
            pivot = clocks.end();
        }

        void tick() {
            while (true) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                m_clockstamp.lock(); //get the lock

                if (!clocks.empty()) { // are there clocks?
//                std::cout << "\033[2J\033[1;1H" << std::endl;
#ifdef DEBUG
                    std::cout << "CLOCKS : ";
                    for (auto &el : clocks) {
                        std::cout << '{' << el.first << ',' << el.second << '}';
                    }

                    std::cout << std::endl;
#endif
                    clockstamp--;
#ifdef DEBUG
                    if (clockstamp >= 0) { // can it tick?
                        std::cout << clockstamp << std::endl;
                    } else { // end of tick
                        refreshAll();
                    }
#else
                    if (clockstamp < 0) {
                        refreshAll();
                    }
#endif
                }
                m_clockstamp.unlock();

            }
        }

        void refresh(unsigned id) {
            m_clockstamp.lock();

            if (pivot != clocks.end()) { // there's pivot, so it isn't empty

                if (pivot->first == id) { // if the pivot is the current dude
                    if (clocks.size() == 1) {
                        clocks[id] = STAMP;
                        duration_clockstamp = clockstamp = STAMP;
                    } else {
                        clocks[id] = STAMP + timeInterval();
                        refreshAll();   // update everything
                    }
                } else {
                    clocks[id] = STAMP + timeInterval();
                }

            } else {
                reset();
                clocks[id] = STAMP;
                refreshAll();
            }

            m_clockstamp.unlock();
        }

        bool isActive(unsigned id) {
            bool active = false;

            m_clockstamp.lock();
            active = clocks.find(id) != clocks.end();
            m_clockstamp.unlock();

            return active;
        }

        std::string getActiveListAsJSONArray() {
            std::stringstream ret;
            std::string rtrn;

            ret << '[';

            m_clockstamp.lock();
            for (auto &el:clocks) {
                ret << el.first << ',';
            }
            m_clockstamp.unlock();


            rtrn = ret.str();

            if (rtrn.size() > 1) {
                rtrn.pop_back();
            }

            rtrn += ']';

            return rtrn;
        }

        std::vector<unsigned int> getActiveListAsVector() {

            std::vector<unsigned int> ids;

            m_clockstamp.lock();

            ids.reserve(clocks.size());

            for (auto &el:clocks) {
                ids.push_back(el.first);
            }

            m_clockstamp.unlock();

            return std::move(ids);
        }

    };
}

Chronomancer::ClockKeeper keeper;

void refresh(const v8::FunctionCallbackInfo<v8::Value> &args) {
    v8::Isolate *isolate = args.GetIsolate();

    if (args.Length() != 1) {
        isolate->ThrowException(v8::Exception::TypeError(
                v8::String::NewFromUtf8(isolate, "This function requires only one parameters")));
        return;
    }

    unsigned int id = args[0]->Uint32Value();

    try {
        keeper.refresh(id);
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
        for (auto &id : keeper.getActiveListAsVector()) {
            nodes->Set(count, v8::Integer::New(isolate, id));
            ++count;
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
        args.GetReturnValue().Set(v8::Boolean::New(isolate, keeper.isActive(id)));
    } catch (const char *err) {
        isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, err)));
    }
}

void Init(v8::Local<v8::Object> exports) {
    NODE_SET_METHOD(exports, "refresh", refresh);
    NODE_SET_METHOD(exports, "getActives", getActives);
    NODE_SET_METHOD(exports, "isActive", isActive);
}

NODE_MODULE(module_name, Init);