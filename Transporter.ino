/**
 * ╔══════════════════════════════════════════════════════╗
 * ║           TRANSPORTER - Main Sketch          ║
 * ║                   POLINDRA 2024                      ║
 * ╠══════════════════════════════════════════════════════╣
 * ║  Modul :                                             ║
 * ║   config.h          - Semua struct & konstanta       ║
 * ║   motor_control     - rotateMotor, processCarMovement║
 * ║   servo_lift        - servo gripper, lift, fan       ║
 * ║   display      - OLED display, custom logo      ║
 * ║   web_server_mgr    - WiFi AP, WebServer dashboard   ║
 * ║   ps3_handler       - PS3 input & button binding     ║
 * ╚══════════════════════════════════════════════════════╝
 */


#include "config.h"
#include "motor_conf.h"
#include "servo_lift.h"
#include "display.h"    
#include "web_server_mgr.h"
#include "ps3_handler.h"

// ============================================================
//  GLOBAL STATE
// ============================================================
unsigned long lastRecvTime   = 0;
volatile bool pendingRestart = false;


// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println(F("\n╔══════════════════════════╗"));
  Serial.println(F("║      TRANSPORTER           ║"));
  Serial.println(F("╚══════════════════════════╝"));

  // 1. Motor (TB6612FNG — setup pin + STBY HIGH)
  motorSetup();

  // 2. Servo, Lift, Fan
  servoLiftSetup();

  // 3. Load config dari flash
  loadMotorConfig();
  loadServoConfig();
  loadDisplayConfig();         // ⏳ uncomment jika display sudah siap

  // 4. WiFi + Web Server
  //    Harus sebelum displaySetup agar IP sudah tersedia
  webServerSetup();

  // 5. PS3
  ps3Setup();

  // 6. Display — IP diambil dari WiFi AP setelah webServerSetup()
   String ip = WiFi.softAPIP().toString();
   if (!displaySetup(ip)) {     // ⏳ uncomment jika display sudah siap
     Serial.println(F("[DISPLAY] Init failed!"));
   }

  Serial.println(F("[BOOT] Ready!"));
}

// ============================================================
//  LOOP
// ============================================================
void loop() {

  // Web server (non-blocking)
  webServerHandle();

  // Pending restart dari WebServer (misal: save MAC address)
  if (pendingRestart) {
    Serial.println(F("[SYS] Restarting..."));
    delay(500);   // beri waktu HTTP response terkirim
    ESP.restart();
  }

  // Signal timeout — matikan motor jika PS3 tidak merespons
  if (millis() - lastRecvTime > SIGNAL_TIMEOUT) {
    processCarMovement(STOP, 0);
    liftStop();
  }
}
