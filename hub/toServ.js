var request = require('request');
var basics = require('./basics');
var exports = module.exports = {};

var tempUrl = "https://eli-server.ddns.net:443";
var user = "43a59d21-6bb5-4fe4-bdb1-81963d7a24a8";

process.env.NODE_TLS_REJECT_UNAUTHORIZED = "0";

exports.userData = function(url, uuid, start, end){
    var fullUrl = url+"/user_data?uuid="+uuid;
    if (start) {
        fullUrl = fullUrl+"&start="+start;
    }
    if (end) {
        fullUrl = fullUrl+"&end="+end;
    }
    var options = {
        url: fullUrl,
        method: 'GET'
    };
    request(options)
        .on('data', function(data){
            data = data.toString();
            console.log(data);
        })
        .on('error', function(err) {
            console.log("Error", err);
        })
}


exports.sendData = function(url, data, uuid) {
    toSend = {
        uuid: uuid,
        data: data
    }
    request.post(url+"/postData", toSend);
}

exports.postData = function(url, uuid, data){
    var fullUrl = url+"/user_data/";
    var options = {
        url: fullUrl,
        method: 'POST',
        body: data
    };
    request(options)
}
