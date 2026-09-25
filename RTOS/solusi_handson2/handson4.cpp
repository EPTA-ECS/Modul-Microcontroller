#include <Arduino.h>

// ==========================================================
// PEMILIHAN CORE
// ==========================================================
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// ==========================================================
// PIN
// ==========================================================
static const int LED_YELLOW = 25;
static const int LED_RED = 26;
static const int LED_GREEN = 27;
static const int BUTTON_1 = 32;
static const int BUTTON_2 = 33;
static const int POT = 34;

// ==========================================================
// VARIABEL SISTEM
// ==========================================================
volatile bool systemEnabled = true;
volatile int commandCount = 0;

// ==========================================================
// TIMING
// ==========================================================
static const int YELLOW_ON_MS = 300;
static const int YELLOW_OFF_MS = 300;
static const int RED_ON_MS = 100;
static const int RED_OFF_MS = 100;
static const int POT_SAMPLE_MS = 20;
static const int BUTTON_SAMPLE_MS = 20;
static const int SERIAL_SAMPLE_MS = 500;

// ==========================================================
// TASK 1 — LED YELLOW
// ==========================================================
void taskYellow(void *parameter) {
  while (1) {
    if (systemEnabled) {
      digitalWrite( LED_YELLOW, HIGH );
      vTaskDelay( pdMS_TO_TICKS( YELLOW_ON_MS ) );
      digitalWrite( LED_YELLOW, LOW );
      vTaskDelay( pdMS_TO_TICKS( YELLOW_OFF_MS ) );
    } else {
      digitalWrite( LED_YELLOW, LOW );
      vTaskDelay( pdMS_TO_TICKS(20) );
    }
  }
}

// ==========================================================
// TASK 2 — LED RED
// ==========================================================
void taskRed(void *parameter) {
  while (1) {
    if (systemEnabled) {
      digitalWrite( LED_RED, HIGH );
      vTaskDelay( pdMS_TO_TICKS( RED_ON_MS ) );
      digitalWrite( LED_RED, LOW );
      vTaskDelay( pdMS_TO_TICKS( RED_OFF_MS ) );
    } else {
      digitalWrite( LED_RED, LOW );
      vTaskDelay( pdMS_TO_TICKS(20) );
    }
  }
}

// ==========================================================
// TASK 3 — POTENTIOMETER → LED GREEN
// ==========================================================
void taskGreen(void *parameter) {
  while (1) {
    if (systemEnabled) {
      int raw = analogRead(POT);
      int brightness = map( raw, 0, 4095, 0, 255 );
      analogWrite( LED_GREEN, brightness );
    } else {
      analogWrite( LED_GREEN, 0 );
    }
    vTaskDelay( pdMS_TO_TICKS( POT_SAMPLE_MS ) );
  }
}

// ==========================================================
// TASK 4 — BUTTON
// ==========================================================
void taskButton(void *parameter) {
  bool lastButton1 = HIGH;
  bool lastButton2 = HIGH;

  while (1) {
    bool currentButton1 = digitalRead(BUTTON_1);
    bool currentButton2 = digitalRead(BUTTON_2);

// ========================================================
// BUTTON 1
// ========================================================
    if ( lastButton1 == HIGH && currentButton1 == LOW ) {
      systemEnabled = !systemEnabled;
      if (systemEnabled) {
        Serial.println( "SYSTEM ON" );
      } else {
        Serial.println( "SYSTEM OFF" );
      }
    }

// ========================================================
// BUTTON 2
// ========================================================
    if ( lastButton2 == HIGH && currentButton2 == LOW ) {
      commandCount++;
      Serial.println( "COMMAND RECEIVED!" );
      Serial.print( "Command count = " );
      Serial.println( commandCount );
    }

    lastButton1 = currentButton1;
    lastButton2 = currentButton2;

    vTaskDelay( pdMS_TO_TICKS( BUTTON_SAMPLE_MS ) );
  }
}

// ==========================================================
// TASK 5 — SERIAL
// ==========================================================
void taskSerial(void *parameter) {
  while (1) {
    Serial.println();
    Serial.println( "================================" );
    Serial.print( "System : " );
    if (systemEnabled) {
      Serial.println("ON");
    } else {
      Serial.println("OFF");
    }
    Serial.print( "Command count : " );
    Serial.println( commandCount );
    Serial.println( "================================" );
    vTaskDelay( pdMS_TO_TICKS( SERIAL_SAMPLE_MS ) );
  }
}

// ==========================================================
// SETUP
// ==========================================================
void setup() {
  Serial.begin(115200);

  pinMode( LED_YELLOW, OUTPUT );
  pinMode( LED_RED, OUTPUT );
  pinMode( LED_GREEN, OUTPUT );
  pinMode( BUTTON_1, INPUT_PULLUP );
  pinMode( BUTTON_2, INPUT_PULLUP );

  xTaskCreatePinnedToCore( taskYellow, "Task Yellow", 2048, NULL, 1, NULL, app_cpu );
  xTaskCreatePinnedToCore( taskRed, "Task Red", 2048, NULL, 1, NULL, app_cpu );
  xTaskCreatePinnedToCore( taskGreen, "Task Green", 2048, NULL, 1, NULL, app_cpu );
  xTaskCreatePinnedToCore( taskButton, "Task Button", 3072, NULL, 2, NULL, app_cpu );
  xTaskCreatePinnedToCore( taskSerial, "Task Serial", 3072, NULL, 1, NULL, app_cpu );
}

// ==========================================================
// LOOP
// ==========================================================
void loop() {
  vTaskDelay( pdMS_TO_TICKS(1000) );
}
