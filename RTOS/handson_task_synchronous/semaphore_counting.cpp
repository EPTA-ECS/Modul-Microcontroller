#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

SemaphoreHandle_t parkinglot;

void carTask(void *pvParameter) {
  int carID = (int)pvParameter;

  for (;;) {
    Serial.print("Car ");
    Serial.print(carID);
    Serial.println(" arriving at parkinglot");

    // Try to take the resource
    xSemaphoreTake(parkinglot, portMAX_DELAY);

    Serial.print("Car ");
    Serial.print(carID);
    Serial.println(" Parked");

    // Car stays in the parking lot for 5 sec
    vTaskDelay(pdMS_TO_TICKS(5000));

    Serial.print("Car ");
    Serial.print(carID);
    Serial.println(" Leaving the Parking lot");

    // Release the parking lot
    xSemaphoreGive(parkinglot);

    // Wait for 3 secs before coming back
    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}

void setup() {
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(1000));  // give Serial time to connect

  parkinglot = xSemaphoreCreateCounting(3, 3);  // 3 parking slots, all free
  // Car Tasks
  xTaskCreatePinnedToCore(carTask, "Car1", 2048, (void *)1, 1, NULL, 1);
  xTaskCreatePinnedToCore(carTask, "Car2", 2048, (void *)2, 1, NULL, 1);
  xTaskCreatePinnedToCore(carTask, "Car3", 2048, (void *)3, 1, NULL, 1);
  xTaskCreatePinnedToCore(carTask, "Car4", 2048, (void *)4, 1, NULL, 1);
  xTaskCreatePinnedToCore(carTask, "Car5", 2048, (void *)5, 1, NULL, 1);
}

void loop() {
  // Empty — all work happens in the tasks
}
