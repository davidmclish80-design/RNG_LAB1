#ifndef NTP_H
#define NTP_H

#include <WiFi.h>
#include <time.h>
#include <Arduino.h>

// ---------------------- NTP CLASS ----------------------
class Ntp {
  public:
    Ntp(const char* msg){
      // optional message
      (void)msg;
    }

    void setup() {
      // NTP configuration
      configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    }

    void printLocalTime() {
      struct tm timeinfo;
      if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
      }
      Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    }

    time_t getTime() {
      time_t now;
      time(&now);
      return now;
    }
};


// ---------------------- TASK ----------------------
extern TaskHandle_t xNTPHandle;

void vNTP( void * pvParameters ) {
  configASSERT( ( ( uint32_t ) pvParameters ) == 1 );   

  // Make this static so it doesn't eat task stack
  static Ntp NTP = Ntp("NTP PROTOCOL INITIATED"); // Instantiate once (static reduces stack use)
  NTP.setup();

  for( ;; ) {  
      vTaskDelay(60000 / portTICK_PERIOD_MS);  
  }     
}

// Function that creates a task.
void vNTPFunction( void ) {
    BaseType_t xReturned;

    xReturned = xTaskCreatePinnedToCore(
                    vNTP,
                    "NTP Protocol",
                    8192,               // Stack size (Bytes in ESP32, words in Vanilla FreeRTOS)
                    ( void * ) 1,
                    12,
                    &xNTPHandle,
                    1);

    if( xReturned != pdPASS ) {
       Serial.println("UNABLE TO CREATE NTP PROTOCOL TASK");
    }
}

#endif
