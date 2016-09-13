module.exports = [
  {
    "type": "heading",
    "defaultValue": "App Config"
  },
  {
    "type":"text",
    "defaultValue":"Intro text"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Colors"
      },
      {
        "type": "color",
        "messageKey": "BACKGROUND_COLOR",
        "defaultValue": "0x000000",
        "label": "Background Color"
      },
      {
        "type": "color",
        "messageKey": "CLOCK_COLOR",
        "defaultValue": "0x55FFFF",
        "label": "Background Color"
      }
    ]
  },
  {
    "type": "select",
    "messageKey": "WEATHER_INTERVAL",
    "defaultValue": 15,
    "label": "Weather Update Interval",
    "options": [
      { 
        "label": "", 
        "value": 15 
      },
      { 
        "label": "5 minutes",
        "value": 5
      },
      { 
        "label": "15 minutes",
        "value": 15 
      },
      { 
        "label": "30 minutes",
        "value": 30
      },
      { 
        "label": "1 hour",
        "value": 60
      }
    ]
  }
  
];