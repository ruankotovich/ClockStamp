let ClockStamp = require('./index.js').ClockStamp;
let stdin = process.openStdin();

const STORES = 500;
const USERS = 100;

// for (let i of Array.from(Array(STORES).keys())) {
//     for (let j of Array.from(Array(USERS).keys())) {
//         ClockStamp.refresh(i, j);
//     }
// }

setInterval(() => {
    process.stdout.write('\033c');
    ClockStamp.getActives().forEach((k) => {
        process.stdout.write(String(k) + ",");
    });
}, 200);

function rand(max) {
    return Math.floor(Math.random() * Math.floor(max));
}

function recycle() {
    setTimeout(() => {
        ClockStamp.refresh(rand(STORES), rand(USERS));
        recycle();
    }, rand(300));
}

recycle();

stdin.addListener("data", function (d) {
    let res = d.toString().trim();
    if (res) {
        if (res === 'a') {
            console.log(ClockStamp.getActives());
        } else {
            let pos = res.split(' ');
            switch (pos[0]) {
                case 'r': {
                    console.log('Refreshing ', pos[2], ' on node ', pos[1]);
                    ClockStamp.refresh(Number(pos[1]), Number(pos[2]));
                }
                    break;
                case 's': {
                    console.log(ClockStamp.getActivesOnNode(Number(pos[1])));
                }
                    break;
            }

        }
    }
});