// POLLING DEMO (NO INTERRUPT) - for comparison
// - LED1 (GPIO 2) blinks with normal delay() in loop()
// - Button (GPIO 18) toggles the RED LED (GPIO 4)
// The button is only CHECKED once per loop (every 2 s).
// Short presses during delay() are MISSED.

#define LED1        2     // blinking LED
#define RED_LED     4     // controlled by the button
#define BUTTON      18    // button between GPIO 18 and GND

bool redOn = false;
int lastButton = HIGH;    // button state from the previous check

void setup() {
  pinMode(LED1, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);       // not pressed = HIGH, pressed = LOW
}

void loop() {
  // 1) Check the button (only happens here, once per loop)
  int nowButton = digitalRead(BUTTON);
  if (lastButton == HIGH && nowButton == LOW) {   // just pressed
    redOn = !redOn;                               // toggle
    digitalWrite(RED_LED, redOn);
  }
  lastButton = nowButton;

  // 2) Blink LED1 - the button is NOT checked during these delays
  digitalWrite(LED1, HIGH);
  delay(1000);
  digitalWrite(LED1, LOW);
  delay(1000);
}
