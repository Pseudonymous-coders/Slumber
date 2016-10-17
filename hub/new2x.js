var request = require('request');

var key = "19f56759ada36266960cc37b2ee3537c";
var device = "15df8a57671bb58f633afa646f348dbc";

var streams = [];

request.get({
    headers: {
    "X-M2X-KEY": key
    },
    uri: "http://api-m2x.att.com/v2/devices/"+device+"/streams"
}, function(err, res, body){
    body = JSON.parse(body);
    streams = body.streams;
});

console.log(streams);
