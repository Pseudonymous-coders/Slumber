var request = require('request');
var basics = require('./basics');

var protocol = "http";
var url = protocol+"://"+"pseudonymous.ddns.net";
url = protocol+"://"+"72.180.45.88";
var port = 6767;
var total = url+":"+port;

var uuid = "43a59d21-6bb5-4fe4-bdb1-81963d7a24a8"

function userData(user, start, end){
    var fullUrl = url+":"+port+"/"+"user_data?uuid="+user;
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
userData(uuid);
