# Wi-Fi Sniffer as a Human detector - now useable with OpenHAB

This project is a further developed fork of Andreas Spiess' "Wi-Fi Sniffer as a Human detector" to be used as a real human detector together with [OpenHAB](http://openhab.org) and MQTT.

The code is able to detect all connected WiFi devices with a stable/steady MAC address, stores them for 10 minutes in the internal DB (huge array, could be optimized for freeing more RAM) and if the MAC is not seen again within this timeframe removes it again.

Main idea behind this fork was to make the MQTT provided data (simple JSON container) useable in openHAB by using transform methods in items. So I finally set up my phones and the phone of my wife as "Contact" items, whenever these items receive an OPEN, someone is at home and if devices leave, the code sends out CLOSE to close the contact item. This works pretty well, especially when using two or more ESP8266 using this code and letting them post to different topics, which I analyse and interpet differently in OpenHAB.

In openHAB you simply define a phone as a Contact item in an MQTT binding and with using tricky JSON parser of openHAB, you first ensure, the provided JSON string includes the MAC of the device to track (e.g. ec1f7208aaaa) and then parse the value with `JSONPATH($.MAC.ec1f7208aaaa)` out of the provided JSON string:

    Contact anwesenheit_andreas_android1 "Andreas Android 1 [MAP(anwesenheit.map):%s]"  <man_2> (Alles,AnwesenheitDevices)
    {
        mqtt="<[mqttbroker:wifi/sniffer1:state:JSONPATH($.MAC.ec1f7208aaaa):.*ec1f7208aaaa.*]"
    }

Finally provided and formated JSON string of topic 'wifi/sniffer1' will e.g. look like:

    {
        "MAC": {
            "9ec7a6dacdf2": "OPEN",
            "ec1f7208aaaa": "OPEN",
            [...]
            "b827eb5abcde": "OPEN"
            "74da3863ffff": "CLOSED"
        }
    }

All "OPEN" entries are seen devices and all "CLOSED" devices just left and trigger openHAB to close the contact switch. This event can easily be handeld by a rule:

    rule "Andreas-Android-Check"
    when
        Item anwesenheit_andreas_android1 changed or
        Item anwesenheit_andreas_android2 changed
    then    
        var String andreasAndroid1 = transform("MAP","anwesenheit.map", anwesenheit_andreas_android1.state.toString)
        var String andreasAndroid2 = transform("MAP","anwesenheit.map", anwesenheit_andreas_android2.state.toString)

        logInfo("Andreas-Device-Status-Change", "Andreas Android Status: " + andreasAndroid1 + "/" + andreasAndroid2)

        var Boolean anwesenheit_andreas_android = (anwesenheit_andreas_android1.state == OPEN || anwesenheit_andreas_android2.state == OPEN)

        if (anwesenheit_andreas_android && anwesenheit_andreas.state != OPEN)
        {
            // Do some cool stuff here
            [...]
        }
    end

In this example there are two "devices" setup as items and put together with OR as I check the MQTT topics of two wifi scanners, one at the front of the house and one at the other end, just to be sure, to know, we are "somewhere" at home. It happens from time to time that only one scanner detects the device, to this shall not immediately trigger a "present off" event.

### Things you need:
- At least an ESP8266 "like" device, this can be a pure 12F, NodeMCU or wemos D1 etc.
- Arduino with pubSubClient library
- Arduino with ArduinoJson library
- Modified pubSubClient.h: *"#define MQTT_MAX_PACKET_SIZE 2048"* at the beginning of pubSubClient.h file
- Running mqtt broker
- A common mobile phone **with no additional software**, just working WiFi, preferably connected to your WiFi

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
- Optimize usage of multiple topics for different devices --> Requieres re-think
- Optimize data struct to a simple char array or string to free some more memory
- Extend documenation beyond this file