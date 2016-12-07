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
        "messageKey": "DATE_COLOR",
        "defaultValue": "0xFFFFFF",
        "label": "Date Color"
      },
      {
        "type": "color",
        "messageKey": "CLOCK_COLOR",
        "defaultValue": "0x55FFFF",
        "label": "Clock Color"
      },
      {
        "type": "color",
        "messageKey": "TEMP_COLOR",
        "defaultValue": "0xFFFFFF",
        "label": "Temperature Color"
      }
    ]
  },
  {
    "type": "heading",
    "defaultValue": "Weather Settings"
  },
  {
    "type": "select",
    "messageKey": "WEATHER_INTERVAL",
    "defaultValue": 15,
    "label": "Update Interval",
    "options": [
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
  },
  {
  	"type":"heading",
  	"defaultValue":"Date Settings"
  },
  {
    "type":"select",
  	"messageKey":"DATE_FORMAT",
  	"defaultValue":  "%m/%d",
  	"label":"Date format",
  	"options": [
      {
  			"label": "dd",
  			"value": "%d"
  		},
      {
  			"label": "mm/dd",
  			"value": "%m/%d"
  		},
  		{
  			"label": "dd/mm",
        "value": "%d-%m"
  		}
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
  
];