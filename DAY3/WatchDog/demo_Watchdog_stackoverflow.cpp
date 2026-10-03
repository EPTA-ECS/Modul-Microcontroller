// STACK OVERFLOW DEMO 2 (BUG): "functions that call the next step"
// Goal: blink LED1 and LED2 one after the other, forever.
// Mistake: each step CALLS the next step instead of RETURNING.
// No function ever finishes, so the stack grows every step. After ~10 s: CRASH.

#include "esp_system.h"

#define LED1 2
#define LED2 4

int step = 0;

void ledB();   // tell the compiler ledB exists (it is written below)

void ledA() {
  char msg[40];                              // local text buffer (on the stack)
  step++;
  snprintf(msg, sizeof(msg), "step %d: LED1 on", step);
  Serial.println(msg);

  digitalWrite(LED1, HIGH);
  digitalWrite(LED2, LOW);
  delay(100);

  ledB();   // <-- BUG: go to the next step by CALLING it (ledA never ends)
}

void ledB() {
  char msg[40];
  step++;
  snprintf(msg, sizeof(msg), "step %d: LED2 on", step);
  Serial.println(msg);

  digitalWrite(LED1, LOW);
  digitalWrite(LED2, HIGH);
  delay(100);

  ledA();   // <-- BUG: ledB never ends either
}

void setup() {
  enableLoopWDT();   // reset the ESP32 if loop() doesn't finish within ~5 s
  Serial.begin(115200);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  // After a crash the ESP32 reboots. If so, freeze here until EN is pressed.
  if (esp_reset_reason() == ESP_RST_PANIC) {
    while (true) delay(1000);
  }
}

void loop() {
  ledA();   // starts the chain... and never comes back here
}
