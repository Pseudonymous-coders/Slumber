var request = require('request')

var url = "pseudonymous.ddns.net";
var port = 6767;
var total = url+":"+port;

var uuid = "43a59d21-6bb5-4fe4-bdb1-81963d7a24a8"

function getAllSleep(user){
    var options = {
        url: total+"/"+user,
        method: 'GET',
        headers: {
            'Accept': 'application/json',
            'Accept-Charset': 'utf-8'
        }
    };
    request(options, function(err, res, body){
        //var json = JSON.parse(body);
        json = body;
        console.log(options.url)
        return json;
    });
}

var sleep = getAllSleep(uuid);
console.log(sleep);
