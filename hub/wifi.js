var wifi = require('node-wifi');

//Initialize wifi module
wifi.init({
    debug : true,
    iface : null
    // the OS will find the right network interface if it is null  
});

//Scan networks
wifi.scan(function(err, networks) {

    if (err) {
        console.log(err);
    } else {
        console.log(networks);
    }
});
