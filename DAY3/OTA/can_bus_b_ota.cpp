/*
  CAN NODE B (responder node) + OTA  |  ESP32-DevKitC V4, Arduino IDE

  Isi:
    - OtaWeb.h  : OTA + serial monitor web (sama di Node A dan B)
    - file ini  : konfigurasi node + kode CAN + perintah

  Perilaku CAN:
    - terima 0x100 dari A, langsung balas 0x200 (echo counter)
    - tiap 500 ms kirim heartbeat 0x201 [uptime | A hidup?]
    - deteksi A timeout (> 300 ms tanpa frame)
    - ringkasan log 1x per detik

  Tools > Partition Scheme harus punya OTA, mis. "Default 4MB with spiffs".
*/

// ============================================================
// 1. KONFIGURASI NODE
// ============================================================
const char* WIFI_SSID  = "T 0 K E K";
const char* WIFI_PASS  = "Tokek333221";
const char* NODE_ID    = "can-node-b";     // HARUS beda dengan Node A -> can-node-b.local
const char* FW_VERSION = "1.0.0";
const char* AUTH_USER  = "admin";
const char* AUTH_PASS  = "admin"; // WAJIB diganti
const int   LED_PIN    = 2;

#include "OtaWeb.h"
#include "driver/twai.h"

// ============================================================
// 2. CAN
// ============================================================
#define CAN_TX_PIN         GPIO_NUM_21
#define CAN_RX_PIN         GPIO_NUM_22
#define ID_SENSOR          0x100
#define ID_ACK             0x200
#define ID_HEARTBEAT       0x201
#define HB_PERIOD_MS       500
#define NODE_A_TIMEOUT_MS  300

bool     canRunning = false;
bool     otaActive  = false;
bool     canVerbose = false;

uint32_t lastRxMs  = 0;
bool     everRx    = false;
uint16_t rxOkCount = 0;
uint32_t lastHbMs  = 0;
bool     aWasAlive = false;

uint32_t statRx = 0, statAckFail = 0;
uint32_t lastSummaryMs = 0;
float    lastTemp = 0;

bool setupCAN() {
  twai_general_config_t g_config =
      TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
  g_config.tx_queue_len = 10;
  g_config.rx_queue_len = 20;
  g_config.alerts_enabled = TWAI_ALERT_ERR_PASS | TWAI_ALERT_BUS_ERROR |
                            TWAI_ALERT_RX_QUEUE_FULL | TWAI_ALERT_BUS_OFF |
                            TWAI_ALERT_BUS_RECOVERED;

  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();

  twai_filter_config_t f_config;                       // terima 0x100 saja
  f_config.acceptance_code = (uint32_t)ID_SENSOR << 21;
  f_config.acceptance_mask = ~((uint32_t)0x7FF << 21);
  f_config.single_filter   = true;

  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
    logf("CAN: driver install GAGAL");
    return false;
  }
  if (twai_start() != ESP_OK) {
    logf("CAN: start GAGAL");
    return false;
  }
  canRunning = true;
  logf("CAN: jalan @ 500 kbit/s (TX=21, RX=22)");
  return true;
}

void handleCanRx() {
  twai_message_t rx;
  while (twai_receive(&rx, 0) == ESP_OK) {
    if (rx.identifier != ID_SENSOR || rx.data_length_code < 7) continue;

    lastRxMs = millis();
    everRx   = true;
    rxOkCount++;
    statRx++;

    uint32_t cnt = rx.data[0] | (rx.data[1] << 8) |
                   (rx.data[2] << 16) | ((uint32_t)rx.data[3] << 24);
    int16_t tempX10 = (int16_t)(rx.data[4] | (rx.data[5] << 8));
    lastTemp = tempX10 / 10.0f;

    if (canVerbose) logf("RX 0x100 cnt=%lu temp=%.1fC", (unsigned long)cnt, lastTemp);

    twai_message_t ack = {};
    ack.identifier       = ID_ACK;
    ack.data_length_code = 8;
    memcpy(&ack.data[0], &rx.data[0], 4);
    ack.data[4] = rxOkCount & 0xFF;
    ack.data[5] = (rxOkCount >> 8) & 0xFF;

    if (twai_transmit(&ack, 0) != ESP_OK) statAckFail++;
  }
}

void sendHeartbeat() {
  bool aAlive = everRx && (millis() - lastRxMs < NODE_A_TIMEOUT_MS);

  // log hanya saat status BERUBAH, bukan tiap 500 ms
  if (aAlive != aWasAlive) {
    logf(aAlive ? "Node A ONLINE" : "! Node A TIMEOUT");
    aWasAlive = aAlive;
  }

  uint16_t uptime = millis() / 1000;
  twai_message_t hb = {};
  hb.identifier       = ID_HEARTBEAT;
  hb.data_length_code = 3;
  hb.data[0] = uptime & 0xFF;
  hb.data[1] = (uptime >> 8) & 0xFF;
  hb.data[2] = aAlive ? 1 : 0;
  twai_transmit(&hb, 0);
}

void handleCanAlerts() {
  uint32_t alerts;
  if (twai_read_alerts(&alerts, 0) != ESP_OK) return;

  if (alerts & TWAI_ALERT_ERR_PASS)      logf("CAN ! error PASSIVE");
  if (alerts & TWAI_ALERT_BUS_ERROR)     logf("CAN ! bus error");
  if (alerts & TWAI_ALERT_RX_QUEUE_FULL) logf("CAN ! RX queue penuh");
  if (alerts & TWAI_ALERT_BUS_OFF) {
    logf("CAN !! BUS-OFF -> recovery");
    canRunning = false;
    twai_initiate_recovery();
  }
  if (alerts & TWAI_ALERT_BUS_RECOVERED && !otaActive) {
    logf("CAN: pulih, start lagi");
    twai_start();
    canRunning = true;
  }
}

void printCanSummary() {
  logf("CAN 1s: rx=%lu ack_fail=%lu temp=%.1fC | A=%s",
       (unsigned long)statRx, (unsigned long)statAckFail, lastTemp,
       aWasAlive ? "OK" : "LOST");
  statRx = statAckFail = 0;
}

// ============================================================
// 3. HOOK OTA
// ============================================================
void onOtaStart() {
  otaActive = true;
  if (canRunning) {
    twai_stop();
    canRunning = false;
  }
  logf("CAN: dihentikan selama OTA");
}

void onOtaFailed() {
  otaActive = false;
  if (!canRunning && twai_start() == ESP_OK) {
    canRunning = true;
    lastHbMs = millis();
    logf("CAN: jalan lagi (OTA gagal)");
  }
}

// ============================================================
// 4. PERINTAH
// ============================================================
void handleCmd(String cmd) {
  cmd.trim();
  if (cmd == "") return;
  logf("> %s", cmd.c_str());

  if (cmd == "help") {
    logf("perintah: help, ver, status, led on, led off, reboot,");
    logf("          can, can verbose on, can verbose off, can stop, can start");
  } else if (cmd == "ver") {
    logf("%s firmware %s", NODE_ID, FW_VERSION);
  } else if (cmd == "status") {
    logf("uptime %lu detik, heap bebas %u byte, RSSI %d dBm",
         millis() / 1000, (unsigned)ESP.getFreeHeap(), WiFi.RSSI());
  } else if (cmd == "led on") {
    digitalWrite(LED_PIN, HIGH); logf("LED menyala");
  } else if (cmd == "led off") {
    digitalWrite(LED_PIN, LOW);  logf("LED mati");
  } else if (cmd == "reboot") {
    logf("restart..."); delay(300); ESP.restart();

  } else if (cmd == "can") {
    twai_status_info_t s;
    if (twai_get_status_info(&s) == ESP_OK) {
      const char* st[] = {"STOPPED", "RUNNING", "BUS_OFF", "RECOVERING"};
      logf("CAN state=%s TEC=%lu REC=%lu txq=%lu rxq=%lu bus_err=%lu rx_lost=%lu",
           st[s.state], (unsigned long)s.tx_error_counter, (unsigned long)s.rx_error_counter,
           (unsigned long)s.msgs_to_tx, (unsigned long)s.msgs_to_rx,
           (unsigned long)s.bus_error_count, (unsigned long)s.rx_missed_count);
    }
  } else if (cmd == "can verbose on") {
    canVerbose = true;  logf("log per frame ON (banyak!)");
  } else if (cmd == "can verbose off") {
    canVerbose = false; logf("log per frame OFF");
  } else if (cmd == "can stop") {
    if (canRunning) { twai_stop(); canRunning = false; }
    logf("CAN dihentikan");
  } else if (cmd == "can start") {
    if (!canRunning && twai_start() == ESP_OK) { canRunning = true; lastHbMs = millis(); }
    logf("CAN %s", canRunning ? "jalan" : "gagal start");
  } else {
    logf("perintah tidak dikenal, ketik help");
  }
}

// ============================================================
// 5. SETUP DAN LOOP
// ============================================================
void setup() {
  Serial.begin(115200);
  Serial.setTimeout(20);
  pinMode(LED_PIN, OUTPUT);

  otaSetup();
  setupCAN();
  lastHbMs = lastSummaryMs = millis();
}

void loop() {
  otaLoop();

  handleCanAlerts();
  handleCanRx();

  if (canRunning && millis() - lastHbMs >= HB_PERIOD_MS) {
    lastHbMs += HB_PERIOD_MS;
    sendHeartbeat();
  }

  if (!otaActive && millis() - lastSummaryMs >= 1000) {
    lastSummaryMs += 1000;
    printCanSummary();
  }
}
