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
    refresh: (node, clock) => {
        return clockstamp.refresh(node, clock);
    },
    getActives: () => {
        return clockstamp.getActives();
    },
    isActive: (id) => {
        return clockstamp.isActive(id);
    },
    getActivesOnNode: (node) => {
        return clockstamp.getActivesOnNode(node);
    },
    isActiveOnNode: (node) => {
        return clockstamp.isActiveOnNode(node);
    }
};

module.exports.ClockStamp = ClockStamp;
