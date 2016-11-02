var net = require('net');
var basics = require('./basics')

var PORT = 3005;
var host = "127.0.0.1";

var client = new net.Socket();

client.connect(PORT, host, function() {
    console.log('Connected to {}:{}'.format(host, PORT));
})

client.on('data', function(data){
    data = JSON.parse(data.toSting());
    var response = {};

    // Get wifi list
    if (data.exec == "getWifi") {
        var wifi = require('node-wifi');
        
        wifi.init({
            debug: false,
            iface: null
        });

        wifi.scan(function(err, networks) {
            if (err) {
                response = {
                    response: "getWifi",
                    data: {
                        error: err
                    }
                }
            } else {
                response = {
                    response: "getWifi",
                    data: {
                        APs: networks
                    }
                }
            }
        });

        client.write(response);
    // Connect to wifi
    } else if (data.exec == "connectWifi") {
        var connected = 0;
        wifi.connect({ssid: data.ssid, password: data.password}, function(err) {
            console.log(err)
            connected = 2;
        });
        if (connected != 2) {
            connected = 1;
        }
        response = {
            reponse: "connectWifi",
            data: {
                connected: connected
            }
        }
    } else {
        response = {
            response: data.exec,
            data: {
                error: "Unknown error/command";
            }
        }
    }
});

client.on('close', function() {
    console.log("Closed Connection")
}
