// INTERRUPT DEMO
// - LED1 (GPIO 2) blinks with normal delay() in loop()
// - Button (GPIO 18) toggles the RED LED (GPIO 4) using an INTERRUPT
// Even while loop() is stuck inside delay(1000), the button reacts at once.

#define LED1        2     // blinking LED
#define RED_LED     4     // controlled by the button
#define BUTTON      18    // button between GPIO 18 and GND

volatile bool redOn = false;           // shared with the interrupt -> volatile
volatile unsigned long lastPress = 0;  // for debounce

// Interrupt Service Routine (ISR): runs the moment the button is pressed
void IRAM_ATTR onButton() {
  unsigned long now = millis();
  if (now - lastPress < 200) return;   // ignore bounce (extra pulses < 200 ms)
  lastPress = now;

  redOn = !redOn;                      // toggle: ON -> OFF -> ON ...
  digitalWrite(RED_LED, redOn);
}

void setup() {
  pinMode(LED1, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);       // not pressed = HIGH, pressed = LOW

  // call onButton() when the pin goes HIGH -> LOW (button pressed)
  attachInterrupt(digitalPinToInterrupt(BUTTON), onButton, FALLING);
}

void loop() {
  digitalWrite(LED1, HIGH);
  delay(1000);                         // CPU is "busy waiting" here...
  digitalWrite(LED1, LOW);
  delay(1000);                         // ...but the button still works instantly
}
