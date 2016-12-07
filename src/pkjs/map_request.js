/*module.exports = function(minified) {
  var clayConfig = this;
  var _ = minified._;
  var $ = minified.$;
  var HTML = minified.HTML;
  
  console.log("Map_request");
  
  var googleMapsApiKey = "AIzaSyCBv-wLDTOob_zI9mP0RdHw7rHS1rQvlX0";
  
  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
    clayConfig.getItemByMessageKey('LOCATION_LAT').hide();
    clayConfig.getItemByMessageKey('LOCATION_LNG').hide();
  });
  
  clayConfig.on(clayConfig.EVENTS.BEFORE_DESTROY, function() {    
    var isAutomatic = clayConfig.getItemByMessageKey('LOCATION_MODE');
    var latitude = clayConfig.getItemByMessageKey('LOCATION_LAT');
    var longitude = clayConfig.getItemByMessageKey('LOCATION_LNG');
    
    clayConfig.meta.userData.autoLoc = isAutomatic;
    
    if (isAutomatic.get() === "0") {
      console.log("api request");
      var maps_api = "https://maps.googleapis.com/maps/api/geocode/json?";
      var address = "address=" + clayConfig.getItemByMessageKey('LOCATION_STRING').get();
      var key = "&key=" + googleMapsApiKey;
      
      console.log("Maps reqest: " + maps_api + address + key);
      
      $.request('post', maps_api + address + key, {token: clayConfig.meta.userData.token})
      .then(function(result) {
        result = JSON.parse(result).results;
        
        latitude.set(result[0].geometry.location.lat);
        longitude.set(result[0].geometry.location.lng);
      })
      .error(function(status, statusText, responseText) {
        console.log(status);
      });
    }
    
  });
  
};
*/