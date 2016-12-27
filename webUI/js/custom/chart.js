google.charts.load('current', {'packages':['line']});
google.charts.setOnLoadCallback(drawChart);

var url = "http://192.168.1.4:6767/user_data/uuid/accel/0/999999999999999999999"

function lastNight() {
  var xmlReq = new XMLHttpRequest();
  xmlReq.open("GET", "http://192.168.1.4:6767/user_data/asda/all/lastNight", false);
  xmlReq.send(null);
  var sleepData = JSON.parse(xmlReq.responseText).slice(0,500);
  var sleepArr = [];
  var firstTime = sleepData[0].time;
  var lastTemp = 0;
  var lastHum = 0;
  sleepData.forEach((item)=>{
    var sleepTime = item.time - firstTime;
    var sleepTemp = item.data.temp;
    var sleepHum = item.data.hum;
    if (sleepTemp != lastTemp){
      sleepArr.push([sleepTime, sleepTemp, sleepHum]);
    }
    if (sleepHum != lastHum){
      sleepArr.push([sleepTime, sleepTemp, sleepHum]);
    }
    lastTemp = sleepTemp;
    lastHum = sleepHum;
  })
  return sleepArr;
}


function drawChart() {
  var data = new google.visualization.DataTable();
  data.addColumn('number', 'Time');
  data.addColumn('number', 'Temp');
  data.addColumn('number', 'Humidity');

  data.addRows(lastNight());


  var options = {
    width: '100%',
    height: '100%',
    legend: {position: 'bottom'},
    curveType: 'function'
  };

  var chart = new google.charts.Line(document.getElementById('lastNight'));

  chart.draw(data, options);
}
