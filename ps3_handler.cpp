#include "ps3_handler.h"
#include "motor_conf.h"
#include "servo_lift.h"
#include "display.h"
#include <Preferences.h>

// ============================================================
//  GLOBAL CONFIG & STATE
// ============================================================
Ps3Config       ps3Config    = { "2c:81:58:2f:26:a9" };
volatile bool   eStopActive  = false;  // Emergency Stop state

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
//
//  Trigger : L3 + R3 ditekan bersamaan
//  Resume  : stick analog digerakkan (threshold > 20)
//
//  Saat E-Stop aktif:
//    - motorStandby(false) → STBY LOW → semua motor + lifter mati
//    - Servo & fan tidak terpengaruh (tidak terhubung ke STBY)
//    - Display tampilkan peringatan
//    - Semua input PS3 diabaikan kecuali deteksi resume
// ============================================================
static void triggerEStop() {
  if (eStopActive) return;  // sudah aktif, jangan trigger ulang
  eStopActive = true;
  motorStandby(false);      // STBY LOW — semua motor + lifter mati seketika
  liftStop();               // pastikan lifter IN1/IN2 juga dalam state brake
  ledcWrite(PIN_FAN, 0);    // matikan fan juga
  Serial.println("[ESTOP] ⚠ Emergency Stop AKTIF");
  showEStop(true);          // tampilkan peringatan di OLED
}

static void checkEStopResume() {
  if (!eStopActive) return;

  // Cek apakah ada input stick analog yang signifikan
  const int lx = Ps3.data.analog.stick.lx;
  const int ly = Ps3.data.analog.stick.ly;
  const int rx = Ps3.data.analog.stick.rx;
  const int ry = Ps3.data.analog.stick.ry;

  // Threshold 20 untuk hindari drift stick
  const bool stickMoved = abs(lx) > 20 || abs(ly) > 20 ||
                          abs(rx) > 20 || abs(ry) > 20;

  if (stickMoved) {
    eStopActive = false;
    motorStandby(true);     // STBY HIGH — motor aktif kembali
    Serial.println("[ESTOP] ✓ Resume — motor aktif");
    showEStop(false);       // hapus peringatan dari OLED
  }
}

// ============================================================
//  SUB-HANDLER: Display & tombol aksi
// ============================================================
static void handleButtons() {
  // Servo gripper
  if      (Ps3.data.button.cross)  servoClose();
  else if (Ps3.data.button.square) servoOpen();

  // Fan (manual mode)
  if (!fanConfig.autoMode) {
    ledcWrite(PIN_FAN, Ps3.data.button.circle ? fanConfig.pwmValue : 0);
  }

  // Display — event-based (single press via button_down)
  if (Ps3.data.button.triangle)      showMemberTeam();
  if (Ps3.event.button_down.ps)      { int lv; String msg; getBattInfo(Ps3.data.status.battery, lv, msg); showBattStatus(lv, msg); }
  if (Ps3.event.button_down.start)   showNameTeam();
  if (Ps3.event.button_down.select)  {
    showLogoTeam();
    extern volatile bool logoScrollActive;
    extern unsigned long logoScrollTimer;
    extern bool          logoScrollRight;
    logoScrollActive = true;
    logoScrollRight  = true;
    logoScrollTimer  = millis();
  }
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

  // Kalkulasi kecepatan motor
  int speed = motorConfigs[0].baseSpeed;
  if (r2 > 0) speed += (motorConfigs[0].maxSpeed - speed) * r2 / 255;
  if (l2 > 0) speed -= (speed - motorConfigs[0].minSpeed) * l2 / 255;
  speed = constrain(speed, motorConfigs[0].minSpeed, motorConfigs[0].maxSpeed);

  // Tentukan arah
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

  // Offset kecepatan per manuver
  int s = speed;
  if (dir == RIGHT_2  || dir == LEFT_2)              s = speed - 40;
  if (dir == TURN_LEFT || dir == TURN_RIGHT)          s = speed - 25;
  if (dir == SPIN_LEFT || dir == SPIN_RIGHT)          s = speed - 30;

  processCarMovement(dir, s);

  // Fan auto mode
  robotMoving = (dir != STOP);
  if (fanConfig.autoMode) {
    ledcWrite(PIN_FAN, robotMoving ? fanConfig.pwmValue : 0);
  }
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
  // ── Cek Emergency Stop trigger (L3 + R3) ─────────────────
  const bool l3 = Ps3.data.button.l3;
  const bool r3 = Ps3.data.button.r3;

  if (l3 && r3) {
    triggerEStop();
    lastRecvTime = millis();
    return;  // abaikan semua input lain saat E-Stop di-trigger
  }

  // ── Jika E-Stop sedang aktif: cek resume, lalu return ────
  if (eStopActive) {
    checkEStopResume();
    lastRecvTime = millis();
    return;  // blokir semua input motor/lift selama E-Stop
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
  // Matikan semua via STBY — paling cepat dan paling aman
  motorStandby(false);
  liftStop();
  ledcWrite(PIN_FAN, 0);
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