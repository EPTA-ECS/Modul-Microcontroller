# ESP32 Bench: OTA dan Serial Monitor lewat Browser

Bahan ajar. Satu file `main.cpp`, tanpa library tambahan. ESP32 menyajikan halaman web sendiri berisi:

- **Serial monitor**: melihat pesan dari ESP32 dan mengirim perintah.
- **OTA update**: mengganti firmware lewat WiFi, tanpa kabel USB.

## Tujuan belajar

Setelah mempraktikkan materi ini, kamu bisa:

1. Menjelaskan apa itu OTA dan kenapa berguna (perangkat yang sulit dijangkau kabel).
2. Membuat web server di ESP32 dengan `WebServer`.
3. Membuat serial monitor di web dengan pola *polling*.
4. Mengunggah dan memasang firmware baru lewat WiFi.

## Konsep singkat

**OTA (Over-The-Air)**: ESP32 menerima file firmware (`.bin`) lewat WiFi, menuliskannya ke partisi flash cadangan, lalu restart dan boot dari partisi baru. Syaratnya, firmware yang berjalan harus memuat kode penerima OTA, dan flash pertama tetap lewat kabel.

**Web server di ESP32**: ESP32 menunggu permintaan dari browser di port 80. Tiap alamat (`/`, `/log`, `/update`, dst.) dihubungkan ke satu fungsi.

**Polling**: browser bertanya ke ESP32 berulang kali ("ada pesan baru?"), bukan ESP32 yang mengirim sendiri. Sederhana, dan cukup untuk serial monitor.

```
  Browser                               ESP32
     |  GET /            ------------->  kirim halaman dashboard
     |  GET /log?since=5 ------------->  kirim pesan nomor 6 dan seterusnya   (tiap 0,5 detik)
     |  POST /cmd        ------------->  jalankan perintah, mis. "led on"
     |  POST /update     ------------->  terima file .bin, tulis ke flash, restart
```

## Peta kode

| Bagian di `main.cpp` | Isi | Yang dipelajari |
|---|---|---|
| 1. Konfigurasi | WiFi, nama, password | Variabel konstanta |
| 2. Log | `logf()` dan ring buffer | Array, `printf`, buffer melingkar |
| 3. Perintah | `handleCmd()` | `if / else if`, `String` |
| 4. Halaman web | `PAGE` (HTML, CSS, JS) | Web disimpan di dalam firmware |
| 5. Penangan | `handleRoot`, `handleLog`, `handleUpdate...` | Routing, HTTP GET/POST, upload |
| 6. Setup dan loop | WiFi, daftar alamat, `loop()` | `millis()` tanpa `delay()` |

## Praktik dengan Arduino IDE

**Persiapan sekali saja**

1. *File → Preferences → Additional boards manager URLs*: tambahkan `https://espressif.github.io/arduino-esp32/package_esp32_index.json`.
2. *Tools → Board → Boards Manager*: cari dan instal **esp32 by Espressif Systems**.

**Upload pertama (lewat kabel)**

3. Buat sketch baru, tempel isi `main.cpp`, simpan.
4. Ubah `WIFI_SSID`, `WIFI_PASS`, dan **`AUTH_PASS`**. WiFi harus 2,4 GHz.
5. Menu *Tools*: Board = *ESP32 Dev Module*, Partition Scheme = *Default 4MB with spiffs* (bukan "No OTA"), pilih Port.
6. Klik **Upload**. Kalau macet di "Connecting......", tahan tombol BOOT di board.
7. Buka *Tools → Serial Monitor* (115200). Akan muncul alamat, misalnya `buka http://192.168.1.50`.

**Buka dashboard**

8. Dari laptop atau HP yang satu WiFi, buka alamat itu di browser. Login dengan `admin` dan password yang kamu isi. Kalau `esp32bench.local` tidak terbuka, pakai alamat IP.
9. Ketik `help`, `status`, `led on`, `led off` di kotak perintah. Pesan `detak` muncul tiap 5 detik.

**Coba OTA**

10. Ubah `FW_VERSION` menjadi `"1.0.1"`. Boleh juga ubah pesan `detak` supaya perubahannya terlihat.
11. *Sketch → Export Compiled Binary*, lalu buka *Sketch → Show Sketch Folder* dan masuk ke folder `build/…`. Ambil file **`namasketch.ino.bin`** (bukan yang berakhiran `bootloader`, `partitions`, atau `merged`).
12. Di dashboard pilih file itu, klik **Upload**. Progress bar berjalan, ESP32 restart, dan versi di bagian atas berubah jadi 1.0.1.

## Latihan

| No | Tingkat | Tugas |
|---|---|---|
| 1 | Mudah | Tambah perintah `halo` yang membalas "halo juga" (tambah satu `else if` di `handleCmd`) |
| 2 | Mudah | Ubah interval pesan `detak` dari 5 detik menjadi 2 detik, lalu update lewat OTA |
| 3 | Sedang | Tambah perintah `blink` yang mengedipkan LED 3 kali |
| 4 | Sedang | Tampilkan pembacaan sensor (mis. `analogRead(34)`) di log tiap 1 detik memakai `logf()` |
| 5 | Sedang | Tambah tombol "LED ON/OFF" di halaman web yang memanggil `/cmd` |
| 6 | Sulit | Tolak OTA kalau kondisi tidak aman: buat variabel `bool amanUpdate`, cek di `handleUpdateUpload()` saat `UPLOAD_FILE_START`, dan balas kode 409 |

## Kesalahan yang sering terjadi

| Gejala | Penyebab dan solusi |
|---|---|
| Upload lewat kabel gagal di "Connecting..." | Tahan tombol BOOT saat proses mulai |
| Tidak konek WiFi | WiFi 5 GHz tidak didukung, pakai 2,4 GHz. Cek SSID dan password |
| `esp32bench.local` tidak terbuka | Jaringan atau OS tidak mendukung mDNS, pakai alamat IP |
| Upload OTA menghasilkan `FAIL` | File salah (bootloader/merged) atau Partition Scheme "No OTA" |
| OTA berhasil tapi ESP32 tidak kembali online | Firmware baru tidak memuat kode OTA atau konfigurasi WiFi berubah. Upload ulang lewat kabel |
| Diminta login terus | Username atau password tidak sama dengan yang ada di firmware |

## Catatan keamanan

Password dikirim lewat HTTP biasa (tidak terenkripsi) dan firmware tidak diverifikasi. Ini cukup untuk jaringan kelas atau lab tertutup, tapi **tidak layak** untuk produk sungguhan, yang membutuhkan HTTPS, tanda tangan firmware, dan secure boot.

## Pengembangan lanjut

Folder `lanjutan/` berisi versi FreeRTOS: sensor, keselamatan, perintah, dan web berjalan sebagai task terpisah, lengkap dengan interlock yang membatalkan OTA kalau kondisi pack tidak aman. Pelajari itu setelah versi ini sudah dipahami.
