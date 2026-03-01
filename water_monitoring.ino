#include <WiFi.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

/* ================= WIFI ================= */

const char* ssid = "Galaxy M34 5G 379E";
const char* password = "qwerty@12345";

/* ================= THINGSPEAK ================= */

String apiKey = "YOUR_API_KEY";
const char* server = "http://api.thingspeak.com/update";

/* ================= PIN DEFINITIONS ================= */

#define PH_PIN 34
#define ONE_WIRE_BUS 4
#define TRIG_PIN 5
#define ECHO_PIN 18

/* ================= TEMPERATURE SENSOR ================= */

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

/* ================= TANK PARAMETERS ================= */

float tankHeight = 20.0;   // cm → ADJUST TO YOUR TANK

/* ================= FILTER SETTINGS ================= */

int samples = 10;          // Averaging samples

/* ================= SETUP ================= */

void setup() {

  Serial.begin(115200);

  sensors.begin();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  connectWiFi();
}

/* ================= MAIN LOOP ================= */

void loop() {

  float phValue = readPH();
  float temperatureValue = readTemperature();
  float waterLevel = readWaterLevel();

  Serial.println("\n----- SENSOR DATA -----");
  Serial.print("pH: ");
  Serial.println(phValue);

  Serial.print("Temperature (C): ");
  Serial.println(temperatureValue);

  Serial.print("Water Level (cm): ");
  Serial.println(waterLevel);

  detectAnomalies(phValue, temperatureValue, waterLevel);

  uploadToThingSpeak(phValue, temperatureValue, waterLevel);

  delay(5000);   // ThingSpeak safe interval
}

/* ================= WIFI CONNECTION ================= */

void connectWiFi() {

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");
}

/* ================= pH SENSOR ================= */

float readPH() {

  float total = 0;

  for (int i = 0; i < samples; i++) {
    total += analogRead(PH_PIN);
    delay(10);
  }

  float raw = total / samples;

  float voltage = raw * (3.3 / 4095.0);

  float ph = 7 + ((2.5 - voltage) / 0.18);

  return ph;
}

/* ================= TEMPERATURE SENSOR ================= */

float readTemperature() {

  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);

  return temp;
}

/* ================= ULTRASONIC SENSOR ================= */

float readWaterLevel() {

  float totalDistance = 0;

  for (int i = 0; i < samples; i++) {

    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);

    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH);

    float distance = duration * 0.034 / 2;

    totalDistance += distance;

    delay(20);
  }

  float avgDistance = totalDistance / samples;

  float level = tankHeight - avgDistance;

  if (level < 0) level = 0;

  return level;
}

/* ================= ANALYTICS ================= */

void detectAnomalies(float ph, float temp, float level) {

  if (ph < 6.5 || ph > 8.5)
    Serial.println("⚠ pH OUT OF SAFE RANGE");

  if (temp > 35)
    Serial.println("⚠ HIGH TEMPERATURE");

  if (level < 3)
    Serial.println("⚠ LOW WATER LEVEL");
}

/* ================= THINGSPEAK UPLOAD ================= */

void uploadToThingSpeak(float ph, float temp, float level) {

  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  HTTPClient http;

  String url = server;
  url += "?api_key=" + apiKey;
  url += "&field1=" + String(ph);
  url += "&field2=" + String(temp);
  url += "&field3=" + String(level);

  http.begin(url);
  int httpResponseCode = http.GET();

  Serial.print("ThingSpeak Response: ");
  Serial.println(httpResponseCode);

  http.end();
}