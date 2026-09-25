#define POT 34
#define LED 25

void setup() {
     Serial.begin(115200);
}

void loop() {
     int analogValue = analogRead(POT);
     int brightness = map(analogValue, 0, 4095, 0, 255);

     analogWrite(LED, brightness);

     Serial.print("ADC = ");
     Serial.print(analogValue);

     Serial.print(" | Brightness = ");
     Serial.println(brightness);

     delay(20);
}
