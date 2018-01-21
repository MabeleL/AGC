//Chronic Care monitor IoT kit for diabetic patients
@Victoria Sogomo
@Leonard Mabele

#include <OneWire.h>
#include <DallasTemperature.h>
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal.h>
#include "Wire.h"
#include <stdio.h>
#include "Adafruit_FONA.h"
#include <Process.h>
#include <ArduinoJson.h>
#include "Adafruit_SleepyDog.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_FONA.h"
#include "Adafruit_MQTT_Client.h"
#define USE_ARDUINO_INTERRUPTS true    // Set-up low-level interrupts for most acurate BPM math.
#include <PulseSensorPlayground.h>     // Includes the PulseSensorPlayground Library.



/************************* WiFi Access Point *********************************/
#define FONA_APN       "safaricom"
#define FONA_USERNAME  ""
#define FONA_PASSWORD  ""

//FONA Configuration for ATmega328P
#define FONA_RX  2  // FONA serial RX pin (pin 2 for shield).
#define FONA_TX  3   // FONA serial TX pin (pin 3 for shield).
#define FONA_RST 4   // FONA reset pin (pin 4 for shield)
#define LED 13


SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);

Adafruit_FONA fona = Adafruit_FONA(FONA_RST);


/**************************Public Broker**************************/
#define AIO_SERVER      "broker.mqttdashboard.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    ""
#define AIO_KEY         ""
#define DEVICE_ID "Device_A"

/*****************Global State II*****************************/

// Setup the FONA MQTT class by passing in the FONA class and MQTT server and login details.
Adafruit_MQTT_FONA mqtt(&fona, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// You don't need to change anything below this line!
#define halt(s) { Serial.println(F( s )); while(1);  }

// FONAconnect is a helper function that sets up the FONA and connects to
// the GPRS network. See the fonahelper.cpp tab above for the source!
boolean FONAconnect(const __FlashStringHelper *apn, const __FlashStringHelper *username, const __FlashStringHelper *password);

/****************************** Feeds ***************************************/
// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish assetPub = Adafruit_MQTT_Publish(&mqtt, "feeds/" AIO_USERNAME "/ehealth/" DEVICE_ID);


const int rs = 12, en = 11, d4 = 10, d5 = 8, d6 = 7, d7 = 6;
LiquidCrystal lcd(12,11,10,8,7,6);

//  Variables
const int PulseWire = 0;       // PulseSensor PURPLE WIRE connected to ANALOG PIN 0
const int LED13 = 13;          // The on-board Arduino LED, close to PIN 13.
int Threshold = 550;

PulseSensorPlayground pulseSensor;

// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 9
// Setup a oneWire instance to communicate with any OneWire devices
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);


void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
  }
  Serial.println("MQTT Connected!");
}


void setup()
{

  while (!Serial);
  // Watchdog is optional!
  //Watchdog.enable(8000);
  Serial.begin(115200);
  Serial.println(F("Adafruit FONA MQTT demo"));
  // start serial port
  // Watchdog is optional!
  //Watchdog.enable(8000);
  // Enable GPS.
  fona.enableGPS(true);

  // Initial GPS read
  bool gpsFix = fona.getGPS(&latitude, &longitude);
  // start serial port
  Serial.println("Temperature Data");

  // Configure the PulseSensor object, by assigning our variables to it.
  pulseSensor.analogInput(PulseWire);
  pulseSensor.blinkOnPulse(LED13);       //auto-magically blink Arduino's LED with heartbeat.
  pulseSensor.setThreshold(Threshold);

  // Double-check the "pulseSensor" object was created and "began" seeing a signal.
   if (pulseSensor.begin()) {
    Serial.println("We created a pulseSensor Object !");  //This prints one time at Arduino power-up,  or on Arduino reset.
  // Start up the library
      // Initialise the FONA module
      while (! FONAconnect(F(FONA_APN), F(FONA_USERNAME), F(FONA_PASSWORD))) {
        Serial.println("Retrying FONA");
      }

      Serial.println(F("Connected to Cellular!"));

    // Start up the library
  sensors.begin();
  lcd.begin(16, 2);

  Watchdog.reset();
  delay(5000);  // wait a few seconds to stabilize connection
  Watchdog.reset();
}

void loop()
{
  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  Serial.print(" Waiting...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");

  Serial.print("Temperature: ");
  float bodyTemp = sensors.getTempCByIndex(0);
  Serial.print(bodyTemp); // Why "byIndex"?
    // You can have more than one IC on the same bus.
    // 0 refers to the first IC on the wire
    delay(20);

    int myBPM = pulseSensor.getBeatsPerMinute();  // Calls function on our pulseSensor object that returns BPM as an "int".
                                                  // "myBPM" hold this BPM value now.

   if (pulseSensor.sawStartOfBeat()) {            // Constantly test to see if "a beat happened".
    Serial.println("♥  A HeartBeat Happened ! "); // If test is "true", print a message "a heartbeat happened".
    Serial.print("BPM: ");                        // Print phrase "BPM: "
    Serial.println(myBPM);                        // Print the value inside of myBPM.
   }

     delay(20);                    // considered best practice in a simple sketch.

   lcd.setCursor(0, 1);
   lcd.print(bodyTemp);
   lcd.setCursor(1,0);
   lcd.print(myBPM);

   // Watchdog reset at start of loop--make sure everything below takes less than 8 seconds in normal operation!
   Watchdog.reset();

   // Grab a GPS reading.
  float latitude, longitude;
  bool gpsFix = fona.getGPS(&latitude, &longitude);

   Serial.print("Latitude: ");
   Serial.println(latitude, 5);
   Serial.println("");

   Serial.print("Longitude: ");
   Serial.println(longitude, 5);
   Serial.println("");

   // Ensure the connection to the MQTT server is alive (this will make the first
 MQTT_connect();

   Watchdog.reset();

   StaticJsonBuffer<100> jsonBuffer;
   JsonObject& payload = jsonBuffer.createObject();
   payload["device_id"] = DEVICE_ID;
   payload["body_temperature"] = bodyTemp;
   payload["Heart Rate"] = myBPM;
   //payload["oil_level"] = distance_sonar;
   payload["latitude"] = latitude;
   payload["longitude"] = longitude;
   //payload["geo-distance"] = distance;

   String sPayload = "";
   payload.printTo(sPayload);
   const char* cPayload = &sPayload[0u];

   // Now we can publish stuff!
  Serial.print(F("\nPublishing "));
  Serial.print(cPayload);
  Serial.print("...");


   if(!assetPub.publish(cPayload)) {
   Serial.println(F("Failed"));
   } else {
   Serial.println(F("OK!"));
   }

   // ping the server to keep the mqtt connection alive
 if(! mqtt.ping()) {
   Console.println(F("MQTT Ping failed."));
 }
   Watchdog.reset();
   delay(2000);
}
