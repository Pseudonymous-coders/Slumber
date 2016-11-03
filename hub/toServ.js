var request = require('request');
var basics = require('./basics');
var exports = module.exports = {};

var tempUrl = "http://72.180.45.88:6767";
var user = "43a59d21-6bb5-4fe4-bdb1-81963d7a24a8"

exports.userData = function(url, uuid, start, end){
    var fullUrl = url+"user_data?";
    if (start) {
        fullUrl = fullUrl+"&start="+start;
    }
    if (end) {
        fullUrl = fullUrl+"&end="+end;
    }
    var options = {
        url: fullUrl,
        method: 'GET',
        headers: {
            'uuid': uuid,
            'Accept': 'application/json',
            'Accept-Charset': 'utf-8'
        }
    };
    request(options)
        .on('data', function(data){
            data = data.toString();
            console.log(data);
        })
}


exports.sendData = function(url, data, uuid) {
    toSend = {
        uuid: uuid,
        data: data
    }
    request.post(url+"/postData", toSend);
}
