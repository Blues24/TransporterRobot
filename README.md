# 🤖 TransporterRobot — WIRAGORA POLINDRA

Robot transporter 4 roda mecanum berbasis **ESP32** dengan kendali **PS3 DualShock** via Bluetooth, motor driver **TB6612FNG**, dan konfigurasi via **Web Dashboard**.

---

## 📁 Struktur Modul

```
Transporter/
├── Transporter.ino       # Main sketch — setup() & loop()
├── config.h              # Semua pin, konstanta, dan struct config
├── motor_conf.h/.cpp     # Kontrol 4 motor (TB6612FNG, brake mode)
├── servo_lift.h/.cpp     # Servo gripper, lift motor
├── ps3_handler.h/.cpp    # PS3 input, button binding, Emergency Stop
├── web_server_mgr.h/.cpp # WiFi AP, Web Dashboard, config persistence
└── display.h/.cpp        # OLED SSD1306 display
```

---

## 🛠️ Hardware

| Komponen | Spesifikasi |
|---|---|
| Mikrokontroler | ESP32 (38-pin) |
| Motor Driver | TB6612FNG × 2 IC |
| Motor | DC brushed × 4 (roda mecanum) |
| Lift Motor | DC brushed × 1 (via TB6612FNG channel sisa) |
| Servo | Standard servo × 1 (gripper) |
| Display | OLED SSD1306 128×64 (I2C) |
| Controller | PS3 DualShock (Bluetooth) |
| Buzzer | Passive buzzer |

### Pin Configuration

Semua pin didefinisikan di `config.h` — ubah di sini jika ada revisi hardware:

```cpp
// STBY — shared untuk semua motor + lifter
#define PIN_MOTOR_STBY  2

// IC1: FRONT_RIGHT (Ch.A) & FRONT_LEFT (Ch.B)
#define PIN_FR_IN1  17   #define PIN_FL_IN1  5
#define PIN_FR_IN2  16   #define PIN_FL_IN2  18
#define PIN_FR_PWM  4    #define PIN_FL_PWM  19

// IC2: BACK_RIGHT (Ch.A) & BACK_LEFT (Ch.B)
#define PIN_BR_IN1  33   #define PIN_BL_IN1  26
#define PIN_BR_IN2  25   #define PIN_BL_IN2  27
#define PIN_BR_PWM  32   #define PIN_BL_PWM  14

// Lifter
#define LIFT_IN1  3
#define LIFT_IN2  1
#define LIFT_PWM  23

// Peripheral
#define SERVO_PIN_R  33
#define BUZZER_PIN   0   // ⚠️ placeholder — sesuaikan dengan skematik
```

---

## 📦 Instalasi Library

### 1. ESP32 Board Package

Tambahkan URL berikut di Arduino IDE:
**File → Preferences → Additional Boards Manager URLs**

```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

Kemudian install via **Tools → Board → Boards Manager**:
- Cari `esp32` by Espressif Systems → **Install**

### 2. Library via Arduino Library Manager

Buka **Sketch → Include Library → Manage Libraries**, lalu install:

| Library | Author | Versi | Keterangan |
|---|---|---|---|
| **Adafruit GFX Library** | Adafruit | ≥ 1.11.0 | Dependency wajib untuk SSD1306 |
| **Adafruit SSD1306** | Adafruit | ≥ 2.5.0 | Driver OLED display |
| **ESP32Servo** | Kevin Harrington | ≥ 0.13.0 | Servo support untuk ESP32 |

> ⚠️ Saat install **Adafruit SSD1306**, Arduino IDE akan bertanya apakah ingin install dependencies — pilih **"Install All"** agar Adafruit GFX ikut terinstall.

### 3. Library Manual (tidak tersedia di Library Manager)

#### PS3 Controller Library (PS3_Controller_Host — patched)

Library ini sudah di-patch manual untuk kompatibilitas dengan ESP32 Arduino core terbaru dan tersedia di repo project ini.

**Langkah instalasi:**
1. Copy folder `PS3_Controller_Host` dari repo ke direktori libraries Arduino:
   ```
   Windows : C:\Users\<nama>\Documents\Arduino\libraries\
   macOS   : ~/Documents/Arduino/libraries/
   Linux   : ~/Arduino/libraries/
   ```
2. Atau via Arduino IDE: **Sketch → Include Library → Add .ZIP Library**

> ⚠️ Jangan install `PS3_Controller_Host` dari sumber lain — gunakan versi yang sudah di-patch di repo ini karena versi original tidak kompatibel dengan ESP32 core 3.x.

### 4. Library Built-in (tidak perlu install)

Library berikut sudah tersedia otomatis bersama ESP32 board package:

| Library | Keterangan |
|---|---|
| `WiFi.h` | WiFi & Access Point |
| `WebServer.h` | HTTP Web Server |
| `Preferences.h` | Penyimpanan config ke flash (NVS) |
| `Wire.h` | Komunikasi I2C |

---

## 🎮 Kontrol PS3

| Input | Fungsi |
|---|---|
| Analog kiri | Maju / Mundur / Strafe |
| Analog kanan | Putar di tempat / Arc turn |
| R2 (analog) | Tambah kecepatan |
| L2 (analog) | Kurangi kecepatan |
| R1 | Lift naik |
| L1 | Lift turun |
| ✕ (Cross) | Gripper tutup |
| □ (Square) | Gripper buka |
| △ (Triangle) | Buzzer beep |
| D-Pad | Gerak alternatif (maju/mundur/putar) |
| SELECT | Tampilkan logo |
| PS Button | Tampilkan status baterai |
| **L3 + R3** | ⛔ **Emergency Stop** |
| Gerak stick | ✅ Resume dari Emergency Stop |

---

## 🌐 Web Dashboard

Setelah ESP32 menyala, hubungkan ke WiFi:

| Parameter | Value |
|---|---|
| SSID | `Transporter 17` |
| Password | `fajarganteng` |
| URL | `http://192.168.4.1` |

### Fitur Dashboard

| Tab | Fitur |
|---|---|
| ⚙️ Motor | Base / Min / Max speed per motor individual |
| 🦾 Servo & Lift | Open/close angle gripper, speed lift up/down, test langsung |
| 📺 Display | Brightness, contrast, font size, upload custom logo BMP |
| 🎮 PS3 | Set MAC address controller, status koneksi live |
| 🔧 System | Emergency Stop, Restart ESP32, Factory Reset |

### Ganti MAC Address PS3

MAC address controller bisa diubah tanpa upload ulang sketch:
1. Buka tab **🎮 PS3** di Web Dashboard
2. Masukkan MAC address format `xx:xx:xx:xx:xx:xx`
3. Klik **Save MAC & Restart** — ESP32 restart otomatis

Cara mendapatkan MAC address PS3:
- Gunakan **SixaxisPairTool** di PC via kabel USB
- Download: https://sixaxispairtool.en.lo4d.com

---

## ⚙️ Konfigurasi Board di Arduino IDE

| Setting | Value |
|---|---|
| Board | ESP32 Dev Module |
| Upload Speed | 921600 |
| CPU Frequency | 240MHz (WiFi/BT) |
| Flash Frequency | 80MHz |
| Flash Mode | QIO |
| Flash Size | 4MB (32Mb) |
| Partition Scheme | **Huge APP (3MB No OTA/1MB SPIFFS)** |
| PSRAM | Disabled |
| Port | (sesuai port ESP32 di PC) |

---

## 📋 Checklist Sebelum Upload

- [ ] Semua library sudah terinstall (gunakan PS3_Controller_Host yang sudah di-patch dari repo)
- [ ] Board setting sudah sesuai tabel di atas — pastikan **Partition Scheme: Huge APP**
- [ ] MAC address PS3 sudah diset (bisa via WebServer setelah upload)
- [ ] `BUZZER_PIN` di `config.h` sudah disesuaikan dengan skematik
- [ ] Pin di `config.h` sudah sesuai dengan skema hardware

---

## 🔧 Troubleshooting

**Motor tidak bergerak sama sekali**
→ Cek pin `PIN_MOTOR_STBY` — harus HIGH agar TB6612FNG aktif

**PS3 tidak terhubung**
→ MAC address belum di-set atau salah — buka Web Dashboard tab PS3

**Web Dashboard tidak bisa diakses**
→ Pastikan terhubung ke WiFi `Transporter 17`, bukan WiFi lain

**Sketch terlalu besar saat compile**
→ Pastikan Partition Scheme di Arduino IDE sudah diset ke **Huge APP (3MB No OTA/1MB SPIFFS)**

**Error compile library PS3**
→ Pastikan menggunakan `PS3_Controller_Host` yang sudah di-patch dari repo ini, bukan versi original dari jvpernis

---
