let clockstamp = null;

// Load the precompiled binary for windows.
if (process.platform === "win32" && process.arch === "x64") {
    clockstamp = require('./bin/winx64/clockstamp');
} else if (process.platform === "win32" && process.arch === "ia32") {
    clockstamp = require('./bin/winx86/clockstamp');
} else {
    // Load the new built binary for other platforms.
    clockstamp = require('./build/Release/clockstamp');
    console.log(__dirname);
}

/*=============== METÓDOS PÚBLICOS DO MÓDULO ==================*/

let ClockStamp = {
    refresh: (id) => {
        return clockstamp.refresh(id);
    },
    getActives: () => {
        return clockstamp.getActives();
    },
    isActive: (id) => {
        return clockstamp.refresh(id);
    }
};

module.exports.ClockStamp = ClockStamp;

let stdin = process.openStdin();

stdin.addListener("data", function (d) {
    let res = d.toString().trim();
    if (res) {
        if (res === 'a') {
            console.log(ClockStamp.getActives());
        } else {
            ClockStamp.refresh(Number(res));
        }
    }
});