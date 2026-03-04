#ifndef MOTOR_CONF_H
#define MOTOR_CONF_H

#include "config.h"

// Motor Init
void motorSetup();
void motorStandby(bool active);
// Kontrol Motor Utama

/**
 * @brief Putar satu motor dengan kecepatan tertentu.
 * @param motorNumber  Index motor (FRONT_RIGHT_MOTOR, dll)
 * @param motorSpeed   Positif = maju, Negatif = mundur, 0 = stop
 */
 void rotateMotor(int motorNumber, int motorSpeed);

 /**
 * @brief Proses gerakan mobil berdasarkan arah dan kecepatan.
 * @param direction  Konstanta arah (FORWARD, BACKWARD, dll)
 * @param motorSpeed Kecepatan 0-255
 */
 void processCarMovement(int direction, int motorSpeed);

#endif  // MOTOR_CONF_H