/*
 * CAN (TWAI) NODE A  -  "Sensor node"   |  Arduino IDE
 * Board   : ESP32-DevKitC V4  -> Tools > Board > "ESP32 Dev Module"
 * Core    : esp32 by Espressif Systems (Boards Manager), v2.x or v3.x
 * Library : none needed - uses the TWAI driver built into the ESP32 core
 *
 * Role:
 *   - Every 100 ms -> send ID 0x100 : [counter(4B) | temp x10 (2B) | status(1B) | 0]
 *   - Receive 0x200 (ACK from B, echoes counter) -> print round-trip time
 *   - Receive 0x201 (heartbeat from B)
 *   - Watch bus errors, auto-recover from BUS-OFF
 *
 * Wiring: GPIO21 -> transceiver TX(CTX), GPIO22 -> RX(CRX), 3V3, GND
 *         CANH-CANH, CANL-CANL, common GND, 120 ohm at each bus end
 */

#include "driver/twai.h"

#define CAN_TX_PIN    GPIO_NUM_21
#define CAN_RX_PIN    GPIO_NUM_22

#define ID_SENSOR     0x100   // A -> B
#define ID_ACK        0x200   // B -> A
#define ID_HEARTBEAT  0x201   // B -> all

#define TX_PERIOD_MS  100

uint32_t counter      = 0;
uint32_t lastTxMs     = 0;
uint32_t lastTxCount  = 0;
uint32_t lastTxMicros = 0;
bool     canRunning   = false;

// ---------------------------------------------------------------
void setupCAN() {
  twai_general_config_t g_config =
      TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
  g_config.tx_queue_len = 10;
  g_config.rx_queue_len = 20;
  g_config.alerts_enabled = TWAI_ALERT_ERR_PASS | TWAI_ALERT_BUS_ERROR |
                            TWAI_ALERT_RX_QUEUE_FULL | TWAI_ALERT_BUS_OFF |
                            TWAI_ALERT_BUS_RECOVERED;

  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();

  // Hardware filter: accept only 0x200 and 0x201 (ignore ID bit 0).
  // Standard ID lives in bits [31:21]; mask bit 1 = "don't care".
  twai_filter_config_t f_config;
  f_config.acceptance_code = (uint32_t)ID_ACK << 21;
  f_config.acceptance_mask = ~((uint32_t)0x7FE << 21);
  f_config.single_filter   = true;

  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
    Serial.println("Driver install FAILED");
    while (1) delay(1000);
  }
  if (twai_start() != ESP_OK) {
    Serial.println("Driver start FAILED");
    while (1) delay(1000);
  }
  canRunning = true;
  Serial.println("TWAI started @ 500 kbit/s");
}

// ---------------------------------------------------------------
void sendSensorFrame() {
  int16_t tempX10 = 250 + (counter % 100);   // fake 25.0 .. 34.9 C

  twai_message_t msg = {};
  msg.identifier       = ID_SENSOR;
  msg.extd             = 0;                  // 11-bit standard ID
  msg.data_length_code = 8;
  msg.data[0] = counter & 0xFF;              // little-endian packing
  msg.data[1] = (counter >> 8) & 0xFF;
  msg.data[2] = (counter >> 16) & 0xFF;
  msg.data[3] = (counter >> 24) & 0xFF;
  msg.data[4] = tempX10 & 0xFF;
  msg.data[5] = (tempX10 >> 8) & 0xFF;
  msg.data[6] = 0x01;                        // status OK
  msg.data[7] = 0x00;

  lastTxCount  = counter;
  lastTxMicros = micros();

  if (twai_transmit(&msg, pdMS_TO_TICKS(10)) == ESP_OK) {
    Serial.printf("TX 0x%03X cnt=%lu temp=%.1fC\n", ID_SENSOR,
                  (unsigned long)counter, tempX10 / 10.0f);
  } else {
    Serial.println("TX failed (no ACK / queue full / bus-off)");
  }
  counter++;
}

// ---------------------------------------------------------------
void handleRx() {
  twai_message_t rx;
  // Non-blocking: drain everything that is waiting
  while (twai_receive(&rx, 0) == ESP_OK) {
    if (rx.identifier == ID_ACK) {
      uint32_t echo = rx.data[0] | (rx.data[1] << 8) |
                      (rx.data[2] << 16) | ((uint32_t)rx.data[3] << 24);
      if (echo == lastTxCount) {
        Serial.printf("RX ACK cnt=%lu  RTT=%lu us\n",
                      (unsigned long)echo, (unsigned long)(micros() - lastTxMicros));
      } else {
        Serial.printf("RX ACK cnt=%lu (expected %lu)\n",
                      (unsigned long)echo, (unsigned long)lastTxCount);
      }
    } else if (rx.identifier == ID_HEARTBEAT) {
      uint16_t uptime = rx.data[0] | (rx.data[1] << 8);
      Serial.printf("RX HEARTBEAT B uptime=%u s, sees A=%s\n",
                    uptime, rx.data[2] ? "OK" : "LOST");
    }
  }
}

// ---------------------------------------------------------------
void handleAlerts() {
  uint32_t alerts;
  if (twai_read_alerts(&alerts, 0) != ESP_OK) return;

  if (alerts & TWAI_ALERT_ERR_PASS)       Serial.println("! Error PASSIVE (wiring/termination/other node?)");
  if (alerts & TWAI_ALERT_BUS_ERROR)      Serial.println("! Bus error");
  if (alerts & TWAI_ALERT_RX_QUEUE_FULL)  Serial.println("! RX queue full - frames lost");
  if (alerts & TWAI_ALERT_BUS_OFF) {
    Serial.println("!! BUS-OFF -> recovery");
    canRunning = false;
    twai_initiate_recovery();
  }
  if (alerts & TWAI_ALERT_BUS_RECOVERED) {
    Serial.println("Bus recovered -> restart");
    twai_start();
    canRunning = true;
  }
}

// ---------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== CAN NODE A ===");
  setupCAN();
  lastTxMs = millis();
}

void loop() {
  handleAlerts();
  handleRx();

  // Non-blocking periodic send (no delay() -> RX is never blocked)
  if (canRunning && millis() - lastTxMs >= TX_PERIOD_MS) {
    lastTxMs += TX_PERIOD_MS;
    sendSensorFrame();
  }
}
