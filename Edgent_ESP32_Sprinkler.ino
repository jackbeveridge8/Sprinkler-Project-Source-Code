#define BLYNK_TEMPLATE_ID "TMPLNTvQD_2P"
#define BLYNK_DEVICE_NAME "Sprinkler Project"

#define BLYNK_FIRMWARE_VERSION        "0.1.4"

#define BLYNK_PRINT Serial
//#define BLYNK_DEBUG

#define APP_DEBUG

#include "BlynkEdgent.h"
#include "DHT.h"

#define DHT_PIN 13
#define LED_PIN 23
#define VALVE_PIN 32
#define MOISTURE_SENSOR_PIN 33
#define FLOWRATE_PIN 35
#define FLOWRATE_CALIBRATION 0.0005f
#define MOISTURE_CALIBRATION_HIGH 3300.0f
#define MOISTURE_CALIBRATION_LOW 1500.0f

#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);

BlynkTimer timer; 

//-----Virtual Pins------
// V1 = Temperature
// V2 = Humidity
// V3 = Valve
// V4 = Moisture Sensor
// V5 = Flow Rate Sensor
//-----------------------
//---Virtual Pin Flags---
// V6 = Temperature High Flag
// V7 = Water High Flag
// V8 = Moisture Low Flag
// V9 = Sunset Flag
// V10 = Sprinkler On
//-----------------------

//variables
int Pulses = 0;
double Temperature = 0;
double Humidity = 0;
int Moisture = 0;
double flowRate = 0;
int sunset = 0;
int waterReset = 0;


// Flags
bool tempHighFlag = 0;
bool waterLevelFlag = 0;
bool moistureLevelLowFlag = 0;


BLYNK_WRITE(V0) //led for testing only, used to ensure app can control the microcontroller board.
{
  int pinValue = param.asInt(); 
  digitalWrite(LED_PIN,pinValue);
}

BLYNK_WRITE(V3) //valve
{
  int pinValue = param.asInt(); 
  digitalWrite(VALVE_PIN,pinValue);
}

BLYNK_WRITE(V9) //sunset
{
  sunset = param.asInt(); 
}

BLYNK_WRITE(V11) //reset water used
{
  waterReset = param.asInt(); 
}

void IncrementLiquid()
{
  Pulses++;
}

void sensorUpdateTimer()
{
  Temperature = dht.readTemperature();
  Humidity = dht.readHumidity();
  int MoistureRaw = analogRead(MOISTURE_SENSOR_PIN);
  flowRate = FLOWRATE_CALIBRATION * Pulses;

  //moisture calibration
  Moisture = 100*(MOISTURE_CALIBRATION_HIGH - MoistureRaw)/(MOISTURE_CALIBRATION_HIGH - MOISTURE_CALIBRATION_LOW);
  (Moisture > 100) ? Moisture = 100 : Moisture = Moisture;
  (Moisture < 0) ? Moisture = 0 : Moisture = Moisture;

  Blynk.virtualWrite(V1, Temperature);
  Blynk.virtualWrite(V2, Humidity);
  Blynk.virtualWrite(V4, Moisture);
  Blynk.virtualWrite(V5, flowRate);
}

// Setup all events in here to change a flag and update the notifcation monitor on the app
// if all events true then set the virtual sprinkler turn on flag to high which will trigger cloud automation.
void eventSetup()
{
  // temperature level too high
    if (Temperature > 20 && tempHighFlag == 0)
  {
    Blynk.logEvent("temperature_too_high", String("High Temperature Detected! Tº: ") + Temperature);
    tempHighFlag = 1;
    Blynk.virtualWrite(V6, 1);
  } else if (Temperature <= 20)
  {
    tempHighFlag = 0;
    Blynk.virtualWrite(V6, 0);
  }

  // moisture level too low
  if (Moisture < 40 && moistureLevelLowFlag == 0)
  {
    Blynk.logEvent("moisture_level_low", String("Moisture level is too low!: ") + Moisture);
    moistureLevelLowFlag = 1;
    Blynk.virtualWrite(V8, 1);
  }else if (Moisture >= 40)
  {
    moistureLevelLowFlag = 0;
    Blynk.virtualWrite(V8, 0);
  }
  
  // water limit reached
  if (flowRate > 5 && waterLevelFlag == 0)
  {
    Blynk.logEvent("water_limit_reached", String("Water Limit has been reached!:  ") + flowRate);
    waterLevelFlag = 1;
    Blynk.virtualWrite(V7, 1);
  }else if (flowRate <= 5)
  {
    waterLevelFlag = 0;
    Blynk.virtualWrite(V7, 0);
  }

  //if moisture level is low, 5 litres of water hasnt been used, and it is 1 hour before sunset -- Sprinkler will turn on.
  //temperature wont cause the automatic turn on as in winter the temperature will never reach above 32 but the plants still need watering.
  if (moistureLevelLowFlag == 1 && waterLevelFlag == 0 && sunset == 1)
  {
    Blynk.virtualWrite(V10, 1); //Automatic Sprinkler Turn On Flag set to 1
  } else if (moistureLevelLowFlag == 0 || waterLevelFlag == 1 || sunset == 0)
  {
    Blynk.virtualWrite(V10, 0); //Automatic Sprinkler Turn On Flag set to 1
  }

  //water used reset
  if (waterReset == 1){
    Pulses = 0;
  }


/*
  // humidity level too high
  if (humidity >  && tempHighFlag = 0)
  {
    Blynk.logEvent("temperature_too_high", String("High Temperature Detected! Tº: ") + V1);
    tempHighFlag = 1;
  }
*/

}

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  pinMode(VALVE_PIN, OUTPUT);
  pinMode(FLOWRATE_PIN, INPUT_PULLUP);

  Serial.begin(115200);
  delay(100);

  attachInterrupt(digitalPinToInterrupt(FLOWRATE_PIN), IncrementLiquid, CHANGE);

  dht.begin();

  BlynkEdgent.begin();

  // data will be sent every 10000ms which is every 10 second.
  timer.setInterval(10000L, sensorUpdateTimer); 
}

void loop() {
  BlynkEdgent.run();
  timer.run();
  eventSetup();
}


