//##################################################################################################################
//##                                      ELET2415 DATA ACQUISITION SYSTEM CODE                                   ##
//##                                                                                                              ##
//##################################################################################################################

// LIBRARY IMPORTS
// #include <rom/rtc.h>  // not needed; use "esp_sleep.h" only if you use sleep

#ifndef _WIFI_H 
#include <WiFi.h>
#endif

#ifndef STDLIB_H
#include <stdlib.h>
#endif

#ifndef STDIO_H
#include <stdio.h>
#endif

#ifndef ARDUINO_H
#include <Arduino.h>
#endif 
 
#ifndef ARDUINOJSON_H
#include <ArduinoJson.h>
#endif

#include <PubSubClient.h>


// DEFINE VARIABLES
#define ARDUINOJSON_USE_DOUBLE      1 
// DEFINE THE PINS THAT WILL BE MAPPED TO THE 7 SEG DISPLAY BELOW, 'a' to 'g'
#define a     15
#define b     12
#define c     14
#define d     27
#define e     26
#define f     25
#define g     33
#define dp    32
/* Complete all others */



// DEFINE VARIABLES FOR TWO LEDs AND TWO BUTTONs. LED_A, LED_B, BTN_A , BTN_B
#define LED_A 4
/* Complete all others */
#define LED_B 16
#define BTN_A 17


// MQTT CLIENT CONFIG  
static const char* pubtopic       = "620171852";                    // Add your ID number here
static const char* subtopic[]     = {"/elet2415", "620171852_sub"};  // Array of Topics(Strings) to subscribe to
static const char* mqtt_server    = "www.yanacreations.com";                // Broker IP address or Domain name as a String 
static uint16_t mqtt_port         = 1883;

// WIFI CREDENTIALS
const char* ssid                  = "gadfa’s iPhone"; // Add your Wi-Fi ssid
const char* password              = "12345678"; // Add your Wi-Fi password 

WiFiClient espClient;
PubSubClient mqtt(espClient);



// TASK HANDLES 
TaskHandle_t xMQTT_Connect          = NULL; 
TaskHandle_t xNTPHandle             = NULL;  
TaskHandle_t xLOOPHandle            = NULL;  
TaskHandle_t xUpdateHandle          = NULL;
TaskHandle_t xButtonCheckeHandle    = NULL; 

// FUNCTION DECLARATION   
void checkHEAP(const char* Name);   // RETURN REMAINING HEAP SIZE FOR A TASK
void initMQTT(void);                // CONFIG AND INITIALIZE MQTT PROTOCOL
unsigned long getTimeStamp(void);   // GET 10 DIGIT TIMESTAMP FOR CURRENT TIME
void callback(char* topic, byte* payload, unsigned int length);
void initialize(void);
bool publish(const char *topic, const char *payload); // PUBLISH MQTT MESSAGE(PAYLOAD) TO A TOPIC
void vButtonCheck( void * pvParameters );
void vUpdate( void * pvParameters ); 
void GDP(void);   // GENERATE DISPLAY PUBLISH

/* Declare your functions below */
void Display(unsigned char number);
int8_t getLEDStatus(int8_t LED);
void setLEDState(int8_t LED, int8_t state);
void toggleLED(int8_t LED);

void configurePins(void); // CONFIGURE ARDUINO PINS AS INPUT/OUTPUT
  

//############### IMPORT HEADER FILES ##################
#ifndef NTP_H
#include "NTP.h"
#endif

#ifndef MQTT_H
#include "mqtt.h"
#endif

// Temporary Variables
uint8_t number = 0;


void setup() {
  Serial.begin(115200); 
 
  delay(500);
  Serial.println("BOOT OK");

 // INIT SERIAL  

  // CONFIGURE THE ARDUINO PINS OF THE 7SEG AS OUTPUT
  pinMode(a,OUTPUT);
  /* Configure all others here */
  configurePins();

  setLEDState(LED_B, LOW);
  setLEDState(LED_A, LOW);
  initialize();           // INIT WIFI, MQTT & NTP 
  vButtonCheckFunction(); // UNCOMMENT IF USING BUTTONS THEN ADD LOGIC FOR INTERFACING WITH BUTTONS IN THE vButtonCheck FUNCTION

}
  


void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}





  
//####################################################################
//#                          UTIL FUNCTIONS                          #       
//####################################################################
void vButtonCheck(void * pvParameters)  {
  configASSERT(((uint32_t) pvParameters) == 1);

  bool last = HIGH;  // INPUT_PULLUP: not pressed = HIGH

  for (;;) {
    bool now = digitalRead(BTN_A);

    // run only once when button is first pressed
    if (last == HIGH && now == LOW) {

      toggleLED(LED_A);
      toggleLED(LED_B);

      StaticJsonDocument<256> doc;
      char message[256] = {0};

      doc["id"]        = "620171852";
      doc["timestamp"] = getTimeStamp();
      doc["ledA"]      = getLEDStatus(LED_A);
      doc["ledB"]      = getLEDStatus(LED_B);

      serializeJson(doc, message);

      publish(pubtopic, message);

      // debounce (ignore bounce for a short time)
      vTaskDelay(pdMS_TO_TICKS(50));
    }

    last = now;
    vTaskDelay(pdMS_TO_TICKS(10)); // check often
  }
}


void vUpdate( void * pvParameters )  {
    configASSERT( ( ( uint32_t ) pvParameters ) == 1 );    
           
    for( ;; ) {
          // Task code goes here.   
          // PUBLISH to topic every second.
          StaticJsonDocument<256> doc;
          char message[256]  = {0};

          // Add key:value pairs to JSon object
          doc["id"]         = "620171852";

          serializeJson(doc, message);  // Seralize / Covert JSon object to JSon string and store in char* array

          if(mqtt.connected() ){
            publish(pubtopic, message);
          }
          
            
        vTaskDelay(5000 / portTICK_PERIOD_MS);  
    }
}

unsigned long getTimeStamp(void) {
          // RETURNS 10 DIGIT TIMESTAMP REPRESENTING CURRENT TIME
          time_t now;         
          time(&now); // Retrieve time[Timestamp] from system and save to &now variable
          return now;
}

void callback(char* topic, byte* payload, unsigned int length) {
  // ############## MQTT CALLBACK  ######################################
  // RUNS WHENEVER A MESSAGE IS RECEIVED ON A TOPIC SUBSCRIBED TO

  Serial.printf("\nMessage received : ( topic: %s ) \n", topic);

  // Copy payload into a bounded C-string (no heap allocation, avoids leaks)
  static char received[512];
  if (length >= sizeof(received)) {
    Serial.println("Payload too large; dropped.");
    return;
  }
  memcpy(received, payload, length);
  received[length] = '\0';

  Serial.printf("Payload : %s \n", received);

  // CONVERT MESSAGE TO JSON
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, received);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  // PROCESS MESSAGE
  const char* type = doc["type"] | "";

  if (strcmp(type, "toggle") == 0) {
    // Process messages with ‘{"type": "toggle", "device": "LED A"}’ Schema
    const char* led = doc["device"] | "";

    if (strcmp(led, "LED A") == 0) {
      toggleLED(LED_A);
    }
    if (strcmp(led, "LED B") == 0) {
      toggleLED(LED_B);
    }

    // PUBLISH UPDATE BACK TO FRONTEND
    StaticJsonDocument<256> out;
    char message[256] = {0};

    out["id"]        = "620171852";
    out["timestamp"] = getTimeStamp();
    out["number"]    = number;
    out["ledA"]      = getLEDStatus(LED_A);
    out["ledB"]      = getLEDStatus(LED_B);

    serializeJson(out, message);
    publish(pubtopic, message);
  }
}

bool publish(const char *topic, const char *payload){   
     bool res = false;
     try{
        res = mqtt.publish(topic,payload);
        // Serial.printf("\nres : %d\n",res);
        if(!res){
          res = false;
          throw false;
        }
     }
     catch(...){
      Serial.printf("\nError (%d) >> Unable to publish message\n", res);
     }
  return res;
}

//***** Complete the util functions below ******
static inline void setSegments(bool A,bool B,bool C,bool D,bool E,bool F,bool G,bool DP=false);

void Display(unsigned char number)
{
   /* This function takes an integer between 0 and 9 as input. This integer must be written to the 7-Segment display */
  switch(number)
  {
    case 0: setSegments(1,1,1,1,1,1,0); break; // a b c d e f on
    case 1: setSegments(0,1,1,0,0,0,0); break; // b c
    case 2: setSegments(1,1,0,1,1,0,1); break; // a b d e g
    case 3: setSegments(1,1,1,1,0,0,1); break; // a b c d g
    case 4: setSegments(0,1,1,0,0,1,1); break; // b c f g
    case 5: setSegments(1,0,1,1,0,1,1); break; // a c d f g
    case 6: setSegments(1,0,1,1,1,1,1); break; // a c d e f g
    case 7: setSegments(1,1,1,0,0,0,0); break; // a b c
    case 8: setSegments(1,1,1,1,1,1,1); break; // all
    case 9: setSegments(1,1,1,1,0,1,1); break; // a b c d f g

    default:
      // blank display if number not 0-9
      setSegments(0,0,0,0,0,0,0,0);
      break;
  }
}

int8_t getLEDStatus(int8_t LED) {
  if (digitalRead(LED)==HIGH){return 1;} else {return 0;}
  // RETURNS THE STATE OF A SPECIFIC LED. 0 = LOW, 1 = HIGH  
}

void setLEDState(int8_t LED, int8_t state){
  digitalWrite(LED, state);
  // SETS THE STATE OF A SPECIFIC LED   
}

void toggleLED(int8_t LED){
  digitalWrite(LED, !getLEDStatus(LED));
  // TOGGLES THE STATE OF SPECIFIC LED   
}


void GDP(void){
  // GENERATE, DISPLAY THEN PUBLISH INTEGER

  number =  random(0,10);

  // DISPLAY integer on 7Seg.
  Display(number);

  // PUBLISH number to topic.
  StaticJsonDocument<256> doc;
  char message[256]  = {0};

  doc["id"]           = "620171852";
  doc["timestamp"]    = getTimeStamp();
  doc["number"]       = number;
  doc["ledA"]         = getLEDStatus(LED_A);
  doc["ledB"]         = getLEDStatus(LED_B);

  serializeJson(doc, message);
  publish(pubtopic, message);
}

void configurePins(void){
  // CONFIGURE ARDUINO PINS AS INPUT/OUTPUT
  pinMode(b,OUTPUT);
  pinMode(c,OUTPUT);
  pinMode(d,OUTPUT);
  pinMode(e,OUTPUT);
  pinMode(f,OUTPUT);
  pinMode(g,OUTPUT);
  pinMode(dp,OUTPUT);
  pinMode(LED_A,OUTPUT);
  pinMode(LED_B,OUTPUT);
  pinMode(BTN_A,INPUT_PULLUP);
}

//CONSTANTS FOR SEVEN SEG BELOW
// Set this depending on your 7-seg type:
const uint8_t SEG_ON  = HIGH;  // common cathode
const uint8_t SEG_OFF = LOW;

// If your display is common ANODE, swap them:
// const uint8_t SEG_ON  = LOW;
// const uint8_t SEG_OFF = HIGH;

// FIX: no default value here (DP has no "=false" here)
static inline void setSegments(bool A,bool B,bool C,bool D,bool E,bool F,bool G,bool DP)
{
  digitalWrite(a,  A ? SEG_ON : SEG_OFF);
  digitalWrite(b,  B ? SEG_ON : SEG_OFF);
  digitalWrite(c,  C ? SEG_ON : SEG_OFF);
  digitalWrite(d,  D ? SEG_ON : SEG_OFF);
  digitalWrite(e,  E ? SEG_ON : SEG_OFF);
  digitalWrite(f,  F ? SEG_ON : SEG_OFF);
  digitalWrite(g,  G ? SEG_ON : SEG_OFF);
  digitalWrite(dp, DP ? SEG_ON : SEG_OFF);
}

void initialize(void) {
  // ---- WiFi connect ----
  Serial.println("Init: WiFi begin...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(250);
    // optional timeout so you don’t hang forever
    if (millis() - start > 20000) {
      Serial.println("\nWiFi connect timeout (check ssid/password).");
      break;
    }
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected. IP: ");
    Serial.println(WiFi.localIP());
  }
  Serial.print("Init: WiFi connected, IP=");
  Serial.println(WiFi.localIP());


  // ---- Start tasks ----
  Serial.println("Init: starting NTP + MQTT tasks...");
  // NTP task (stack fixed in NTP.h)
  vNTPFunction();

  // MQTT tasks
  MQTT_ConnectFunction();   // handles connect + subscribe
  vLOOPFunction();          // calls mqtt.loop()
  vUpdateFunction();        // your periodic publish task (optional)
}

