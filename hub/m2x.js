var exports = module.exports = {}
M2X = require('m2x');
m2x = new M2X("19f56759ada36266960cc37b2ee3537c");
var beginning = "";
 
exports.logVal = function(stream, val) {
    m2x.devices.setStreamValue("15df8a57671bb58f633afa646f348dbc", stream, { "timestamp": new Date().toISOString(), "value": val });
}

exports.clearVals = function(stream) {
    m2x.devices.deleteStream("15df8a57671bb58f633afa646f348dbc", stream);
    m2x.devices.updateStream("15df8a57671bb58f633afa646f348dbc", stream, {display_name: stream});
}

exports.clearVals("accel-y");
