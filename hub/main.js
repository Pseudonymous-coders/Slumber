var nrf = require('nrfuart');
var wifi = require('node-wifi');
var toServ = require('./toServ.js');

var xmax = ymax = zmax = hbmax = tempmax = VBatt = 0;
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
            if (typeof data != "undefined"){
                counter += 1;
                if (data[0] == "V") {
                    VBatt = data.substr(1);
                } else if (data.length > 5){
                    var rawVals = {
                        temp: data.split(";")[0],
                        hb: data.split(";")[1],
                        x: data.split(";")[2].substr(0,3),
                        y: data.split(";")[2].substr(3,6),
                        z: data.split(";")[2].substr(6,9)
                    };
                    xmax = (rawVals.x > xmax) ? rawVals.x : xmax;
                    ymax = (rawVals.y > ymax) ? rawVals.y : ymax;
                    zmax = (rawVals.z > zmax) ? rawVals.z : zmax;
                    hbmax = (rawVals.hb > hbmax) ? rawVals.hb : hbmax;
                    tempmax = (rawVals.temp > tempmax) ? rawVals.temp : tempmax;
                    if (counter >= 20) {
                        counter = 0;
                        console.log("Max vals: ");
                        console.log("X:"+xmax);
                        console.log("Y: "+ymax);
                        console.log("Z: "+zmax);
                        console.log("HB: "+hbmax);
                        console.log("Temp: "+tempmax);
                        console.log("VBatt: "+VBatt);
                        xmax = ymax = zmax = hbmax = tempmax = VBatt =0;
                    }
                }
            }
        });
    });
});

if (typeof ssid != "undefined" && typeof pass != 'undefined') {
    wifi.connect({ssid: ssid, password: pass}, function(err){
        if (err) {
            console.log(err);
            ssid = null;
            pass = null;
        }
        console.log("Connected");
    });
}

