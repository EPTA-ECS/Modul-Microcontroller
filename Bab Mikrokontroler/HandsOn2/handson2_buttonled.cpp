#define BUTTON 27
#define LED 25

void setup() {
     pinMode(BUTTON, INPUT_PULLUP);
     pinMode(LED, OUTPUT);
}

void loop() {
     int buttonState = digitalRead(BUTTON);

     if (buttonState == LOW) {
          digitalWrite(LED, HIGH);
     } else {
          digitalWrite(LED, LOW);
     }
}
