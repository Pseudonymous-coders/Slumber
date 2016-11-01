var request = require('request')

var url = "pseudonymous.ddns.net";
var port = 6767;
var total = url+":"+port;

var uuid = "43a59d21-6bb5-4fe4-bdb1-81963d7a24a8"

function getAllSleep(uuid){
    var options = {
        url: total+"/"+uuid,
        method: 'GET',
        headers: {
            'Accept': 'application/json',
            'Accept-Charset': 'utf-8'
        }
    };
    request(options, function(err, res, body){
        var json = JSON.parse(body);
        return json;
    });
}

var sleep = getAllSleep();
