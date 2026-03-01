#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <vector>

// Motor Movement Direction Constant
#define STOP           0
#define FORWARD        1
#define BACKWARD       2
#define LEFT_1         3
#define RIGHT_1        4
#define LEFT_2         5
#define RIGHT_2        6
#define FORWARD_LEFT   7
#define FORWARD_RIGHT  8
#define BACKWARD_LEFT  9
#define BACKWARD_RIGHT 10
#define SPIN_LEFT      11
#define SPIN_RIGHT     12
#define TURN_LEFT      13
#define TURN_RIGHT     14


// Motor Index
#define BACK_RIGHT_MOTOR 0
#define BACK_LEFT_MOTOR 1
#define FRONT_RIGHT_MOTOR 2
#define FRONT_LEFT_MOTOR 3

// PWM Settings
#define PWM_FREQ          5000
#define PWM_RESOLUTION    8
#define MAX_MOTOR_SPEED   255
#define MIN_MOTOR_SPEED   120

// Pinout for Lifter and Servo
#define LIFT_IN1      3    
#define LIFT_IN2      1
#define LIFT_PWM      23

#define SERVO_PIN_R   33

// Shared Pin
#define PIN_MOTOR_STBY 2

// Pin IC 1
#define PIN_FR_IN1    17
#define PIN_FR_IN2    16
#define PIN_FR_PWM    4

#define PIN_FL_IN1    5
#define PIN_FL_IN2    18
#define PIN_FL_PWM    19

// Pin IC 2
#define PIN_BR_IN1    33
#define PIN_BR_IN2    25
#define PIN_BR_PWM    32

#define PIN_BL_IN1    26
#define PIN_BL_IN2    27
#define PIN_BL_PWM    14

// Motor Pin Struct
struct MOTOR_PINS {
  int pinIN1;   // arah a
  int pinIN2;   // arah b
  int pinPWM;   // speed (PWM)
  int pwmCh;    // ESP32 ledc channel 
};

extern std::vector<MOTOR_PINS> motorPins;


// Config Structs
// Config untuk semua pengaturan yang bisa dilakukan oleh webserver
struct MotorConfig {
  int baseSpeed;
  int minSpeed;
  int maxSpeed;
};

struct DisplayConfig {
  int brightness;
  int contrast;
  int displayMode;    // 0=normal, 1=dark, 2=minimal
  int displayTimeout; // menggunakan satuan milisecond (ms)
  int fontSize;       // 1=small, 2=medium, 3=large
  int updateRate;     // jarak untuk update berupa ms
  bool customLogo;    // true = pakai bitmap dari preferences, false = menggunakan default
};

struct ServoConfig {
  int openAngle;    // Derajat ketika servo membuka
  int closeAngle;   // Derajat ketika servo menutup
  int servoSpeed;   // ms per derajat
};

struct LiftConfig {
  int speedUp;        // 0-255
  int speedDown;      // 0-255
};

struct Ps3Config {
  char macAddress[18]; // contoh dari penulisan MAC ADDRESS "xx:xx:xx:xx:xx:xx\0" 
                       //untuk \0 itu adalah null terminator untuk definisi cari sendiri di google
};

// Global Config Instance (extern)
extern MotorConfig    motorConfigs[4];
extern DisplayConfig  displayConfig;
extern ServoConfig    servoConfig;
extern LiftConfig     liftConfig;
extern Ps3Config      ps3Config;

// Runtime State
#define SIGNAL_TIMEOUT 1000 // ms atau 1 detik
extern unsigned long lastRecvTime;
extern volatile bool pendingRestart;

#endif // CONFIG_H