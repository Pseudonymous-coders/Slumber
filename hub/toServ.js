var request = require('request');
var sReq = require('sync-request');
var basics = require('./basics');
var exports = module.exports = {};

//var tempUrl = "https://eli-server.ddns.net:443";
var tempUrl = "http://eli-server.ddns.net:6767";
var user = "43a59d21-6bb5-4fe4-bdb1-81963d7a24a8";

process.env.NODE_TLS_REJECT_UNAUTHORIZED = "0";

exports.userData = function(url, uuid, type, start, end){
    var fullUrl = url+"/user_data/{}/{}/{}/{}".format(uuid,type,start,end);
    var options = {
        url: fullUrl,
        method: 'GET'
    };
    var RetData = "";
    var temp = 0;
    RetData = sReq('GET', fullUrl).getBody().toString();
    /*request(options)
        .on('data', function(data){
            RetData = data.toString();
        })
        .on('error', function(err) {
            console.log(err);
            return(1);
        })*/
    return RetData;
}

exports.postData = function(url, uuid, data){
    var fullUrl = url+"/user_data";
    var options = {
        url: fullUrl,
        method: 'POST',
        body: data
    };
    request(options)
}

