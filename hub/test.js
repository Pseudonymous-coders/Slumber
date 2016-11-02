var request = require('request');

request
    .get("http://72.180.45.88:6767/user_data?uuid=43a59d21-6bb5-4fe4-bdb1-81963d7a24a8&start=1")
    .on('response', function(res){
        console.log(res.statusCode);
        console.log()
    })
    .on('data', function(data){
        console.log(data.toString());
    })
    .on('error', function(err){
        console.log("Error:  "+err);
    })
