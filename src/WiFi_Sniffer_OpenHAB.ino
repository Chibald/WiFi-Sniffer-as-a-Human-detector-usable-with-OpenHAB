/*
  Copyright 2017 Andreas Spiess

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
  FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  This software is based on the work of Ray Burnette: https://www.hackster.io/rayburne/esp8266-mini-sniff-f6b93a

  This software is modified and extended by Dipl.-Inform. (FH) Andreas Link, http://AndreasLink.de to be usable with OpenHAB as a Contact-item (http://openhab.org).

    Andreas iPhone  : d04f7ecfaaaa
    Andreas S6edge  : ec1f7208aaaa
    Maike Samsung A3: ec107b5cbbbb
*/

#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <set>
#include <string>
#include "./functions.h"
#include "./mqtt.h"

#define disable 0
#define enable  1
#define SENDTIME 30000
#define MAXDEVICES 80
#define JBUFFER 15+ (MAXDEVICES * 40)
#define PURGETIME 600000 // aka 600 sec = 10 Min
#define MINRSSI -150  //Try to catch, how far ever they are heard

// uint8_t channel = 1;
unsigned int channel = 1;
int clients_known_count_old, aps_known_count_old;
unsigned long sendEntry, deleteEntry;
char jsonString[JBUFFER];


String device[MAXDEVICES];
int nbrDevices = 0;
int usedChannels[15];

// ----------------------------------------------------------------------------------- //

#define mySSID "MySSID"
#define myPASSWORD "MyPassword"

//Comment this block, if you prefer not to use a static IP
IPAddress ip = IPAddress(192,168,0,42);
IPAddress subnet = IPAddress(255,255,255,0);
IPAddress gw = IPAddress(192,168,0,1);
IPAddress dns = IPAddress(192,168,0,1);


// ----------------------------------------------------------------------------------- //

StaticJsonBuffer<JBUFFER>  jsonBuffer;

void setup() 
{
  Serial.begin(115200);
  Serial.printf("\n\nSDK version:%s\n\r", system_get_sdk_version());
  Serial.println(F("Human detector by Andreas Spiess. ESP8266 mini-sniff by Ray Burnette http://www.hackster.io/rayburne/projects"));
  Serial.println(F("Based on the work of Ray Burnette http://www.hackster.io/rayburne/projects"));
  Serial.println(F("Finally modified to work with OpenHAB by AndreasLink.de"));

  wifi_set_opmode(STATION_MODE);            // Promiscuous works only with station mode
  wifi_set_channel(channel);
  wifi_promiscuous_enable(disable);
  wifi_set_promiscuous_rx_cb(promisc_cb);   // Set up promiscuous callback
  wifi_promiscuous_enable(enable);
}


void loop() 
{
  channel = 1;
  boolean sendMQTT = false;
  wifi_set_channel(channel);
  while (true) 
  {
    nothing_new++;               // Array is not finite, check bounds and adjust if required
    if (nothing_new > 200)       // monitor channel for 200 ms
    {                
      nothing_new = 0;
      channel++;
      
      if (channel == 15) 
        break;             // Only scan channels 1 to 14
        
      wifi_set_channel(channel);
    }
    delay(1);  // critical processing timeslice for NONOS SDK! No delay(0) yield()

    if (clients_known_count > clients_known_count_old) 
    {
      clients_known_count_old = clients_known_count;
      sendMQTT = true;
    }
    
    if (aps_known_count > aps_known_count_old) 
    {
      aps_known_count_old = aps_known_count;
      sendMQTT = true;
    }
    
    if (millis() - sendEntry > SENDTIME) 
    {
      sendEntry = millis();
      sendMQTT = true;
    }
  }
  
  purgeDevice();  //Remove old devices from internal DB
  
  if (sendMQTT) 
  {
    showDevices();
    sendDevices();
  }
}


void connectToWiFi() 
{
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to '");
  Serial.print(mySSID);
  Serial.println("'");

  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gw, subnet, dns);
  WiFi.begin(mySSID, myPASSWORD);

  int connectionCounter = 0;

  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
    connectionCounter++;

    //Finally reboot the ESP if there is no WiFi for at least a minute
    if (connectionCounter > 120)
      ESP.restart();
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void purgeDevice() 
{
  for (int u = 0; u < clients_known_count; u++) 
  {
    if ((millis() - clients_known[u].lastDiscoveredTime) > PURGETIME) 
    {
      Serial.print("Purge Client ");
      Serial.print(u);
      Serial.print(" --> ");
      Serial.println(formatMac1(clients_known[u].station));
      
      purge_client(clients_known[u]); //Add purged station to array, to identify for openhab "logout" MQTT value
      
      for (int i = u; i < clients_known_count; i++) 
        memcpy(&clients_known[i], &clients_known[i + 1], sizeof(clients_known[i]));
        
      clients_known_count--;
      break;
    }
  }
  
  for (int u = 0; u < aps_known_count; u++) 
  {
    if ((millis() - aps_known[u].lastDiscoveredTime) > PURGETIME) 
    {
      Serial.print("Purge Bacon" );
      Serial.println(u);
      for (int i = u; i < aps_known_count; i++) 
        memcpy(&aps_known[i], &aps_known[i + 1], sizeof(aps_known[i]));
        
      aps_known_count--;
      break;
    }
  }
}


void showDevices() 
{
  Serial.println();
  Serial.println();
  Serial.println("-------------------Device DB-------------------");
  Serial.println("APs and devices: ");
  Serial.println(aps_known_count + clients_known_count);

  // add Beacons
  for (int u = 0; u < aps_known_count; u++) 
  {
    Serial.print("Beacon ");
    Serial.print(formatMac1(aps_known[u].bssid));
    Serial.print(", RSSI ");
    Serial.print(aps_known[u].rssi);
    Serial.print(", channel ");
    Serial.println(aps_known[u].channel);
  }

  Serial.println();

  // add Clients
  for (int u = 0; u < clients_known_count; u++) 
  {
    Serial.print("Client ");
    Serial.print(formatMac1(clients_known[u].station));
    Serial.print(", RSSI ");
    Serial.print(clients_known[u].rssi);
    Serial.print(", channel ");
    Serial.println(clients_known[u].channel);
  }
}

void sendDevices() 
{
  String deviceMac;

  // Setup MQTT
  wifi_promiscuous_enable(disable);
  connectToWiFi();
  
  client.setServer(mqttServer, 1883);
  while (!client.connected()) {
    Serial.print("Connecting to MQTT... ");

    String clientId = "ESPClient-";
    //clientId += String(random(0xffff), HEX);
    clientId += String(ESP.getChipId());
    
    if (client.connect(clientId.c_str()))
    {
      Serial.print("connected as ");
      Serial.println(clientId);      
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
    }
    yield();
  }

  // Purge json string
  jsonBuffer.clear();
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& macrssi = root.createNestedObject("MAC");

  // add Beacons
  for (int u = 0; u < aps_known_count; u++) 
  {
    deviceMac = formatMac1(aps_known[u].bssid);
    if (aps_known[u].rssi > MINRSSI) 
    {
      //mac.add(deviceMac);
      //macrssi[deviceMac] = "Ch: " + String(aps_known[u].channel) + ", RSSI: " + String(aps_known[u].rssi) + "dBm";
      macrssi[deviceMac] = "OPEN";
    }
  }

  // Add Clients
  for (int u = 0; u < clients_known_count; u++) 
  {
    deviceMac = formatMac1(clients_known[u].station);
    if (clients_known[u].rssi > MINRSSI) 
    {
      //mac.add(deviceMac);
      //macrssi[deviceMac] = "Ch: " + String(clients_known[u].channel) + ", RSSI: " + String(clients_known[u].rssi) + "dBm";
      macrssi[deviceMac] = "OPEN";
    }
  }

  //Add purged clients
  if (clients_purged_count > 0)
  {
    for (int u = 0; u < clients_purged_count; u++) 
    {
      deviceMac = formatMac1(clients_purged[u].station);
      macrssi[deviceMac] = "CLOSED";
    }
    clients_purged_count = 0;
  }

  Serial.println();
  Serial.printf("Number of devices: %02d\n", macrssi.size());
  root.prettyPrintTo(Serial);
  root.printTo(jsonString);
  if (client.publish("wifi/sniffer1", jsonString) == 1)
  {
    Serial.println();
    Serial.println("Successfully published");
  } else {
    Serial.println();
    Serial.println("!!!!! Not published. please add #define MQTT_MAX_PACKET_SIZE 2048 at the beginning of pubSubClient.h file");
    Serial.println();
  }
  client.loop();
  client.disconnect ();
  delay(100);
  wifi_promiscuous_enable(enable);
  sendEntry = millis();
}

