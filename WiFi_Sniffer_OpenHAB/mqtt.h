#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient client(espClient);

// MQTT
const char* mqttServer = "192.168.0.253";

