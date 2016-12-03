require('./basics');
var nrf = require('nrfuart');
var toServ = require('./toServ');
var serialCom = require('./serialCom')

var chart = require('ascii-chart');
var clear = require('clear');

// Common variables
var user = "43a59d21-6bb5-4fe4-bdb1-81963d7a24a8";
var tempUrl = "http://eli-server.ddns.net:6767"

// Accel/temp/voltage variables
// Counter
var counter = 0;
var fourCount = 0;
var fourAccels= [];
// Sets calibration cap
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

var testData = [];

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
                    temp = mainSense[0];
                    hum = mainSense[1];
                    VBatt = mainSense[2];
                    toServ.postData(tempUrl, user, "TnH", {temp: temp, hum: hum});
                    toServ.postData(tempUrl, user, "VBatt", {vbatt: VBatt} );
                } else if (data[0] == "B") {
                    var accel = data.split(";").slice(1);
                    var curX = parseInt(accel[0]),
                        curY = parseInt(accel[1]),
                        curZ = parseInt(accel[2]);
                    x.push(curX);
                    y.push(curY);
                    z.push(curZ);
                    counter += 1;
                    if (counter == countCap) {
                        console.log("Finished Calibration...");
                        xmin = Math.min.apply(null,x);
                        ymin = Math.min.apply(null,y);
                        zmin = Math.min.apply(null,z);
                        xrange = Math.max.apply(null,x) - Math.min.apply(null, x);
                        yrange = Math.max.apply(null,y) - Math.min.apply(null, y);
                        zrange = Math.max.apply(null,z) - Math.min.apply(null, z);
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
                        totAccel = (0 <= totAccel && totAccel <= 100) ? totAccel : 0;
                        fourAccels.push(totAccel);
                        if (counter > countCap){
                            fourCount += 1;
                            if (fourCount == 4){
                                console.log(fourAccels);
                                totAccel = Math.max.apply(null, fourAccels);
                                toServ.postData(tempUrl, user, "accel", {accel: totAccel});
                                //serialCom.sendData({"response": "liveUpdate", data: {sleepScore: Math.abs(100-Math.max(curX, curY, curZ))}})
                                fourCount = 0;
                                fourAccels = [];
                            } 
                            if (counter == countCap + 5) {
                                var nightArr = [];
                                for (i = 0; i < 14500; i++) {
                                    nightArr.push({type:"accel", time: i * 2 , data: {"accel": Math.round(100/14500 * i)}});
                                } 
                                serialCom.sendData({"response":"nightUpdate","data":{night:nightArr}});
                            }
                        }
                    }
                }
            }
        });
    });
});
