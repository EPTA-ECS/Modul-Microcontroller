#define BUTTON 27
#define LED 25

void setup() {
     pinMode(BUTTON, INPUT_PULLUP);
     pinMode(LED, OUTPUT);

     Serial.begin(115200);
}

void loop() {
     int buttonState = digitalRead(BUTTON);

     Serial.println(buttonState);

     delay(100);
}
