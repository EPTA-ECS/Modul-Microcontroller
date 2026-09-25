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
#define PERIODE_TASK  100
#define PERIODE_PRINT 200

Servo servoMasuk;
Servo servoKeluar;

int jumlahPengunjung = 0;
SemaphoreHandle_t dataMutex;

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

void taskMasuk(void *parameter)
{
  pinMode(TRIG_MASUK, OUTPUT);
  pinMode(ECHO_MASUK, INPUT);

  for (;;)
  {
    long jarak = bacaJarak(TRIG_MASUK, ECHO_MASUK);

    if (jarak > 0 && jarak < JARAK_DETEKSI)
    {
      bool bolehMasuk = false;

      // Cek dan tambah harus dalam satu kunci, kalau dipisah
      // taskKeluar di core lain bisa menyela di antaranya.
      xSemaphoreTake(dataMutex, portMAX_DELAY);
      if (jumlahPengunjung < KAPASITAS_MAX)
      {
        jumlahPengunjung++;
        bolehMasuk = true;
      }
      xSemaphoreGive(dataMutex);

      if (bolehMasuk)
      {
        servoMasuk.write(SERVO_BUKA);
        vTaskDelay(pdMS_TO_TICKS(WAKTU_BUKA));
        servoMasuk.write(SERVO_TUTUP);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(PERIODE_TASK));
  }
}

void taskKeluar(void *parameter)
{
  pinMode(TRIG_KELUAR, OUTPUT);
  pinMode(ECHO_KELUAR, INPUT);

  for (;;)
  {
    long jarak = bacaJarak(TRIG_KELUAR, ECHO_KELUAR);

    if (jarak > 0 && jarak < JARAK_DETEKSI)
    {
      bool bolehKeluar = false;

      xSemaphoreTake(dataMutex, portMAX_DELAY);
      if (jumlahPengunjung > 0)
      {
        jumlahPengunjung--;
        bolehKeluar = true;
      }
      xSemaphoreGive(dataMutex);

      if (bolehKeluar)
      {
        servoKeluar.write(SERVO_BUKA);
        vTaskDelay(pdMS_TO_TICKS(WAKTU_BUKA));
        servoKeluar.write(SERVO_TUTUP);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(PERIODE_TASK));
  }
}

// Satu-satunya task yang memakai Serial.
void taskSerial(void *parameter)
{
  Serial.println("\nSMART BUILDING - FreeRTOS");
  Serial.println("Kapasitas maksimum: " + String(KAPASITAS_MAX) + " orang\n");

  int sebelumnya = -1;

  for (;;)
  {
    int current;

    xSemaphoreTake(dataMutex, portMAX_DELAY);
    current = jumlahPengunjung;
    xSemaphoreGive(dataMutex);

    if (current != sebelumnya)
    {
      Serial.println("Di dalam: " + String(current) +
                     "/" + String(KAPASITAS_MAX) +
                     (current >= KAPASITAS_MAX ? "  [PENUH]"
                                               : "  [TERSEDIA]"));
      sebelumnya = current;
    }

    vTaskDelay(pdMS_TO_TICKS(PERIODE_PRINT));
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  dataMutex = xSemaphoreCreateMutex();

  servoMasuk.setPeriodHertz(50);
  servoMasuk.attach(SERVO_MASUK, 500, 2400);
  servoMasuk.write(SERVO_TUTUP);

  servoKeluar.setPeriodHertz(50);
  servoKeluar.attach(SERVO_KELUAR, 500, 2400);
  servoKeluar.write(SERVO_TUTUP);

  xTaskCreatePinnedToCore(taskMasuk,  "TaskMasuk",  4096, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(taskKeluar, "TaskKeluar", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(taskSerial, "TaskSerial", 4096, NULL, 1,
                          NULL, tskNO_AFFINITY);
}

void loop()
{
  vTaskDelay(pdMS_TO_TICKS(1000));
}
