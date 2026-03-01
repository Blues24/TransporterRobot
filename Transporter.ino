/**
 * ╔══════════════════════════════════════════════════════╗
 * ║           TRANSPORTER - Main Sketch          ║
 * ║                   POLINDRA 2024                      ║
 * ╠══════════════════════════════════════════════════════╣
 * ║  Modul :                                             ║
 * ║   config.h          - Semua struct & konstanta       ║
 * ║   motor_control     - rotateMotor, processCarMovement║
 * ║   servo_lift        - servo gripper, lift, fan       ║
 * ║   display_oled      - OLED display, custom logo      ║
 * ║   web_server_mgr    - WiFi AP, WebServer dashboard   ║
 * ║   ps3_handler       - PS3 input & button binding     ║
 * ╚══════════════════════════════════════════════════════╝
 */

#include "config.h"
#include "motor_control.h"
#include "servo_lift.h"
#include "display.h"
#include "web_server_mgr.h"
#include "ps3_handler.h"

// ============================================================
//  GLOBAL STATE (definisi — extern di config.h)
// ============================================================
unsigned long lastRecvTime  = 0;
volatile bool pendingRestart = false;
volatile bool robotMoving    = false;

// Logo scroll state (non-blocking, menggantikan delay() lama)
volatile bool logoScrollActive = false;
bool          logoScrollRight  = true;
unsigned long logoScrollTimer  = 0;
#define LOGO_SCROLL_MS 2000

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println(F("\n╔══════════════════════════╗"));
  Serial.println(F("  ║        TRANSPORTER       ║"));
  Serial.println(F("  ╚══════════════════════════╝"));

  // 1. Motor
  motorSetup();

  // 2. Servo, Lift, Fan
  servoLiftSetup();

  // 3. Load semua config dari flash
  loadMotorConfig();
  loadServoConfig();   // juga load liftConfig & fanConfig
  loadDisplayConfig();

  // 4. WiFi + Web Server
  webServerSetup();

  // 5. PS3 (load MAC dari flash, lalu Ps3.begin)
  ps3Setup();

  // 6. Display
  if (!displaySetup()) {
    Serial.println(F("[DISPLAY] Init failed!"));
  }

  Serial.println(F("[BOOT] Ready!"));
}

// ============================================================
//  LOOP — ringan, tidak ada blocking
// ============================================================
void loop() {

  // ── Web server request (non-blocking) ────────────────────
  webServerHandle();

  // ── Pending restart (dari WebServer: save MAC / reset) ───
  if (pendingRestart) {
    Serial.println(F("[SYS] Restarting..."));
    delay(500);  // beri waktu response HTTP terkirim
    ESP.restart();
  }

  // ── Signal timeout — matikan motor jika PS3 hilang ───────
  if (millis() - lastRecvTime > SIGNAL_TIMEOUT) {
    processCarMovement(STOP, 0);
    liftStop();
    if (!fanConfig.autoMode) ledcWrite(FAN_PIN, 0);
  }

  // ── Logo scroll non-blocking ──────────────────────────────
  // Menggantikan delay(2000)×2 yang sebelumnya memblokir PS3
  if (logoScrollActive) {
    if (millis() - logoScrollTimer >= LOGO_SCROLL_MS) {
      logoScrollTimer = millis();
      if (logoScrollRight) {
        showLogoScrollLeft();   // ganti ke arah kiri
        logoScrollRight = false;
      } else {
        showLogoStop();         // selesai
        logoScrollActive = false;
      }
    }
  }
}
