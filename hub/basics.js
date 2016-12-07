String.prototype.format = function () {
  var i = 0, args = arguments;
  return this.replace(/{}/g, function () {
      return typeof args[i] != 'undefined' ? args[i++] : '';
    });
};

user = "43a59d21-6bb5-4fe4-bdb1-81963d7a24a8";
tempUrl = "http://eli-server.ddns.net:6767"

