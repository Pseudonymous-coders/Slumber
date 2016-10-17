var request = require('request');
var exports = module.exports = {};
var key = "19f56759ada36266960cc37b2ee3537c";
var device = "15df8a57671bb58f633afa646f348dbc";
var url = "http://api-m2x.att.com/v2/devices/"

request.get({
    headers: {
    "X-M2X-KEY": key
    },
    uri: url+device+"/streams"
}, function(err, res, body){
    body = JSON.parse(body);
    exports.streams = body.streams;
});


exports.values = function(stream) {
    request.get({
        
    })
}
