/**
 * FreeRTOS Race Condition Demo
 */

// Global shared variable
static int shared_var = 0;

//*****************************************************************************
// Tasks

void incTask1(void *parameters) {
  int local_var;

  while (1) {
    // 1. Read the global variable
    local_var = shared_var;
   
    // 2. Modify it locally
    local_var++;

    delayMicroseconds(esp_random() % 500);

    // 4. Write it back (Overwriting whatever Task 2 just did)
    shared_var = local_var;

    Serial.print("Task 1 wrote: ");
    Serial.println(shared_var);

    // Tiny delay to keep the Serial Monitor from totally crashing
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void incTask2(void *parameters) {
  int local_var;
  while (1) {
    local_var = shared_var;
    local_var++;

    delayMicroseconds(esp_random() % 500);

    shared_var = local_var;

    Serial.print("Task 2 wrote: ");
    Serial.println(shared_var);

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

//***********************************************************
// Main

void setup() {
  Serial.begin(115200);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println("\n---FreeRTOS DUAL-CORE Race Condition Demo---");

  // '0' for Core 0, and '1' for Core 1.
  xTaskCreatePinnedToCore(incTask1, "Task 1", 1024, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(incTask2, "Task 2", 1024, NULL, 1, NULL, 1);

  vTaskDelete(NULL);
}

void loop() {
  // Empty
}
