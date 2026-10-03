/*
  ESP32 Bench (versi belajar)
  OTA update + serial monitor lewat browser, dalam satu file.

  Cara pakai singkat:
    1. Isi WIFI_SSID, WIFI_PASS, dan ganti AUTH_PASS di bagian KONFIGURASI.
    2. Upload sketch ini ke ESP32 lewat kabel USB (cukup sekali).
    3. Lihat alamat IP di Serial Monitor, lalu buka di browser.
    4. Setelah itu firmware baru bisa dikirim lewat browser (OTA), tanpa kabel.

  Penting: firmware pengganti yang kamu upload lewat OTA HARUS tetap memuat kode OTA ini.
  Kalau tidak, OTA berikutnya tidak bisa dan kamu harus upload ulang lewat kabel.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

// ============================================================
// 1. KONFIGURASI (ubah sesuai kebutuhan)
// ============================================================
const char* WIFI_SSID  = "NAMA_WIFI";            // WiFi harus 2,4 GHz
const char* WIFI_PASS  = "PASSWORD_WIFI";
const char* NODE_ID    = "esp32bench";           // dipakai juga sebagai nama: esp32bench.local
const char* FW_VERSION = "1.0.0";                // naikkan angkanya setiap kali update
const char* AUTH_USER  = "admin";
const char* AUTH_PASS  = "ganti-password-ini";   // WAJIB diganti
const int   LED_PIN    = 2;                      // LED bawaan di kebanyakan board ESP32

WebServer server(80);                            // web server di port 80

// ============================================================
// 2. LOG
//    Pesan disimpan di "ring buffer": 100 tempat, kalau penuh
//    pesan paling lama ditimpa. Browser mengambil pesan yang baru.
//    Pakai logf() seperti Serial.printf(): pesan muncul di
//    Serial Monitor DAN di dashboard web.
// ============================================================
const int LOG_N = 100;
String ring[LOG_N];          // tempat menyimpan pesan
uint32_t logSeq = 0;         // nomor urut pesan terakhir (naik terus)

void logf(const char* format, ...) {
  char buf[160];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  Serial.println(buf);                 // tampil di Serial Monitor
  logSeq++;                            // nomor pesan baru
  ring[logSeq % LOG_N] = buf;          // simpan di ring buffer
}

// ============================================================
// 3. PERINTAH
//    Perintah dari kotak input di web (atau Serial Monitor)
//    diproses di sini. Mau tambah perintah? Tambah "else if" baru.
// ============================================================
void handleCmd(String cmd) {
  cmd.trim();                          // buang spasi dan enter di ujung
  if (cmd == "") return;

  logf("> %s", cmd.c_str());           // tampilkan perintah yang diterima

  if (cmd == "help") {
    logf("perintah: help, ver, status, led on, led off, reboot");
  } else if (cmd == "ver") {
    logf("%s firmware %s", NODE_ID, FW_VERSION);
  } else if (cmd == "status") {
    logf("uptime %lu detik, heap bebas %u byte, RSSI %d dBm",
         millis() / 1000, (unsigned)ESP.getFreeHeap(), WiFi.RSSI());
  } else if (cmd == "led on") {
    digitalWrite(LED_PIN, HIGH);
    logf("LED menyala");
  } else if (cmd == "led off") {
    digitalWrite(LED_PIN, LOW);
    logf("LED mati");
  } else if (cmd == "reboot") {
    logf("restart...");
    delay(300);
    ESP.restart();
  } else {
    logf("perintah tidak dikenal, ketik help");
  }
}

// ============================================================
// 4. HALAMAN WEB
//    Seluruh halaman (HTML + CSS + JavaScript) disimpan sebagai
//    teks di dalam firmware. ESP32 mengirimnya ke browser saat
//    alamat "/" dibuka.
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
  let nomorTerakhir = 0;      // nomor pesan terakhir yang sudah diterima browser

  // ---- Serial monitor: minta pesan baru ke ESP32 setiap 0,5 detik ----
  async function ambilLog() {
    try {
      const respon = await fetch('/log?since=' + nomorTerakhir);
      const teks = await respon.text();
      const baris = teks.split('\n');
      nomorTerakhir = Number(baris.shift());      // baris pertama = nomor pesan terakhir
      for (const b of baris) {
        if (b === '') continue;
        const div = document.createElement('div');
        div.textContent = b;
        terminal.appendChild(div);
      }
      terminal.scrollTop = terminal.scrollHeight; // gulung ke bawah
    } catch (e) { /* ESP32 sedang restart, coba lagi */ }
    setTimeout(ambilLog, 500);
  }

  // ---- Kirim perintah ke ESP32 ----
  async function kirimPerintah() {
    const kotak = document.getElementById('perintah');
    if (kotak.value === '') return;
    await fetch('/cmd', { method: 'POST', body: new URLSearchParams({ c: kotak.value }) });
    kotak.value = '';
  }
  document.getElementById('perintah').addEventListener('keydown', (e) => {
    if (e.key === 'Enter') kirimPerintah();
  });

  // ---- Tampilkan info ESP32 di bagian atas ----
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

  // ---- OTA: kirim file .bin ke ESP32 ----
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

  // Setelah OTA, cek terus sampai ESP32 online lagi
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
// 5. PENANGAN PERMINTAAN DARI BROWSER
//    Setiap alamat (/, /info, /log, /cmd, /update) punya satu fungsi.
// ============================================================

// Cek username dan password. Kalau salah, browser diminta login.
bool cekLogin() {
  if (server.authenticate(AUTH_USER, AUTH_PASS)) return true;
  server.requestAuthentication();
  return false;
}

// Alamat "/" : kirim halaman dashboard
void handleRoot() {
  if (!cekLogin()) return;
  server.send_P(200, "text/html", PAGE);
}

// Alamat "/info" : kirim data ESP32 dalam format JSON
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

// Alamat "/log?since=N" : kirim pesan log yang nomornya lebih besar dari N.
// Baris pertama balasan = nomor pesan terakhir, baris berikutnya = pesannya.
void handleLog() {
  if (!cekLogin()) return;
  uint32_t since = server.arg("since").toInt();
  if (since > logSeq) since = 0;                        // ESP32 baru restart
  if (logSeq - since > LOG_N) since = logSeq - LOG_N;   // browser ketinggalan jauh

  String out = String(logSeq) + "\n";
  for (uint32_t n = since + 1; n <= logSeq; n++) {
    out += ring[n % LOG_N] + "\n";
  }
  server.send(200, "text/plain", out);
}

// Alamat "/cmd" : terima perintah dari kotak input
void handleCmdRoute() {
  if (!cekLogin()) return;
  handleCmd(server.arg("c"));
  server.send(200, "text/plain", "ok");
}

// ---- OTA ----
// Upload file terjadi dalam potongan-potongan kecil. Fungsi handleUpdateUpload()
// dipanggil berulang kali, satu kali per potongan. Setelah semua potongan
// diterima, barulah handleUpdateSelesai() dipanggil.
bool uploadDitolak = false;

void handleUpdateUpload() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {                 // potongan pertama
    uploadDitolak = !server.authenticate(AUTH_USER, AUTH_PASS);
    if (uploadDitolak) return;
    logf("OTA mulai: %s", upload.filename.c_str());
    Update.begin(UPDATE_SIZE_UNKNOWN);                      // siapkan partisi tujuan

  } else if (uploadDitolak) {
    return;                                                 // login salah, abaikan sisanya

  } else if (upload.status == UPLOAD_FILE_WRITE) {          // potongan data
    Update.write(upload.buf, upload.currentSize);           // tulis ke flash

  } else if (upload.status == UPLOAD_FILE_END) {            // potongan terakhir
    if (Update.end(true)) logf("OTA selesai: %u byte", (unsigned)upload.totalSize);
    else                  logf("OTA gagal");
  }
}

void handleUpdateSelesai() {
  if (!cekLogin()) return;
  if (Update.hasError()) {
    server.send(500, "text/plain", "FAIL");
    return;
  }
  server.send(200, "text/plain", "OK");
  logf("restart untuk memakai firmware baru");
  delay(500);
  ESP.restart();                                            // boot ke firmware baru
}

// ============================================================
// 6. SETUP DAN LOOP
// ============================================================
void setup() {
  Serial.begin(115200);
  Serial.setTimeout(20);
  pinMode(LED_PIN, OUTPUT);

  // Sambung ke WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Menghubungkan WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }
  Serial.println();

  MDNS.begin(NODE_ID);        // agar bisa dibuka lewat http://esp32bench.local

  // Daftarkan alamat-alamat web
  server.on("/",       HTTP_GET,  handleRoot);
  server.on("/info",   HTTP_GET,  handleInfo);
  server.on("/log",    HTTP_GET,  handleLog);
  server.on("/cmd",    HTTP_POST, handleCmdRoute);
  server.on("/update", HTTP_POST, handleUpdateSelesai, handleUpdateUpload);
  server.begin();

  logf("boot %s firmware %s", NODE_ID, FW_VERSION);
  logf("buka http://%s atau http://%s.local", WiFi.localIP().toString().c_str(), NODE_ID);
}

void loop() {
  server.handleClient();                       // layani permintaan browser

  if (Serial.available()) {                    // perintah dari Serial Monitor kabel
    handleCmd(Serial.readStringUntil('\n'));
  }

  // Contoh pesan berkala tanpa delay(): kirim log tiap 5 detik
  static unsigned long terakhir = 0;
  if (millis() - terakhir >= 5000) {
    terakhir = millis();
    logf("detak, uptime %lu detik", millis() / 1000);
  }
}
