#include "motor_conf.h"

// Definisi Global 

std::vector<MOTOR_PINS> motorPins = {
  { PIN_FR_IN1, PIN_FR_IN2, PIN_FR_PWM, PWM_CH_FR },  // [0] FRONT_RIGHT
  { PIN_FL_IN1, PIN_FL_IN2, PIN_FL_PWM, PWM_CH_FL },  // [1] FRONT_LEFT
  { PIN_BR_IN1, PIN_BR_IN2, PIN_BR_PWM, PWM_CH_BR },  // [2] BACK_RIGHT
  { PIN_BL_IN1, PIN_BL_IN2, PIN_BL_PWM, PWM_CH_BL },  // [3] BACK_LEFT
};

MotorConfig motorConfigs[4] = {
  {210, 140, 255},  // [0] FRONT_RIGHT
  {210, 140, 255},  // [1] FRONT_LEFT
  {210, 140, 255},  // [2] BACK_RIGHT
  {210, 140, 255},  // [3] BACK_LEFT
};

// Setup or init
void motorSetup(){
  // Aktifkan STBY pin 
  pinMode(PIN_MOTOR_STBY, OUTPUT);
  digitalWrite(PIN_MOTOR_STBY, HIGH);

  // setup semua motor
  for (size_t i = 0; i < motorPins.size(); i++){
    pinMode(motorPins[i].pinIN1, OUTPUT);
    pinMode(motorPins[i].pinIN2, OUTPUT);
    ledAttachChannel(
                     motorPins[i].pinPWM,
                     PWM_FREQ,
                     PWM_RESOLUTION
                     motorPins.pwmCh
                     );
    // Stop motor
    rotateMotor(i, 0);
  }
}

void motorStandby(bool active) {
  digitalWrite(PIN_MOTOR_STBY, active ? HIGH : LOW);
}

// ============================================================
//  ROTATE MOTOR — TB6612FNG
//
//  Tabel kebenaran TB6612FNG:
//  ┌──────┬──────┬──────┬─────────────────┐
//  │ IN1  │ IN2  │ PWM  │ Output          │
//  ├──────┼──────┼──────┼─────────────────┤
//  │ HIGH │ LOW  │ PWM  │ Maju (CW)       │
//  │ LOW  │ HIGH │ PWM  │ Mundur (CCW)    │
//  │ HIGH │ HIGH │ PWM  │ Brake (short)   │ ← STOP kita
//  │ LOW  │ LOW  │ x    │ Coast (free)    │
//  └──────┴──────┴──────┴─────────────────┘
// ============================================================

void rotateMotor(int motorNumber, int motorSpeed) {
  const MOTOR_PINS& m = motorPins[motorNumber];

  if (motorSpeed > 0) {
    // Maju
    digitalWrite(m.pinIN1, HIGH);
    digitalWrite(m.pinIN2, LOW);
    ledcWrite(m.pinPWM, abs(motorSpeed));
  } else if (motorSpeed < 0) {
    // Mundur
    digitalWrite(m.pinIN1, LOW);
    digitalWrite(m.pinIN2, HIGH);
    ledcWrite(m.pinPWM, abs(motorSpeed));
  } else {
    // Rem
    digitalWrite(m.pinIN1, HIGH);
    digitalWrite(m.pinIN2, HIGH);
    ledcWrite(m.pinPWM, 255); // PWM max saat brake = pengereman instant dan maksimal
  }
}

// Process Robot movement
void processCarMovement(int direction, int speed){
  switch (direction) {
    
    case FORWARD:
      rotateMotor(FRONT_RIGHT_MOTOR, speed);
      rotateMotor(FRONT_LEFT_MOTOR,  speed);
      rotateMotor(BACK_RIGHT_MOTOR,  speed);
      rotateMotor(BACK_LEFT_MOTOR,   speed);
      break;
    
    case BACKWARD:
      rotateMotor(FRONT_RIGHT_MOTOR, -speed);
      rotateMotor(FRONT_LEFT_MOTOR,  -speed);
      rotateMotor(BACK_RIGHT_MOTOR,  -speed);
      rotateMotor(BACK_LEFT_MOTOR,   -speed);
      break;

    // Strafe kiri(hanya ada di roda mecanum)
    case LEFT_1:
    case LEFT_2:
      rotateMotor(FRONT_RIGHT_MOTOR, speed);
      rotateMotor(FRONT_LEFT_MOTOR,  -speed);
      rotateMotor(BACK_RIGHT_MOTOR,  -speed);
      rotateMotor(BACK_LEFT_MOTOR,   speed);
      break;

    case FORWARD_RIGHT:
      rotateMotor(FRONT_RIGHT_MOTOR, STOP);
      rotateMotor(FRONT_LEFT_MOTOR,  speed);
      rotateMotor(BACK_RIGHT_MOTOR,  speed);
      rotateMotor(BACK_LEFT_MOTOR,   STOP);
      break;
    
    case FORWARD_LEFT:
      rotateMotor(FRONT_RIGHT_MOTOR, speed);
      rotateMotor(FRONT_LEFT_MOTOR,  STOP);
      rotateMotor(BACK_RIGHT_MOTOR,  STOP);
      rotateMotor(BACK_LEFT_MOTOR,   speed);
      break;
    
    case BACKWARD_RIGHT:
      rotateMotor(FRONT_RIGHT_MOTOR, STOP);
      rotateMotor(FRONT_LEFT_MOTOR,  -speed);
      rotateMotor(BACK_RIGHT_MOTOR,  -speed);
      rotateMotor(BACK_LEFT_MOTOR,   STOP);
      break; 

    case SPIN_LEFT:
      rotateMotor(FRONT_RIGHT_MOTOR, speed);
      rotateMotor(FRONT_LEFT_MOTOR,  -speed);
      rotateMotor(BACK_RIGHT_MOTOR,  speed);
      rotateMotor(BACK_LEFT_MOTOR,   -speed);
      break;
    
    case SPIN_RIGHT:
      rotateMotor(FRONT_RIGHT_MOTOR, -speed);
      rotateMotor(FRONT_LEFT_MOTOR,  speed);
      rotateMotor(BACK_RIGHT_MOTOR,  -speed);
      rotateMotor(BACK_LEFT_MOTOR,   speed);
      break;
    
    // -- Drift / arc turn
    case TURN_LEFT:
      rotateMotor(FRONT_RIGHT_MOTOR, speed);
      rotateMotor(FRONT_LEFT_MOTOR,  -speed / 3);
      rotateMotor(BACK_RIGHT_MOTOR,  speed);
      rotateMotor(BACK_LEFT_MOTOR,   -speed / 3);
      break;

    case TURN_RIGHT:
      rotateMotor(FRONT_RIGHT_MOTOR, speed);
      rotateMotor(FRONT_LEFT_MOTOR,  -speed / 3);
      rotateMotor(BACK_RIGHT_MOTOR,  speed);
      rotateMotor(BACK_LEFT_MOTOR,   -speed / 3);
      break;
    
    default: // Stop
      rotateMotor(FRONT_RIGHT_MOTOR, STOP);
      rotateMotor(FRONT_LEFT_MOTOR,  STOP);
      rotateMotor(BACK_RIGHT_MOTOR,  STOP);
      rotateMotor(BACK_LEFT_MOTOR,   STOP);
      break;
  } 
}