#include "ps3_handler.h"
#include "motor_conf.h"
#include "servo_lift.h"
#include "display.h"  
#include <Preferences.h>

// ============================================================
//  GLOBAL CONFIG & STATE
// ============================================================
Ps3Config       ps3Config   = { "2c:81:58:2f:26:a9" };
volatile bool   eStopActive = false;

static Preferences ps3Prefs;

// ============================================================
//  CONFIG PERSISTENCE
// ============================================================
void loadPs3Config() {
  ps3Prefs.begin("ps3_cfg", true);
  String mac = ps3Prefs.getString("mac", ps3Config.macAddress);
  mac.toCharArray(ps3Config.macAddress, sizeof(ps3Config.macAddress));
  ps3Prefs.end();
  Serial.print("[PS3] MAC: ");
  Serial.println(ps3Config.macAddress);
}

void savePs3Config() {
  ps3Prefs.begin("ps3_cfg", false);
  ps3Prefs.putString("mac", ps3Config.macAddress);
  ps3Prefs.end();
}

// ============================================================
//  HELPER: batt status → level + message
// ============================================================
static void getBattInfo(int status, int& level, String& msg) {
  switch (status) {
    case ps3_status_battery_charging: level = 100; msg = "Charging"; break;
    case ps3_status_battery_full:     level = 100; msg = "FULL";     break;
    case ps3_status_battery_high:     level = 75;  msg = "HIGH";     break;
    case ps3_status_battery_low:      level = 25;  msg = "LOW";      break;
    case ps3_status_battery_dying:    level = 10;  msg = "DYING";    break;
    case ps3_status_battery_shutdown: level = 0;   msg = "SHUTDOWN"; break;
    default:                          level = 0;   msg = "UNKNOWN";  break;
  }
}

// ============================================================
//  EMERGENCY STOP
// ============================================================
static void triggerEStop() {
  if (eStopActive) return;
  eStopActive = true;
  motorStandby(false);    // STBY LOW — semua motor + lifter mati
  liftStop();
  Serial.println("[ESTOP] ⚠ Emergency Stop AKTIF");
  showEStop(true);     // ⏳ uncomment jika display sudah siap
}

static void checkEStopResume() {
  if (!eStopActive) return;

  const int lx = Ps3.data.analog.stick.lx;
  const int ly = Ps3.data.analog.stick.ly;
  const int rx = Ps3.data.analog.stick.rx;
  const int ry = Ps3.data.analog.stick.ry;

  // Threshold 20 untuk hindari drift stick
  const bool stickMoved = abs(lx) > 20 || abs(ly) > 20 ||
                          abs(rx) > 20 || abs(ry) > 20;
  if (stickMoved) {
    eStopActive = false;
    motorStandby(true);   // STBY HIGH — motor aktif kembali
    Serial.println("[ESTOP] ✓ Resume — motor aktif");
    showEStop(false);  // ⏳ uncomment jika display sudah siap
  }
}


// ============================================================
//  BUZZER
//  Aktifkan setelah BUZZER_PIN ditentukan di config.h:
//  1. Ganti XX di #define BUZZER_PIN
//  2. Uncomment ledcAttachChannel di servo_lift.cpp
//  3. Uncomment fungsi di bawah ini
// ============================================================
static void buzzerBeep(int durationMs = 100) {
  ledcWrite(BUZZER_PIN, 128);       // 50% duty cycle = volume penuh
  delay(durationMs);                // ⚠️ blocking — ganti millis() jika perlu
  ledcWrite(BUZZER_PIN, 0);
}

static void buzzerOff() {
  ledcWrite(BUZZER_PIN, 0);
}

// ============================================================
//  SUB-HANDLER: Tombol aksi
// ============================================================
static void handleButtons() {
  // Servo gripper
  if      (Ps3.data.button.cross)  servoClose();
  else if (Ps3.data.button.square) servoOpen();

  // Buzzer — △ (Triangle)
  if (Ps3.event.button_down.triangle) buzzerBeep(100);  // beep 100ms saat tekan
  
  if (Ps3.event.button_up.triangle)   buzzerOff();      // matikan saat lepas

  if (Ps3.event.button_down.ps) {
     int lv; String msg;
     getBattInfo(Ps3.data.status.battery, lv, msg);
     showBattStatus(lv, msg);
  }
  if (Ps3.event.button_down.select) showLogo();  // static, tidak ada scroll

}

// ============================================================
//  SUB-HANDLER: Gerak robot
// ============================================================
static void handleMovement() {
  const int lx = Ps3.data.analog.stick.lx;
  const int ly = Ps3.data.analog.stick.ly;
  const int rx = Ps3.data.analog.stick.rx;
  const int ry = Ps3.data.analog.stick.ry;
  const int r2 = Ps3.data.analog.button.r2;
  const int l2 = Ps3.data.analog.button.l2;

  int speed = motorConfigs[0].baseSpeed;
  if (r2 > 0) speed += (motorConfigs[0].maxSpeed - speed) * r2 / 255;
  if (l2 > 0) speed -= (speed - motorConfigs[0].minSpeed) * l2 / 255;
  speed = constrain(speed, motorConfigs[0].minSpeed, motorConfigs[0].maxSpeed);

  int dir = STOP;
  if      (ry > -63 && ry <  63 && rx < -60) dir = RIGHT_2;
  else if (ry > -63 && ry <  63 && rx >  60) dir = LEFT_2;
  else if (lx < -63 && ly < -63)             dir = FORWARD_LEFT;
  else if (lx >  63 && ly < -63)             dir = FORWARD_RIGHT;
  else if (rx < -63 && ry < -63)             dir = TURN_LEFT;
  else if (rx >  63 && ry < -63)             dir = TURN_RIGHT;
  else if (ly >  63 && lx < -63)             dir = BACKWARD_LEFT;
  else if (ly >  63 && lx >  63)             dir = BACKWARD_RIGHT;
  else if (lx > -63 && lx <  63 && ly >  60) dir = BACKWARD;
  else if (lx > -63 && lx <  63 && ly < -60) dir = FORWARD;
  else if (ly > -63 && ly <  63 && lx < -60) dir = SPIN_RIGHT;
  else if (ly > -63 && ly <  63 && lx >  60) dir = SPIN_LEFT;
  else if (Ps3.data.button.up)               dir = FORWARD;
  else if (Ps3.data.button.down)             dir = BACKWARD;
  else if (Ps3.data.button.right)            dir = SPIN_LEFT;
  else if (Ps3.data.button.left)             dir = SPIN_RIGHT;

  int s = speed;
  if (dir == RIGHT_2   || dir == LEFT_2)           s = speed - 40;
  if (dir == TURN_LEFT || dir == TURN_RIGHT)        s = speed - 25;
  if (dir == SPIN_LEFT || dir == SPIN_RIGHT)        s = speed - 30;

  processCarMovement(dir, s);

}

// ============================================================
//  SUB-HANDLER: Lift
// ============================================================
static void handleLift() {
  if      (Ps3.data.analog.button.r1 > 0) liftUp(liftConfig.speedUp);
  else if (Ps3.data.analog.button.l1 > 0) liftDown(liftConfig.speedDown);
  else                                      liftStop();
}

// ============================================================
//  MAIN NOTIFY CALLBACK
// ============================================================
void ps3Notify() {
  // ── Emergency Stop: L3 + R3 ──────────────────────────────
  if (Ps3.data.button.l3 && Ps3.data.button.r3) {
    triggerEStop();
    lastRecvTime = millis();
    return;
  }

  // ── Jika E-Stop aktif: hanya cek resume ──────────────────
  if (eStopActive) {
    checkEStopResume();
    lastRecvTime = millis();
    return;
  }

  // ── Normal operation ──────────────────────────────────────
  handleButtons();
  handleMovement();
  handleLift();
  lastRecvTime = millis();
}

// ============================================================
//  CONNECT / DISCONNECT
// ============================================================
void ps3OnConnect() {
  eStopActive = false;
  motorStandby(true);
  Serial.println("[PS3] Connected.");
}

void ps3OnDisconnect() {
  motorStandby(false);
  liftStop();
  Serial.println("[PS3] Disconnected. Restarting...");
  ESP.restart();
}

// ============================================================
//  SETUP
// ============================================================
void ps3Setup() {
  loadPs3Config();
  Ps3.attach(ps3Notify);
  Ps3.attachOnConnect(ps3OnConnect);
  Ps3.attachOnDisconnect(ps3OnDisconnect);
  Ps3.begin(ps3Config.macAddress);
  Serial.print("[PS3] Listening for: ");
  Serial.println(ps3Config.macAddress);
}