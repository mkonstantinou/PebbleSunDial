var myAPIKey = '';

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
		'lat=' + lat + '&lon=' + long + '&cnt=1&appid=' + myAPIKey, true);
		
	req.onload = function() {
		if (req.readyState === 4) {
			if (req.status === 200) {
			
			}
		}
	}
}