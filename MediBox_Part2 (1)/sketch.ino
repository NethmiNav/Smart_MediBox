#include <WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <cmath>
#include <ESP32Servo.h>
#include<NTPClient.h>
#include<WiFiUdp.h>


#define DHT_PIN 12
#define LDR_PIN 33
#define SERVO_PIN 14
#define BUZZER 04

WiFiClient espClient;
PubSubClient mqttClient(espClient);
DHTesp dhtSensor;
Servo servoMotor;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);


char tempAr[6];

bool isScheduledON = false;
unsigned long scheduledOnTime;

// Use a more unique client ID
String clientId = "esp1254785458" + String(random(0, 1000));
const char* esp_client = clientId.c_str(); // Valid as long as clientId stays in scope

int LDR_reading = 5*1000; // Sampling interval for LDR
int LDR_time_period = 120*1000; // 2 minutes ( Sending interval for LDR)
int MAX_ADC_VALUE = 4063; // Maximum ADC value for ESP32
int MIN_ADC_VALUE = 32;
int TEMP_send_interval = 1000; // 2 seconds
unsigned long LDR_VALUE_SUM = 0;
int minimum_angle = 30; // Minimum angle for servo
float controlling_factor = 0.75; // Controlling factor for servo
int ideal_temperature = 30; // Ideal temperature for servo
float normalizedLDR = 0.0; // Normalized LDR value

// topics
const char* LDR_DATA = "220415A/LDR_DATA";
const char* TEMP_DATA = "220415A/TEMP_DATA";

const char* LDR_reading_topic = "220415/reading_interval"; // sampling interval
const char* LDR_time_period_topic = "220415/time_period"; // sending interval
const char* minimum_angle_topic = "220415/minimum_angle"; // minimum angle
const char* controlling_factor_topic = "220415/controlling_factor"; // controlling factor
const char* ideal_temperature_topic = "220415/ideal_temperature"; // ideal temperature

void setup() {
  Serial.begin(115200);
  delay(2000);  // Pause for 2 seconds
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
  setupWiFi();
  
  timeClient.begin();
  timeClient.setTimeOffset(5.5*3600);

  // Set the callback function BEFORE connecting
  mqttClient.setCallback(receiveCallback);
  
  connectToBroker(); 

  servoMotor.setPeriodHertz(50);
  servoMotor.attach(SERVO_PIN, 500, 2400);

  pinMode(BUZZER,OUTPUT);
  digitalWrite(BUZZER,LOW);
}

void loop() {
  
  if (!mqttClient.connected()) connectToBroker();  mqttClient.setCallback(receiveCallback);  // Set the callback function
  mqttClient.loop();  // Keep connection alive

  // read LDR data every 5 second
  static unsigned long lastLDRTime = 0;
  if (millis() - lastLDRTime >= LDR_reading) {
    LDR_VALUE_SUM += MAX_ADC_VALUE - analogRead(LDR_PIN);
    lastLDRTime = millis();
  }

  // send LDR data every 2 minutes
  static unsigned long lastLDRSendTime = 0;
  if (millis() - lastLDRSendTime >= LDR_time_period) {
    float periods = LDR_time_period / LDR_reading;
    float avgLDR = (float)LDR_VALUE_SUM / periods;
    normalizedLDR = (avgLDR) / (MAX_ADC_VALUE - MIN_ADC_VALUE);
    char msg[12];
    dtostrf(normalizedLDR, 1, 4, msg);
    mqttClient.publish(LDR_DATA, msg);
    LDR_VALUE_SUM = 0; // Reset sum after sending
    lastLDRSendTime = millis();
  }

  // send temperature data every 2 seconds
  static unsigned long lastTempTime = 0;
  if (millis() - lastTempTime >= TEMP_send_interval) {
    TempAndHumidity data = dhtSensor.getTempAndHumidity();
    dtostrf(data.temperature, 1, 2, tempAr);
    mqttClient.publish(TEMP_DATA, tempAr);
    lastTempTime = millis();

  }
  

  static unsigned long last_update = 0;
  int time_now = millis();
  if ( time_now - last_update >= 2000) {
    last_update = time_now;

    servoMotor.write(constrain(calculateServoPosition(), 0, 180));
    //Serial.print("Servo angle updated");
    //Serial.println(constrain(calculateServoPosition(), 0, 180));
  }

  checkSchedule();

}

void connectToBroker() {
 
  const char* mqtt_server = "test.mosquitto.org";
  mqttClient.setServer(mqtt_server, 1883);
  
  int attempts = 0;
  while (!mqttClient.connected() && attempts < 5) {
    Serial.println("Connecting to MQTT broker...");
    if (mqttClient.connect(esp_client)) {
      Serial.println("Connected to MQTT broker");
      mqttClient.subscribe(LDR_reading_topic);
      mqttClient.subscribe(LDR_time_period_topic);
      mqttClient.subscribe(minimum_angle_topic);
      mqttClient.subscribe(controlling_factor_topic);
      mqttClient.subscribe(ideal_temperature_topic);
      mqttClient.subscribe("220415A/MAIN-ON-OFF");
      mqttClient.subscribe("220415A/SCH-ON");
    } else {
      Serial.print("Failed to connect, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" - trying again in 2 seconds");
      delay(2000);
      attempts++;
    }
  }
  if (!mqttClient.connected()) {
    Serial.println("Failed to connect to MQTT broker after several attempts");
    delay(10000);  // Wait longer before trying again
  }
}

void receiveCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");

  // Create a properly null-terminated string from the payload
  char message[length + 1];  // +1 for null terminator
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
    Serial.print((char)payload[i]);
  }
  message[length] = '\0';  // Add null terminator
  Serial.println();

  if (strcmp(topic, LDR_reading_topic) == 0) {
    LDR_reading = atoi(message) * 1000; // Convert to milliseconds
    Serial.print("LDR reading interval set to: ");
    Serial.println(LDR_reading);
  } else if (strcmp(topic, LDR_time_period_topic) == 0) {
    LDR_time_period = atof(message)* 1000; // Convert to milliseconds
    Serial.print("LDR time period set to: ");
    Serial.println(LDR_time_period);
  } else if (strcmp(topic, minimum_angle_topic) == 0) {
    minimum_angle = atoi(message);
    Serial.print("Minimum angle set to: ");
    Serial.println(minimum_angle);
  } else if (strcmp(topic, controlling_factor_topic) == 0) {
    controlling_factor = atof(message);
    Serial.print("Controlling factor set to: ");
    Serial.println(controlling_factor);
  } else if (strcmp(topic, ideal_temperature_topic) == 0) {
    ideal_temperature = atoi(message);
    Serial.print("Ideal temperature set to: ");
    Serial.println(ideal_temperature);
  }else if(strcmp(topic,"220415A/MAIN-ON-OFF")){
    buzzerOn(message[0]=='1');
  }else if(strcmp(topic,"220415A/SCH-ON")==1){
      if(message[0]=='N'){
        isScheduledON=false;
      }else{
        isScheduledON=true;
        scheduledOnTime = atol(message);
      }
  }

  else {
    Serial.println("Unknown topic");
  }
    
}

void buzzerOn(bool on){
  if(on){
    tone(BUZZER,256);
  }else{
    noTone(BUZZER);
  }
}

double calculateServoPosition() {
  // Returns angle in degrees based on sensor inputs and control logic
  
  TempAndHumidity sensorData = dhtSensor.getTempAndHumidity();
  double currentTemp = sensorData.temperature;  // Current temperature reading

  double angle = minimum_angle +
                 (180.0 - minimum_angle) *
                 normalizedLDR *
                 controlling_factor *
                 std::log((double)LDR_time_period / LDR_reading) *
                 (currentTemp / ideal_temperature);

  return angle;
}

void setupWiFi(){
  Serial.println();
  Serial.println("Connecting to ");
  Serial.println("Wokwi-Guest");
  WiFi.begin("Wokwi-GUEST", "");

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.println(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

unsigned long getTime(){
  timeClient.update();
  return timeClient.getEpochTime();
}

void checkSchedule(){
  if(isScheduledON){
    unsigned long currentTime =getTime();
    if(currentTime > scheduledOnTime){
      buzzerOn(true);
      isScheduledON=false;
      mqttClient.publish("220415A/ON-OFF-ESP","1");
      mqttClient.publish("220415A/SCH-ESP-ON","0");
      Serial.println("Scheduled ON");
    }
  }
}

