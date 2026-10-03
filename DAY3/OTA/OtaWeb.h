/*
  OtaWeb.h  -  OTA update + serial monitor lewat browser
  (dipindahkan dari sketch "ESP32 Bench", isinya hampir sama)

  File ini SAMA PERSIS di Node A dan Node B. Jangan diubah per node.
  Hal yang berbeda per node (NODE_ID, FW_VERSION, kode CAN) ada di file .ino.

  Yang ditambahkan dibanding versi asli:
    - otaSetup() / otaLoop()  -> dipanggil dari setup() / loop() di .ino
    - WiFi punya batas waktu, jadi CAN tetap jalan walau WiFi mati
    - onOtaStart() / onOtaFailed() -> "hook" ke .ino, agar CAN bisa
      dihentikan dengan rapi saat flash sedang ditulis
    - Update.begin() dicek hasilnya

  Sebelum #include "OtaWeb.h", file .ino WAJIB mendefinisikan:
    WIFI_SSID, WIFI_PASS, NODE_ID, FW_VERSION, AUTH_USER, AUTH_PASS, LED_PIN
  dan juga fungsi:
    void handleCmd(String cmd);  void onOtaStart();  void onOtaFailed();
*/
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <stdarg.h>

// Fungsi-fungsi ini ditulis di file .ino
void handleCmd(String cmd);
void onOtaStart();
void onOtaFailed();

WebServer server(80);

// ============================================================
// LOG (ring buffer 100 pesan) - pakai logf() seperti Serial.printf()
// ============================================================
const int LOG_N = 100;
String ring[LOG_N];
uint32_t logSeq = 0;

void logf(const char* format, ...) {
  char buf[160];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  Serial.println(buf);
  logSeq++;
  ring[logSeq % LOG_N] = buf;
}

// ============================================================
// HALAMAN WEB (tidak berubah dari versi asli)
// ============================================================
const char PAGE[] PROGMEM = R"HTML(
<!doctype html>
<html lang="id">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 Bench</title>
<style>
  body { font-family: sans-serif; background: #eef1f4; margin: 0; padding: 16px; color: #17222b; }
  h1 { font-size: 20px; margin: 0 0 4px; }
  h2 { font-size: 16px; margin-top: 0; }
  #info { color: #5d6b76; font-size: 14px; margin-bottom: 16px; }
  .kotak { background: white; border: 1px solid #cfd6dc; border-radius: 8px;
           padding: 14px; margin-bottom: 16px; max-width: 760px; }
  #terminal { height: 280px; overflow: auto; background: #16212b; color: #d7e2ea;
              padding: 8px; border-radius: 6px; font: 13px monospace; white-space: pre-wrap; }
  input, button { font-size: 15px; padding: 6px 10px; }
  button { cursor: pointer; }
  progress { width: 100%; margin-top: 10px; }
</style>
</head>
<body>

<h1>ESP32 Bench</h1>
<div id="info">menghubungkan...</div>

<div class="kotak">
  <h2>Serial monitor</h2>
  <div id="terminal"></div>
  <p>
    <input id="perintah" placeholder="ketik perintah, mis. help" size="30">
    <button onclick="kirimPerintah()">Kirim</button>
  </p>
</div>

<div class="kotak">
  <h2>OTA update</h2>
  <input type="file" id="berkas" accept=".bin">
  <button onclick="uploadFirmware()">Upload</button>
  <progress id="bar" value="0" max="100"></progress>
  <div id="pesanOta"></div>
</div>

<script>
  const terminal = document.getElementById('terminal');
  const info     = document.getElementById('info');
  const bar      = document.getElementById('bar');
  const pesanOta = document.getElementById('pesanOta');
  let nomorTerakhir = 0;

  async function ambilLog() {
    try {
      const respon = await fetch('/log?since=' + nomorTerakhir);
      const teks = await respon.text();
      const baris = teks.split('\n');
      nomorTerakhir = Number(baris.shift());
      for (const b of baris) {
        if (b === '') continue;
        const div = document.createElement('div');
        div.textContent = b;
        terminal.appendChild(div);
      }
      terminal.scrollTop = terminal.scrollHeight;
    } catch (e) { }
    setTimeout(ambilLog, 500);
  }

  async function kirimPerintah() {
    const kotak = document.getElementById('perintah');
    if (kotak.value === '') return;
    await fetch('/cmd', { method: 'POST', body: new URLSearchParams({ c: kotak.value }) });
    kotak.value = '';
  }
  document.getElementById('perintah').addEventListener('keydown', (e) => {
    if (e.key === 'Enter') kirimPerintah();
  });

  async function tampilInfo() {
    try {
      const j = await (await fetch('/info')).json();
      info.textContent = j.node + '  |  firmware ' + j.fw + '  |  IP ' + j.ip + '  |  uptime ' + j.up + ' detik';
      return true;
    } catch (e) {
      info.textContent = 'offline';
      return false;
    }
  }

  function uploadFirmware() {
    const berkas = document.getElementById('berkas').files[0];
    if (!berkas) { pesanOta.textContent = 'Pilih file .bin dulu.'; return; }
    const data = new FormData();
    data.append('firmware', berkas);
    const xhr = new XMLHttpRequest();
    xhr.open('POST', '/update');
    xhr.upload.onprogress = (e) => { bar.value = (e.loaded / e.total) * 100; };
    xhr.onload = () => {
      if (xhr.status === 200) {
        pesanOta.textContent = 'Terkirim. ESP32 restart, menunggu online lagi...';
        setTimeout(tunggu, 4000);
      } else {
        pesanOta.textContent = 'Gagal: ' + xhr.responseText;
      }
    };
    xhr.onerror = () => { pesanOta.textContent = 'Koneksi putus.'; };
    xhr.send(data);
  }

  async function tunggu() {
    if (await tampilInfo()) pesanOta.textContent = 'ESP32 online lagi. Cek versi firmware di atas.';
    else setTimeout(tunggu, 2000);
  }

  ambilLog();
  tampilInfo();
  setInterval(tampilInfo, 3000);
</script>
</body>
</html>
)HTML";

// ============================================================
// PENANGAN PERMINTAAN DARI BROWSER
// ============================================================
bool cekLogin() {
  if (server.authenticate(AUTH_USER, AUTH_PASS)) return true;
  server.requestAuthentication();
  return false;
}

void handleRoot() {
  if (!cekLogin()) return;
  server.send_P(200, "text/html", PAGE);
}

void handleInfo() {
  if (!cekLogin()) return;
  String json = "{";
  json += "\"node\":\"" + String(NODE_ID) + "\",";
  json += "\"fw\":\"" + String(FW_VERSION) + "\",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"up\":" + String(millis() / 1000);
  json += "}";
  server.send(200, "application/json", json);
}

void handleLog() {
  if (!cekLogin()) return;
  uint32_t since = server.arg("since").toInt();
  if (since > logSeq) since = 0;
  if (logSeq - since > LOG_N) since = logSeq - LOG_N;

  String out = String(logSeq) + "\n";
  for (uint32_t n = since + 1; n <= logSeq; n++) {
    out += ring[n % LOG_N] + "\n";
  }
  server.send(200, "text/plain", out);
}

void handleCmdRoute() {
  if (!cekLogin()) return;
  handleCmd(server.arg("c"));
  server.send(200, "text/plain", "ok");
}

// ---- OTA ----
bool uploadDitolak = false;

void handleUpdateUpload() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    uploadDitolak = !server.authenticate(AUTH_USER, AUTH_PASS);
    if (uploadDitolak) return;
    logf("OTA mulai: %s", upload.filename.c_str());
    onOtaStart();                                           // BARU: hentikan CAN dulu
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {               // BARU: cek hasilnya
      logf("OTA gagal mulai (partisi OTA tidak ada?)");
    }

  } else if (uploadDitolak) {
    return;

  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      // error disimpan di Update, dicek di handleUpdateSelesai()
    }

  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) logf("OTA selesai: %u byte", (unsigned)upload.totalSize);
    else                  logf("OTA gagal");

  } else if (upload.status == UPLOAD_FILE_ABORTED) {         // BARU: koneksi putus
    Update.abort();
    logf("OTA dibatalkan");
    onOtaFailed();
  }
}

void handleUpdateSelesai() {
  if (!cekLogin()) return;
  if (Update.hasError()) {
    server.send(500, "text/plain", "FAIL");
    onOtaFailed();                                          // BARU: hidupkan CAN lagi
    return;
  }
  server.send(200, "text/plain", "OK");
  logf("restart untuk memakai firmware baru");
  delay(500);
  ESP.restart();
}

// ============================================================
// otaSetup() / otaLoop()
// ============================================================
void otaSetup() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Menghubungkan WiFi");

  // BARU: maksimal 15 detik. Kalau gagal, node tetap jalan (CAN aktif),
  // WiFi akan mencoba sambung ulang sendiri di latar belakang.
  unsigned long mulai = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - mulai < 15000) {
    delay(400);
    Serial.print(".");
  }
  Serial.println();

  MDNS.begin(NODE_ID);

  server.on("/",       HTTP_GET,  handleRoot);
  server.on("/info",   HTTP_GET,  handleInfo);
  server.on("/log",    HTTP_GET,  handleLog);
  server.on("/cmd",    HTTP_POST, handleCmdRoute);
  server.on("/update", HTTP_POST, handleUpdateSelesai, handleUpdateUpload);
  server.begin();

  logf("boot %s firmware %s", NODE_ID, FW_VERSION);
  if (WiFi.status() == WL_CONNECTED)
    logf("buka http://%s atau http://%s.local", WiFi.localIP().toString().c_str(), NODE_ID);
  else
    logf("WiFi belum tersambung - CAN tetap jalan, OTA menunggu WiFi");
}

void otaLoop() {
  server.handleClient();
  if (Serial.available()) {
    handleCmd(Serial.readStringUntil('\n'));
  }
}
