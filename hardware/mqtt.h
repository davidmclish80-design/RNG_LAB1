#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

extern WiFiClient espClient;
extern PubSubClient mqtt;

extern TaskHandle_t xMQTT_Connect;
extern TaskHandle_t xLOOPHandle;
extern TaskHandle_t xUpdateHandle;
extern TaskHandle_t xButtonCheckeHandle;

extern const char* mqtt_server;
extern uint16_t mqtt_port;
extern const char* subtopic[];
extern const char* pubtopic;

extern void callback(char* topic, byte* payload, unsigned int length);
extern bool publish(const char *topic, const char *payload);
extern void vUpdate( void * pvParameters );
extern void vButtonCheck( void * pvParameters );

void MQTT_Connect( void * pvParameters )  {
  configASSERT( ( ( uint32_t ) pvParameters ) == 1 );  

  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setCallback(callback);

  for( ;; ) {
    if (!mqtt.connected()) {
      String clientId = "ESP32Client-";
      clientId += String((uint32_t)ESP.getEfuseMac(), HEX);

      if (mqtt.connect(clientId.c_str())) {
        // subscribe to topics
        mqtt.subscribe(subtopic[0]);
        mqtt.subscribe(subtopic[1]);
      }
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

/* Function that creates a task. */
void MQTT_ConnectFunction( void ) {
  BaseType_t xReturned;

  xReturned = xTaskCreatePinnedToCore(
                MQTT_Connect,
                "MQTT CONNECT",
                4096,                     /* Stack size (Bytes in ESP32, words in Vanilla FreeRTOS) */
                ( void * ) 1,
                8,
                &xMQTT_Connect,
                1);

  if ( xReturned != pdPASS ) { 
    Serial.println(" UNABLE TO CREATE MQTT CONNECT TASK"); 
  }
}

void vLOOP( void * pvParameters )  {
  configASSERT( ( ( uint32_t ) pvParameters ) == 1 );

  for( ;; ) {
    if(mqtt.connected()){
      mqtt.loop();
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

/* Function that creates a task. */
void vLOOPFunction( void ) {
  BaseType_t xReturned;

  xReturned = xTaskCreatePinnedToCore(
                    vLOOP,
                    "LOOP",
                    8096,
                    ( void * ) 1,
                    10,
                    &xLOOPHandle,
                    1);

  if( xReturned != pdPASS ) {
     Serial.println("UNABLE TO CREATE LOOP TASK");
  }
}

void vUpdateFunction( void ) {
  BaseType_t xReturned;

  xReturned = xTaskCreatePinnedToCore(
                    vUpdate,
                    "UPDATE",
                    4096,
                    ( void * ) 1,
                    9,
                    &xUpdateHandle,
                    1);

  if( xReturned != pdPASS ) {
     Serial.println("UNABLE TO CREATE UPDATE TASK");
  }
}

void vButtonCheckFunction( void ) {
  BaseType_t xReturned;

  xReturned = xTaskCreatePinnedToCore(
                    vButtonCheck,
                    "BUTTON CHECK",
                    4096,
                    ( void * ) 1,
                    9,
                    &xButtonCheckeHandle,
                    1);

  if( xReturned != pdPASS ) {
     Serial.println("UNABLE TO CREATE BUTTON TASK");
  }
}

#endif
