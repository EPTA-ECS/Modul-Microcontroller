/**
 * FreeRTOS Race Condition Demo — Fixed with Mutex
 */

#include "freertos/semphr.h"

// Global shared variable
static int shared_var = 0;

// Mutex to protect shared_var
static SemaphoreHandle_t mutex;

//***********************************************************
// Tasks

void incTask1(void *parameters) {
  int local_var;

  while (1) {
    xSemaphoreTake(mutex, portMAX_DELAY);   // LOCK: enter critical section

    // 1. Read the global variable
    local_var = shared_var;

    // 2. Modify it locally
    local_var++;

    delayMicroseconds(esp_random() % 500);
    // 4. Write it back — safe now, no one else can interrupt
    shared_var = local_var;

    Serial.print("Task 1: ");
    Serial.println(shared_var);

    xSemaphoreGive(mutex);                  // UNLOCK: leave critical section

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void incTask2(void *parameters) {
  int local_var;

  while (1) {
    xSemaphoreTake(mutex, portMAX_DELAY);   // LOCK

    local_var = shared_var;
    local_var++;

    delayMicroseconds(esp_random() % 500);

    shared_var = local_var;

    Serial.print("Task 2: ");
    Serial.println(shared_var);

    xSemaphoreGive(mutex);                  // UNLOCK

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

//***********************************************************
// Main

void setup() {
  Serial.begin(115200);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println("\n---FreeRTOS DUAL-CORE Race Condition Demo (Fixed)---");

  mutex = xSemaphoreCreateMutex();   // create the mutex BEFORE tasks start

  xTaskCreatePinnedToCore(incTask1, "Task 1", 1024, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(incTask2, "Task 2", 1024, NULL, 1, NULL, 1);

  vTaskDelete(NULL);
}

void loop() {
  // Empty
}
