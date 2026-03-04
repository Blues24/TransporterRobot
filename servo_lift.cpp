#include "servo_lift.h"
#include "config.h"
#include <Preferences.h>

// ============================================================
//  GLOBAL INSTANCES
// ============================================================
static Servo      myServoR;
static Preferences servoPrefs;

ServoConfig servoConfig = { 50, 150, 15 };
LiftConfig  liftConfig  = { 200, 200 };


// ============================================================
//  SETUP
// ============================================================
void servoLiftSetup() {
  // Servo timer allocation
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  // Servo gripper
  myServoR.setPeriodHertz(50);
  myServoR.attach(SERVO_PIN_R, 400, 2500);
  myServoR.write(80);

  // Lifter motor pins (TB6612FNG — IN1, IN2, PWM)
  // STBY dihandle bersama di motor_control.cpp (motorSetup)
  pinMode(PIN_LIFT_IN1, OUTPUT);
  pinMode(PIN_LIFT_IN2, OUTPUT);
  ledcAttachChannel(PIN_LIFT_PWM, PWM_FREQ, PWM_RESOLUTION, PWM_CH_LIFT);
  ledcAttachChannel(BUZZER_PIN, 2000, PWM_RESOLUTION, BUZZER_PWM_CH);
  ledcWrite(BUZZER_PIN, 0);

  liftStop();
}

// ============================================================
//  SERVO GRIPPER
// ============================================================
void servoOpen()  { myServoR.write(servoConfig.openAngle); }
void servoClose() { myServoR.write(servoConfig.closeAngle); }

// ============================================================
//  LIFTER — TB6612FNG
//
//  Sama dengan rotateMotor() tapi untuk lifter.
//  STBY sudah dihandle bersama PIN_MOTOR_STBY di motorSetup().
//  liftStop() pakai BRAKE agar platform tidak melorot.
// ============================================================
void liftUp(int speed) {
  digitalWrite(PIN_LIFT_IN1, HIGH);
  digitalWrite(PIN_LIFT_IN2, LOW);
  ledcWrite(PIN_LIFT_PWM, constrain(speed, 0, 255));
}

void liftDown(int speed) {
  digitalWrite(PIN_LIFT_IN1, LOW);
  digitalWrite(PIN_LIFT_IN2, HIGH);
  ledcWrite(PIN_LIFT_PWM, constrain(speed, 0, 255));
}

void liftStop() {
  // BRAKE — IN1=HIGH, IN2=HIGH, PWM=255
  // Platform tidak melorot karena motor di-short circuit
  digitalWrite(PIN_LIFT_IN1, HIGH);
  digitalWrite(PIN_LIFT_IN2, HIGH);
  ledcWrite(PIN_LIFT_PWM, 255);
}

// ============================================================
//  CONFIG PERSISTENCE
//  (servo + lift + fan digabung satu namespace "servo_cfg")
// ============================================================
void loadServoConfig() {
  servoPrefs.begin("servo_cfg", true);
  servoConfig.openAngle  = servoPrefs.getInt("openAngle",  50);
  servoConfig.closeAngle = servoPrefs.getInt("closeAngle", 150);
  servoConfig.servoSpeed = servoPrefs.getInt("servoSpeed", 15);
  liftConfig.speedUp     = servoPrefs.getInt("liftUp",     200);
  liftConfig.speedDown   = servoPrefs.getInt("liftDown",   200);
  
  servoPrefs.end();
}

void saveServoConfig() {
  servoPrefs.begin("servo_cfg", false);
  servoPrefs.putInt("openAngle",  servoConfig.openAngle);
  servoPrefs.putInt("closeAngle", servoConfig.closeAngle);
  servoPrefs.putInt("servoSpeed", servoConfig.servoSpeed);
  servoPrefs.putInt("liftUp",     liftConfig.speedUp);
  servoPrefs.putInt("liftDown",   liftConfig.speedDown);
  
  servoPrefs.end();
}