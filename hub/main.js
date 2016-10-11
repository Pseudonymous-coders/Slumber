var nrf = require('nrfuart');
var m2x = require('./m2x');
var wifi = require('node-wifi');


var xmax = ymax = zmax = hbmax = tempmax = 0;
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
                if (data[0] == "V") {
                    m2x.logVal("batt", data.substr(1));
                    console.log("Logging batt on M2X");
                }
                if (data.length > 14){
                    counter += 1;
                    var rawVals = {
                        temp: data.split(";")[0],
                        hb: data.split(";")[1],
                        x: data.split(";")[2].substr(0,3),
                        y: data.split(";")[2].substr(3,6),
                        z: data.split(";")[2].substr(6,9)
                    };
                    
                    console.log("x: ",rawVals.x);
                    console.log("y: ",rawVals.y);
                    console.log("z: ",rawVals.z);
                    console.log("hb: ",rawVals.hb);
                    console.log("temp: ",rawVals.temp);
                    
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
                        m2x.logVal("accel-x", xmax);
                        m2x.logVal("accel-y", ymax);
                        m2x.logVal("accel-z", zmax);
                        m2x.logVal("heartbeat", hbmax);
                        m2x.logVal("temp", tempmax);
                        console.log("Logging sensor values on M2X");
                        xmax = ymax = zmax = hbmax = tempmax = 0;
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

