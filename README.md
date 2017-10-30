# Wi-Fi Sniffer as a Human detector -> now useable with OpenHAB

This project is a further developed fork of Andreas Spiess' "Wi-Fi Sniffer as a Human detector" to be used as a real human detector together with OpenHAB and MQTT.

The code is able to detect all connected WiFi devices with a stable/steady MAC address, stores them for at least 10 minutes in the internal DB (huge array, could be optimized for freeing more RAM) and if the MAC is not seen again within this timeframe removes it again.

Main idea behind this fork was to make the mqtt provided data useable in openHAB. So I finally set up my phones and the phone of my wife as "Contact" items, whenever they receive an OPEN, someone is at home and if they leave, the code sends out CLOSE to close the contact item. This works pretty well, especially when using two or more ESP8266 using this code and letting them post to different topics, which I analyse differently in OpenHAB.

### Things you need:
- At least an ESP8266 "like" device, this can be a pure 12F, NodeMCU or wemos D1 etc.
- Arduino with pubSubClient library
- Arduino with ArduinoJson library
- Modified pubSubClient.h: "#define MQTT_MAX_PACKET_SIZE 2048" at the beginning of pubSubClient.h file
- Running mqtt broker
- A common mobile phone with no additional software, just working WiFi, preferably connected to your WiFi

### Things to do before you start:
- In mqtt.h update mqttServer = "Your Broker IP address"
- replace MQTT_USERNAME, MQTT_KEY if you do not use Peter Scargills script or you changed the "admin, admin".
- In WiFi_Sniffer_OpenHAB.ino set "MySSID" and "MyPassword" with own WiFi credentials
- The current mqtt topic is "wifi/sniffer1" --> Search code to replace string, if you want to modify it

### Weakness:
Of course there is also some weakness, you need to be aware of. This setup only works with switched-on mobile phones, if you want to use these as presence trackers. If you put your phone into flight mode over night or turn it off, then you are marked as beeing absent, which might lead to an unwanted situation.


## Todo:
- Optimize setup and plce all setup vars more visible at top
- Optimize file structure, e.g. get rid of mqtt.h
