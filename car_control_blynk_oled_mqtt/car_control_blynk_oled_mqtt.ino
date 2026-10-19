#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME "test"
#define BLYNK_AUTH_TOKEN ""

#include <BlynkSimpleEsp32.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <NewPing.h>

// Ultrasonic sensor
#define TRIG_PIN 5
#define ECHO_PIN 18
#define MAX_DISTANCE 400  // cm

NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);

// oled
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

int status = WL_IDLE_STATUS;

// motor pins
int rightSideIn1 = 27;
int rightSideIn2 = 26;
int rightPWM = 14;
int leftSideIn1 = 33;
int leftSideIn2 = 32;
int leftPWM = 13;


// PWM settings
int freq = 5000;     // 5 kHz
int resolution = 8;  // 8bits -> 0-255

// network
char ssid[] = ""; 
char pass[] = "";

// MQTT Broker
const char *mqtt_broker = "broker.hivemq.com";
const char *topic = "car_meteo";
const char *mqtt_username = "test";
const char *mqtt_password = "test123";
const int mqtt_port = 1883;

char message[128] = "Waiting...";

WiFiClient espClient;
PubSubClient client(espClient);

// Virtual pin V0 - triggered when Blynk button is pressed/released
BLYNK_WRITE(V0) {
  int forward = param.asInt();  // 1 = pressed, 0 = released

  if (forward == 1) {
    moveForward(200);
  } else {
    stopMoving();
  }
}

// Virtual pin V1 - triggered when Blynk button is pressed/released
BLYNK_WRITE(V1) {
  int right = param.asInt();  // 1 = pressed, 0 = released

  if (right == 1) {
    leftSideOn(255);
    rightSideOn(30);
  } else {
    stopMoving();
  }
}

// Virtual pin V2 - triggered when Blynk button is pressed/released
BLYNK_WRITE(V2) {
  int backward = param.asInt();  // 1 = pressed, 0 = released

  if (backward == 1) {
    moveBackwards(200);
  } else {
    stopMoving();
  }
}

// Virtual pin V3 - triggered when Blynk button is pressed/released
BLYNK_WRITE(V3) {
  int left = param.asInt();  // 1 = pressed, 0 = released

  if (left == 1) {
    rightSideOn(255);
    leftSideOn(30);
  } else {
    stopMoving();
  }
}




void setup() {
  Serial.begin(115200);
  pinMode(rightSideIn1, OUTPUT);
  pinMode(rightSideIn2, OUTPUT);
  pinMode(leftSideIn1, OUTPUT);
  pinMode(leftSideIn2, OUTPUT);

  // Right PWM setup
  ledcAttach(rightPWM, freq, resolution);

  // Left PWM setup
  ledcAttach(leftPWM, freq, resolution);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

  delay(2000);
  displayText("Hello World");

  //connecting to a mqtt broker
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  while (!client.connected()) {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public MQTT broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      displayText("Public EMQX MQTT broker connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  // Publish and subscribe
  client.publish(topic, "Hi, I'm ESP32 ^^");
  client.subscribe(topic);
}

void callback(char *topic, byte *payload, unsigned int length) {
  int len = min((int)length, 127);
  for (int i = 0; i < len; i++) {
    message[i] = (char)payload[i];
    //Serial.print((char)payload[i]);
  }
  message[len] = '\0';
}

void loop() {
  Blynk.run();
  client.loop();
  displayText(message);

  unsigned int distance = ultraSonicRead();
  if (distance > 0 and distance <= 3) {
    while (distance > 0 and distance <= 3) {
      displayText("Obstacle too close\nBacking up...");
      moveBackwards(200);
      distance = ultraSonicRead();
    }
    stopMoving();
  }
}

unsigned int ultraSonicRead() {
  // Ultrasonic sensor
  delay(50);  // wait between pings
  unsigned int distance = sonar.ping_cm();
  Serial.print("Distance: ");
  Serial.println(distance);
  return distance;
}

void displayText(char *text) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 10);
  // Display static text
  display.println(text);
  display.display();
}

void leftSideOn(int speed) {
  digitalWrite(leftSideIn1, HIGH);
  digitalWrite(leftSideIn2, LOW);

  ledcWrite(leftPWM, speed);
}

void leftSideBackwards(int speed) {
  digitalWrite(leftSideIn1, LOW);
  digitalWrite(leftSideIn2, HIGH);

  ledcWrite(leftPWM, speed);
}

void leftSideOff() {
  digitalWrite(leftSideIn1, LOW);
  digitalWrite(leftSideIn2, LOW);

  ledcWrite(leftPWM, 0);
}

void rightSideOn(int speed) {
  digitalWrite(rightSideIn1, HIGH);
  digitalWrite(rightSideIn2, LOW);

  ledcWrite(rightPWM, speed);
}


void rightSideOff() {
  digitalWrite(rightSideIn1, LOW);
  digitalWrite(rightSideIn2, LOW);

  ledcWrite(rightPWM, 0);
}


void rightSideBackwards(int speed) {
  digitalWrite(rightSideIn1, LOW);
  digitalWrite(rightSideIn2, HIGH);

  ledcWrite(rightPWM, speed);
}

void moveForward(int speed) {
  rightSideOn(speed);
  leftSideOn(speed);
}

void stopMoving() {
  rightSideOff();
  leftSideOff();
}

void moveBackwards(int speed) {
  rightSideBackwards(speed);
  leftSideBackwards(speed);
}