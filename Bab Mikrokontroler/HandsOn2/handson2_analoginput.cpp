#define POT 34

void setup() {
     Serial.begin(115200);
}

void loop() {
     int analogValue = analogRead(POT);
     int voltage = analogReadMilliVolts(POT);

     Serial.print("ADC = ");
     Serial.print(analogValue);

     Serial.print(" | Voltage = ");
     Serial.print(voltage);

     Serial.println(" mV");

     delay(500);
} 