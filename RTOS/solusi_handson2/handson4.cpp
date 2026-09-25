#define LED_YELLOW 25
#define LED_RED 26
#define LED_GREEN 32
#define BUTTON_1 27
#define BUTTON_2 33
#define POT 34

// =====================================================
// STATUS SISTEM
// =====================================================
bool systemEnabled = true;
int commandCount = 0;

// =====================================================
// VARIABEL TIMING
// =====================================================
const int yellowOnTime = 300;
const int yellowOffTime = 300;
const int redOnTime = 100;
const int redOffTime = 100;

// =====================================================
// SETUP
// =====================================================
void setup() {
pinMode(LED_YELLOW, OUTPUT);
pinMode(LED_RED, OUTPUT);
pinMode(LED_GREEN, OUTPUT);
pinMode(BUTTON_1, INPUT_PULLUP);
pinMode(BUTTON_2, INPUT_PULLUP);
Serial.begin(115200);
Serial.println("Mini Control Panel");
Serial.println("System started.");
}
// =====================================================
// LOOP
// =====================================================
void loop() {
// ===================================================
// 1. LED YELLOW
// ===================================================
if (systemEnabled) {
    digitalWrite( LED_YELLOW, HIGH );
    delay(yellowOnTime);
    digitalWrite( LED_YELLOW, LOW );
    delay(yellowOffTime);
}

// ===================================================
// 2. LED RED
// ===================================================
  if (systemEnabled) {
    digitalWrite( LED_RED, HIGH );
    delay(redOnTime);
    digitalWrite( LED_RED, LOW );
    delay(redOffTime);
  }

// ===================================================
// 3. BUTTON 1
// System ON/OFF
// ===================================================
  if (digitalRead(BUTTON_1) == LOW) {
    systemEnabled = !systemEnabled;
    if (systemEnabled) {
      Serial.println( "SYSTEM ON" );
    } else {
      Serial.println( "SYSTEM OFF" );
      digitalWrite( LED_YELLOW, LOW );
      digitalWrite( LED_RED, LOW );
      analogWrite( LED_GREEN, 0 );
    }
    delay(250);
  }
// ===================================================
// 4. BUTTON 2
// Trigger Event / Command
// ===================================================
  if (digitalRead(BUTTON_2) == LOW) {
    commandCount++;
    Serial.println( "COMMAND RECEIVED!" );
    Serial.print( "Command count = " );
    Serial.println( commandCount );
    delay(250);
  }

// ===================================================
// 5. POTENTIOMETER → LED GREEN
// ===================================================
  if (systemEnabled) {
    int analogValue = analogRead(POT);
    int brightness = map( analogValue, 0, 4095, 0, 255 );
    analogWrite( LED_GREEN, brightness );
    delay(20);
  }
}
