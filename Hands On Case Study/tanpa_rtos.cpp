#include <Arduino.h>
#include <ESP32Servo.h>

// GPIO 4, 5, 23, 26, 27 dipakai modem SIM800L di board T-Call.
#define TRIG_MASUK  32
#define ECHO_MASUK  18
#define SERVO_MASUK 33

#define TRIG_KELUAR  19
#define ECHO_KELUAR  21
#define SERVO_KELUAR 25

#define KAPASITAS_MAX   5
#define JARAK_DETEKSI  15
#define SERVO_TUTUP     0
#define SERVO_BUKA     90
#define WAKTU_BUKA   1000
#define PERIODE_LOOP  100

Servo servoMasuk;
Servo servoKeluar;

int jumlahPengunjung = 0;

long bacaJarak(int pinTrig, int pinEcho)
{
  digitalWrite(pinTrig, LOW);
  delayMicroseconds(2);
  digitalWrite(pinTrig, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinTrig, LOW);

  long durasi = pulseIn(pinEcho, HIGH, 30000);
  if (durasi == 0) return -1;

  return durasi * 0.034 / 2;
}

void cetakStatus()
{
  static int sebelumnya = -1;

  if (jumlahPengunjung == sebelumnya) return;

  Serial.println("Di dalam: " + String(jumlahPengunjung) +
                 "/" + String(KAPASITAS_MAX) +
                 (jumlahPengunjung >= KAPASITAS_MAX ? "  [PENUH]"
                                                    : "  [TERSEDIA]"));
  sebelumnya = jumlahPengunjung;
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  pinMode(TRIG_MASUK, OUTPUT);
  pinMode(ECHO_MASUK, INPUT);
  pinMode(TRIG_KELUAR, OUTPUT);
  pinMode(ECHO_KELUAR, INPUT);

  servoMasuk.setPeriodHertz(50);
  servoMasuk.attach(SERVO_MASUK, 500, 2400);
  servoMasuk.write(SERVO_TUTUP);

  servoKeluar.setPeriodHertz(50);
  servoKeluar.attach(SERVO_KELUAR, 500, 2400);
  servoKeluar.write(SERVO_TUTUP);

  Serial.println("\nSMART BUILDING - Superloop");
  Serial.println("Kapasitas maksimum: " + String(KAPASITAS_MAX) + " orang\n");

  cetakStatus();
}

void loop()
{
  long jarakMasuk = bacaJarak(TRIG_MASUK, ECHO_MASUK);

  if (jarakMasuk > 0 && jarakMasuk < JARAK_DETEKSI)
  {
    if (jumlahPengunjung < KAPASITAS_MAX)
    {
      jumlahPengunjung++;
      cetakStatus();

      servoMasuk.write(SERVO_BUKA);
      delay(WAKTU_BUKA);          // sensor keluar tidak terbaca selama ini
      servoMasuk.write(SERVO_TUTUP);
    }
  }

  long jarakKeluar = bacaJarak(TRIG_KELUAR, ECHO_KELUAR);

  if (jarakKeluar > 0 && jarakKeluar < JARAK_DETEKSI)
  {
    if (jumlahPengunjung > 0)
    {
      jumlahPengunjung--;
      cetakStatus();

      servoKeluar.write(SERVO_BUKA);
      delay(WAKTU_BUKA);          // sensor masuk tidak terbaca selama ini
      servoKeluar.write(SERVO_TUTUP);
    }
  }

  delay(PERIODE_LOOP);
}
