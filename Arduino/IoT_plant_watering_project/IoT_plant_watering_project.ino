#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

//---------RTC--------
#include <Wire.h>
#include <ds3231.h>
struct ts t; 
// ------DHT11 Sensor---------
#include <SimpleDHT.h>        
int pinDHT11 = 0; // D3
SimpleDHT11 dht11(pinDHT11);
byte dhtHumid = 0;  //Stores humidity value
byte dhtTemp = 0; //Stores temperature value

//-------pump Relay Pin-------
int pump = 2; // D4
//--------Grow Light pin--------
int growLight = 14; // D5
//--------Capacitive Soil Moisture sensor------------
const int dry = 595; // value for dry sensor
const int wet = 239; // value for wet sensor

// ---------Config---------
#define WLAN_SSID       "OQFibre-C53JJLQ"   // Wi-Fi name 
#define WLAN_PASS       "48971499"    // Wi-Fi password
// Adafruit server
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "Essaalansari"                       
#define AIO_KEY         "aio_qadg37HFD3L0Y9ZKo8Ss7MvNqGr3"   

// ---------WifiClient Class for connecting to WiFi and establishing connection to MQTT broker ---------------
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Topics
Adafruit_MQTT_Publish SM = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/SM");     // SM = Soil Moisture Topic    
Adafruit_MQTT_Publish temp = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temp");            // Temperature Topic
Adafruit_MQTT_Publish humidity = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/humidity");   // Humidity Topic
Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/GL"); // Grow light Manual On Off topic 

void MQTT_connect();

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println(F("Essa IoT Project"));
  Wire.begin();
  DS3231_init(DS3231_CONTROL_INTCN); // RTC

  pinMode(pump, OUTPUT);
  pinMode(growLight, OUTPUT);
  
  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");

  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {  // keep printing "." until wifi is connected
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi is connected");
  Serial.println("IP address: "); 
  Serial.println(WiFi.localIP());  // Print IP Local address on serial monitor for debugging purposes

  // Subscribe to pump Topic  
  mqtt.subscribe(&onoffbutton);
}

uint32_t x=0;

void loop() {
  //---------RTC------
  DS3231_get(&t);
  int Time = t.hour;
  Serial.println(Time);
  if (Time == 4){
    digitalWrite(growLight, HIGH);
  }
  
  //------Read soil moisture
  int sensorVal = analogRead(A0); // pin A0 of nodemcu
  Serial.println(sensorVal);
  int SoilMoisture = map(sensorVal, wet, dry, 100, 0); // Converting the Raw analog read values to Percentage (%)
  Serial.print("Soil Moisture = ");
  Serial.print(SoilMoisture);
  Serial.println("%");
  if (SoilMoisture < 50){
    digitalWrite(pump, HIGH); // if soil moisture is below 50% turn the water on
  } else if (SoilMoisture > 70){
    digitalWrite(pump, LOW); // Turn of the Water if soil moisture level is above 70%
  }
  
  //-----Read data from dht11 sensor and print on Serial monitor----------
  dht11.read(&dhtTemp, &dhtHumid, NULL);
  Serial.print((int)dhtTemp); Serial.print(" *C, "); 
  Serial.print((int)dhtHumid); Serial.println(" H");

  // Ensures that the NodeMCU is Connected to the Broker
  MQTT_connect();

  // Reading State of the button on GL topic
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &onoffbutton) {
      Serial.print(F("Turning Grow light "));
      String buttonState = (char *)onoffbutton.lastread;
      Serial.print(buttonState);
      if (buttonState == "ON"){
        digitalWrite(growLight, HIGH); // Grow light ON
        Serial.println("OK ON");
      }
      if (buttonState == "OFF"){
        digitalWrite(growLight, LOW); // Grow light OFF 
        Serial.println("OK OFF");
      }
    }
  }

  // Now we can publish Temperature, Humidity and Soil Moisture!
  Serial.print(F("\nSending Temperature and Humidity Data"));
  Serial.print("...");
  if (! temp.publish(dhtTemp)) {  // Publish temperature data to Adafruit Broker
      Serial.println(F("Failed to Publish Temperature"));
    } 
       if (! humidity.publish(dhtHumid)) {   // Publish humidity data to Adafruit Broker
      Serial.println(F("Failed to Publish Humidity"));
    }
      if (! SM.publish(SoilMoisture)) {   // Publish humidity data to Adafruit Broker
      Serial.println(F("Failed to Publish Soil Moisture"));
    }
    else {
      Serial.println(F("Published!"));
    }
delay(10000);
}

void MQTT_connect() {
  int8_t ret;
  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
