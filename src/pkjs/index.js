var Clay = require('pebble-clay');
var clayConfig = require('./config');
//var mapRequest = require('./map_request');
//var userDataJson = {token: 'abc123', lat: 0, lng: 0, autoLoc: "1"};
var clay = new Clay(clayConfig);

var owmApiKey = 'cf3783bad968bcef693601e4ce1e250a';

function iconFromWeatherId(weatherId) {
  if (weatherId < 300) {           //Thunderstorm
    return 0;
  } else if (weatherId < 600) {    // Rain
    return 1;
  } else if (weatherId < 700) {    // Snow
    return 2;
  } else if (weatherId == 800) {   // Clear
    return 3;
  } else if (weatherId < 900) {    // Clouds
    return 4;
  } else {
    return 3;
  }
}

function timeFromUnix(unixtime) {
  var date = new Date(unixtime * 1000);
  var timestr = date.toLocaleTimeString();
  return timestr;
}

function fetchWeather(lat, long) {
	var req = new XMLHttpRequest();
	var weatherreq = 'http://api.openweathermap.org/data/2.5/weather?' + 
		'lat=' + lat + '&lon=' + long + '&cnt=1&appid=' + owmApiKey + '&units=imperial';
  console.log(weatherreq);
	req.open('GET', 'http://api.openweathermap.org/data/2.5/weather?' + 
		'lat=' + lat + '&lon=' + long + '&cnt=1&appid=' + owmApiKey + '&units=imperial', true);
		
	req.onload = function() {
		if (req.readyState === 4) {
			if (req.status === 200) {
			  console.log(req.responseText);
        var response = JSON.parse(req.responseText);
        var temperature = Math.round(response.main.temp);
        var icon = iconFromWeatherId(response.weather[0].id);
        var sunrise = timeFromUnix(response.sys.sunrise);
        var sunset = timeFromUnix(response.sys.sunset);
        Pebble.sendAppMessage({
          'WEATHER_ICON_KEY': icon,
          'WEATHER_TEMPERATURE_KEY': temperature + "\u00B0",
          'WEATHER_SUNRISE_KEY': sunrise,
          'WEATHER_SUNSET_KEY': sunset
        }, function(e) {
          console.log('Send successful!');
        }, function(e) {
          console.log('Send FAILED');
        });
			} else {
        console.log('Error: ' + JSON.stringify(req));
      }
		}
	};
  req.send(null);
}

function locationSuccess(pos) {
  console.log("location success");
  var coordinates = pos.coords;
  fetchWeather(coordinates.latitude, coordinates.longitude);
}

function locationError(err) {
  console.warn('location error (' + err.code + '): ' + err.message);
  Pebble.sendAppMessage({
    'WEATHER_CITY_KEY': 'Loc Unavailable',
    'WEATHER_TEMPERATURE_KEY': 'N/A'
  });
}

var locationOptions = {
  'timeout': 15000,
  'maximumAge': 60000
};

Pebble.addEventListener('ready', function (e) {
  window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError,
                                                  locationOptions);
});

Pebble.addEventListener('appmessage', function (e) {
  window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError,
    locationOptions);
  
});

Pebble.addEventListener('webviewclosed', function (e) {
  /*
  var response = e.response;
  console.log("webviewclosed");
  response = response.replace(/%7B/g, '{')
    .replace(/%22/g, "\"")
    .replace(/%3A/g, ":")
    .replace(/%7D/g, "}")
    .replace(/%2C/g, ",")
    .replace(/%25/g, "%")
    .replace(/%2F/g, "/");
  response = JSON.parse(response);

  var lat = response.LOCATION_LAT.value;
  var lng = response.LOCATION_LNG.value;
  var autoLoc = response.LOCATION_MODE.value;
  
  clay.meta.userData.lat = lat;
  clay.meta.userData.lng = lng;
  clay.meta.userData.autoLoc = autoLoc;
  fetchWeather(lat, lng);
  */
});
