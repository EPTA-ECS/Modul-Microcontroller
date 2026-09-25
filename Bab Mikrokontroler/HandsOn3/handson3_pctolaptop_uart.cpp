#define LED 25

void setup() {
     pinMode(LED, OUTPUT);
     Serial.begin(115200);

     Serial.println("Ketik 1 untuk ON");
     Serial.println("Ketik 0 untuk OFF");
}

void loop() {
     if (Serial.available()) {
          char data = Serial.read();

          if (data == '1') {
               digitalWrite(LED, HIGH);
               Serial.println("LED ON");
          } else if (data == '0') {
               digitalWrite(LED, LOW);
    Serial.println("LED OFF");
          }
     }
}
