require('./basics');
var nrf = require('nrfuart');
//var toServ = require('./toServ');
//var serialCom = require('./serialCom')

var counter = 0;

//sets calibration cap
var countCap = 10;

var temp = 0,
    hum = 0,
    VBatt = 0.0;

// Accel variables
var xmin = 0,
    ymin = 0,
    zmin = 0;
var xrange = 0,
    yrange = 0,
    zrange = 0;
var oldX = 0,
    oldY = 0,
    oldZ = 0;
var x = [],
    y = [],
    z = [];
var totAccel = 0;

console.log("Started BLE server...");
nrf.discoverAll(function(ble_uart){
    console.log("Scanning for devices...");
    ble_uart.on('disconnect', function() {
        console.log("Disconnected!!");
    });
    ble_uart.connectAndSetup(function() {
        counter = 0;

        ble_uart.readDeviceName(function(devName) {
            console.log("Connected to "+ devName);
        });

                ble_uart.on("data", function(data) {
            data = data.toString();
            if (data.split(";").length == 4){
                if (data[0] == "A") {
                    var mainSense = data.split(";").slice(1);
                } else if (data[0] == "B") {
                    var accels = data.split(";").slice(1);
                    var curX = parseInt(accels[0]),
                        curY = parseInt(accels[1]),
                        curZ = parseInt(accels[2]);
                    x.push(curX);
                    y.push(curY);
                    z.push(curZ);
                    if (counter == countCap) {
                        console.log("Finished Calibration...");
                        xmin = Math.min.apply(null,x);
                        ymin = Math.min.apply(null,y);
                        zmin = Math.min.apply(null,z);
                        xrange = Math.max.apply(null,x) - Math.min.apply(null, x);
                        yrange = Math.max.apply(null,y) - Math.min.apply(null, y);
                        zrange = Math.max.apply(null,z) - Math.min.apply(null, z);
                        counter += 1;
                    } else if (counter < countCap) {
                        counter += 1;
                    } else {
                        curX = Math.round((1/50)*Math.abs(curX - xmin));
                        curY = Math.round((1/50)*Math.abs(curY - ymin));
                        curZ = Math.round((1/50)*Math.abs(curZ - zmin));
                        curX = (curX > 100) ? 100 : curX;
                        curY = (curY > 100) ? 100 : curY;
                        curZ = (curZ > 100) ? 100 : curZ;

                        var tmpX = curX,
                            tmpY = curY,
                            tmpZ = curZ;

                        curX = (curX > oldX - 5 && curX < oldX + 5) ? 0 : curX;
                        curY = (curY > oldY - 5 && curY < oldY + 5) ? 0 : curY;
                        curZ = (curZ > oldZ - 5 && curZ < oldZ + 5) ? 0 : curZ;

                        oldX = tmpX;
                        oldY = tmpY;
                        oldZ = tmpZ;
                        totAccel = Math.max(curX, curY, curZ);
                    }
                }
            }
        });
    });
});
