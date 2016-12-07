var nrf = require('nrfuart');
var toServ = require('./toServ');
var serialCom = require('./serialCom')

var chart = require('ascii-chart');
var clear = require('clear');

require('./basics');

try {
    serialCom.sendData({"response": "init", data: {init: true}});
} catch (err) {
    serialCom = {
        sendData: function(data) {
            console.log("Failed to send data, no init to server");
        }
    }
}

// Common variables
// Accel/temp/voltage variables
// Counter
var counter = 0;
var fourCount = 0;
var fourAccels= [];
// Sets calibration cap
var countCap = 10;
var temp = 0,
    hum = 0,
    vbatt = 0.0;
// Accel variables
var accelMin = 0;
var accelRange= 0;
var oldRange = 0;
var oldAccel = 0;
var accels = []
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
                    vbatt = mainSense[2];
                    if (serialCom.isAwake == true) {
                        toServ.postData(tempUrl, user, "TnH", {temp: temp, hum: hum});
                        toServ.postData(tempUrl, user, "VBatt", {vbatt: vbatt} );
                    }
                } else if (data[0] == "B") {
                    var accel = data.split(";").slice(1);
                    var curAccel = parseInt(accel[0])
                    accels.push(curAccel);
                    counter += 1;
                    if (counter == countCap) {
                        console.log("Finished Calibration...");
                        accelMin = Math.min.apply(null,accels);
                    } else {
                        curAccel = Math.round((1/50)*Math.abs(curAccel - accelMin));
                        curAccel = (curAccel > 100) ? 100 : curAccel;

                        var tmpAccel = curAccel;

                        curAccel = (curAccel > oldAccel - 5 && curAccel < oldAccel + 5) ? 0 : curAccel;

                        oldAccel = tmpAccel;
                        totAccel = curAccel; 
                        totAccel = (0 <= totAccel && totAccel <= 100) ? totAccel : 0;
                        fourAccels.push(totAccel);
                        if (counter > countCap){
                            fourCount += 1;
                            if (fourCount == 4){
                                totAccel = Math.max.apply(null, fourAccels);
                                toSend = {"response": "liveUpdate", data: {sleepScore: Math.abs(100-totAccel), temp: temp, hum: hum, vbatt: vbatt}}
                                serialCom.sendData(toSend);
                                if (serialCom.isSleeping == true) {
                                    toServ.postData(tempUrl, user, "accel", {accel: totAccel});
                                }
                                fourCount = 0;
                                fourAccels = [];
                            } 
                        }
                    }
                }
            }
        });
    });
});
