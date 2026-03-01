#ifndef SERVO_LIFT_H
#define SERVO_LIFT_H

#include "config.h"
#include <ESP32Servo.h>

void servoLiftSetup();

// Servo gripper
void servoOpen();
void servoClose();

// Lifter (TB6612FNG brake mode — platform tidak melorot saat stop)
void liftUp(int speed);
void liftDown(int speed);
void liftStop();   // BRAKE: IN1=HIGH, IN2=HIGH, PWM=255

// Config persistence (servo + lift + fan dalam satu namespace)
void loadServoConfig();
void saveServoConfig();

#endif // SERVO_LIFT_H