//https://www.emqx.com/en/blog/esp8266-connects-to-the-public-mqtt-broker

#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "DHT.h"



#define DHTPIN 4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);


// WiFi
const char *ssid = "";       // Enter your Wi-Fi name
const char *password = "";  // Enter Wi-Fi password

// MQTT Broker
const char *mqtt_broker = "broker.hivemq.com";
const char *topic = "car_meteo";
const char *mqtt_username = "test";
const char *mqtt_password = "test123";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  // Set software serial baud to 115200;
  Serial.begin(115200);

  dht.begin();

  delay(2000);
  // Connecting to a WiFi network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the Wi-Fi network");
  //connecting to a mqtt broker
  client.setServer(mqtt_broker, mqtt_port);
  while (!client.connected()) {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public MQTT broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Public EMQX MQTT broker connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
  // Publish and subscribe
  client.publish(topic, "Hi, I'm ESP32 ^^");
}


void loop() {

  String h = String((float)dht.readHumidity());
  String t = String((float)dht.readTemperature());
  
  String message = "Temperature : " + t + "\n\nHumidity : " + h;

  client.publish(topic, message.c_str());

  delay(1000);
}
