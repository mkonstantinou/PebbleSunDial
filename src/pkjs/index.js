var myAPIKey = 'cf3783bad968bcef693601e4ce1e250a';

function iconFromWeatherId(weatherId) {
	if (weatherId < 600) {
		return 2;
	} else if (weatherId < 700) {
		return 3;
	} else if (weatherId < 800) {
		return 1;
	} else {
		return 0;
	}
}

function fetchWeather(lat, long) {
	var req = new XMLHttpRequest();
	
	req.open('GET', 'http://api.openweathermap.org/data/2.5/weather?' + 
		'lat=' + lat + '&lon=' + long + '&cnt=1&appid=' + myAPIKey + '&units=imperial', true);
		
	req.onload = function() {
		if (req.readyState === 4) {
			if (req.status === 200) {
			  console.log(req.responseText);
        var response = JSON.parse(req.responseText);
        var temperature = Math.round(response.main.temp);
        var icon = iconFromWeatherId(response.weather[0].id);
        var city = response.name;
        var sunrise = response.sys.sunrise;
        var sunset = response.sys.sunset;
        console.log('Sunrise: ' + sunrise);
        console.log('Sunset: ' + sunset);
        Pebble.sendAppMessage({
          'WEATHER_ICON_KEY': icon,
          'WEATHER_TEMPERATURE_KEY': temperature + "\u00B0",
          'WEATHER_CITY_KEY': city,
          'WEATHER_SUNRISE_KEY': sunrise,
          'WEATHER_SUNSET_KEY': sunset
        }, function(e) {
          console.log('Send successful!');
        }, function(e) {
          console.log('Send FAILED');
        });
			} else {
        console.log('Error');
      }
		}
	};
  req.send(null);
}

function locationSuccess(pos) {
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
  console.log('connect!' + e.ready);
  window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError,
    locationOptions);
  console.log(e.type);
});

Pebble.addEventListener('appmessage', function (e) {
  window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError,
    locationOptions);
  console.log(e.type);
  console.log(e.payload.temperature);
  console.log('message!');
});

Pebble.addEventListener('webviewclosed', function (e) {
  console.log('webview closed');
  console.log(e.type);
  console.log(e.response);
});
