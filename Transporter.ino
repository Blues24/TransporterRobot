#include <Arduino.h>
#include <Ps3Controller.h>
#include <ESP32Servo.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>

// WiFi Configuration
const char* ssid = "WIRAGORA_2024";
const char* password = "wiragora2024";
WebServer server(80);
Preferences preferences;

// Motor Speed Configuration - Individual for each motor
struct MotorConfig {
  int baseSpeed;
  int minSpeed;
  int maxSpeed;
};

MotorConfig motorConfigs[4] = {
  {210, 140, 255},  // BACK_RIGHT_MOTOR
  {210, 140, 255},  // BACK_LEFT_MOTOR
  {210, 140, 255},  // FRONT_LEFT_MOTOR
  {210, 140, 255}   // FRONT_RIGHT_MOTOR
};

// Display Configuration
struct DisplayConfig {
  int brightness;           // 0-255
  int contrast;             // 0-255
  int displayMode;          // 0=normal, 1=dark, 2=minimal
  int displayTimeout;       // milliseconds
  int fontSize;             // 1=small, 2=medium, 3=large
  int updateRate;           // milliseconds between updates
};

DisplayConfig displayConfig = {
  255,      // brightness
  255,      // contrast
  0,        // displayMode (normal)
  5000,     // displayTimeout
  2,        // fontSize (medium)
  500       // updateRate
};

// Servo Configuration
struct ServoConfig {
  int openAngle;            // angle when open
  int closeAngle;           // angle when closed
  int servoSpeed;           // speed of servo movement (ms per degree)
};

ServoConfig servoConfig = {
  50,       // openAngle
  150,      // closeAngle
  15        // servoSpeed
};

#define FORWARD 1
#define BACKWARD 2
#define LEFT_1 3
#define RIGHT_1 4
#define LEFT_2 5
#define RIGHT_2 6
#define FORWARD_LEFT 7
#define FORWARD_RIGHT 8
#define BACKWARD_LEFT 9
#define BACKWARD_RIGHT 10
#define SPIN_LEFT 11
#define SPIN_RIGHT 12
#define TURN_LEFT 13
#define TURN_RIGHT 14
#define LIFT_UP 15
#define LIFT_DOWN 16
#define STOP 0

#define BACK_RIGHT_MOTOR 0
#define BACK_LEFT_MOTOR 1
#define FRONT_RIGHT_MOTOR 2
#define FRONT_LEFT_MOTOR 3

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define SSD1306_BLUE 2
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int LiftIN1 = 19;
const int LiftIN2 = 5;
const int LiftENA = 18;

const int FanPin = 32;

// Servo myServoL;
Servo myServoR;

// const int servoPinL = 32;  // gripper kiri
const int servoPinR = 33;  // gripper kanan

const int minp = 400;
const int maxp = 2500;

bool servoOpenR = false;

struct MOTOR_PINS {
  int pinIN1;
  int pinIN2;
  int pinEn;
};

std::vector<MOTOR_PINS> motorPins = {
  { 13, 14, 12 },  //BACK_RIGHT_MOTOR
  { 27, 25, 26 },  //BACK_LEFT_MOTOR
  { 0, 15, 2 },    //FRONT_LEFT_MOTOR
  { 4, 17, 16 },   //FRONT_RIGHT_MOTOR
};

#define MAX_MOTOR_SPEED 255
#define MIN_MOTOR_SPEED 140
int BASE_MOTOR_SPEED = 210;



const int PWMFreq = 5000;  // mengatur frequensi pwm output untuk motor menjadi 2kHz
const int PWMResolution = 8;


#define SIGNAL_TIMEOUT 1000  // This is signal timeout in milli seconds. We will reset the data if no signal
unsigned long lastRecvTime = 0;

ESP32PWM pwm;

const unsigned char epd_bitmap_Logo_Polindra [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xc0, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xfe, 0x00, 0x00, 0x1f, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xfc, 0x00, 0x00, 0x0f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xf0, 0x00, 0x00, 0x0f, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xe0, 0x00, 0x00, 0x0f, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xc0, 0x1f, 0xe0, 0x03, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xc0, 0x7f, 0xfc, 0x01, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x80, 0xff, 0xfe, 0x00, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x01, 0xff, 0xff, 0x00, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x03, 0xff, 0xff, 0x80, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x07, 0xff, 0xff, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x07, 0xff, 0xff, 0x40, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x0f, 0xff, 0xfe, 0xe0, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x0f, 0xff, 0xfd, 0xe0, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x0f, 0xff, 0xff, 0xe0, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x1f, 0xff, 0xfb, 0xc0, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x1f, 0xff, 0xf7, 0xc0, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x1f, 0xff, 0xef, 0x90, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x1f, 0xff, 0xff, 0x70, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x0f, 0xff, 0x7e, 0xf0, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x0f, 0xfe, 0xfd, 0xe0, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x0f, 0xfd, 0xfb, 0xc0, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x07, 0xf7, 0xe7, 0x80, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x07, 0xdf, 0xdf, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0x00, 0x3f, 0xbe, 0x01, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0x00, 0xfe, 0x7c, 0x01, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x3e, 0x7f, 0x00, 0xf9, 0xf8, 0x03, 0xff, 0x3c, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x38, 0x7f, 0x80, 0x03, 0xe0, 0x07, 0xff, 0x9c, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x1e, 0x7f, 0xc0, 0x0f, 0x80, 0x07, 0xfe, 0x18, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x1e, 0x3f, 0xc0, 0x00, 0x00, 0x0f, 0xfe, 0xf8, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x1e, 0xbf, 0xe0, 0x00, 0x00, 0x1f, 0xfc, 0x78, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x0e, 0x1f, 0xf0, 0x00, 0x00, 0x7f, 0xf8, 0xf0, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x0f, 0x3f, 0xf8, 0x00, 0x00, 0xff, 0xf8, 0xf0, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x07, 0xe7, 0xff, 0xc0, 0x07, 0xff, 0xf1, 0xe0, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x03, 0x93, 0xff, 0xff, 0xff, 0xff, 0xe1, 0xc0, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x03, 0xe5, 0xff, 0xff, 0xff, 0xff, 0xe3, 0xc0, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x01, 0xea, 0xff, 0xff, 0xff, 0xff, 0x87, 0x80, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0xf4, 0x7f, 0xff, 0xff, 0xfe, 0x5f, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x7a, 0x1f, 0xff, 0xff, 0xfc, 0x5e, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3d, 0x57, 0xff, 0xff, 0xf4, 0xbc, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x03, 0xff, 0xff, 0xa5, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xc0, 0xbf, 0xfd, 0x81, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xe1, 0xa2, 0x00, 0x97, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xfd, 0x81, 0x02, 0x9f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xd5, 0x34, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xf2, 0x4f, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Load motor configuration from Preferences
void loadMotorConfig() {
  preferences.begin("motor_config", false);
  
  for (int i = 0; i < 4; i++) {
    String baseKey = "motor_" + String(i) + "_base";
    String minKey = "motor_" + String(i) + "_min";
    String maxKey = "motor_" + String(i) + "_max";
    
    motorConfigs[i].baseSpeed = preferences.getInt(baseKey.c_str(), motorConfigs[i].baseSpeed);
    motorConfigs[i].minSpeed = preferences.getInt(minKey.c_str(), motorConfigs[i].minSpeed);
    motorConfigs[i].maxSpeed = preferences.getInt(maxKey.c_str(), motorConfigs[i].maxSpeed);
  }
  
  preferences.end();
  Serial.println("Motor config loaded from preferences");
}

// Load display configuration from Preferences
void loadDisplayConfig() {
  preferences.begin("display_config", false);
  
  displayConfig.brightness = preferences.getInt("brightness", 255);
  displayConfig.contrast = preferences.getInt("contrast", 255);
  displayConfig.displayMode = preferences.getInt("displayMode", 0);
  displayConfig.displayTimeout = preferences.getInt("displayTimeout", 5000);
  displayConfig.fontSize = preferences.getInt("fontSize", 2);
  displayConfig.updateRate = preferences.getInt("updateRate", 500);
  
  preferences.end();
  Serial.println("Display config loaded from preferences");
}

// Load servo configuration from Preferences
void loadServoConfig() {
  preferences.begin("servo_config", false);
  
  servoConfig.openAngle = preferences.getInt("openAngle", 50);
  servoConfig.closeAngle = preferences.getInt("closeAngle", 150);
  servoConfig.servoSpeed = preferences.getInt("servoSpeed", 15);
  
  preferences.end();
  Serial.println("Servo config loaded from preferences");
}

// Save motor configuration to Preferences
void saveMotorConfig() {
  preferences.begin("motor_config", false);
  
  for (int i = 0; i < 4; i++) {
    String baseKey = "motor_" + String(i) + "_base";
    String minKey = "motor_" + String(i) + "_min";
    String maxKey = "motor_" + String(i) + "_max";
    
    preferences.putInt(baseKey.c_str(), motorConfigs[i].baseSpeed);
    preferences.putInt(minKey.c_str(), motorConfigs[i].minSpeed);
    preferences.putInt(maxKey.c_str(), motorConfigs[i].maxSpeed);
  }
  
  preferences.end();
  Serial.println("Motor config saved to preferences");
}

// Save display configuration to Preferences
void saveDisplayConfig() {
  preferences.begin("display_config", false);
  
  preferences.putInt("brightness", displayConfig.brightness);
  preferences.putInt("contrast", displayConfig.contrast);
  preferences.putInt("displayMode", displayConfig.displayMode);
  preferences.putInt("displayTimeout", displayConfig.displayTimeout);
  preferences.putInt("fontSize", displayConfig.fontSize);
  preferences.putInt("updateRate", displayConfig.updateRate);
  
  preferences.end();
  Serial.println("Display config saved to preferences");
}

// Save servo configuration to Preferences
void saveServoConfig() {
  preferences.begin("servo_config", false);
  
  preferences.putInt("openAngle", servoConfig.openAngle);
  preferences.putInt("closeAngle", servoConfig.closeAngle);
  preferences.putInt("servoSpeed", servoConfig.servoSpeed);
  
  preferences.end();
  Serial.println("Servo config saved to preferences");
}

// Web Server - Handle root page
void handleRoot() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
  <title>WIRAGORA Robot Config</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: Arial, sans-serif;
      margin: 20px;
      background-color: #f0f0f0;
    }
    .container {
      max-width: 900px;
      margin: 0 auto;
      background-color: white;
      padding: 20px;
      border-radius: 10px;
      box-shadow: 0 0 10px rgba(0,0,0,0.1);
    }
    h1 {
      color: #333;
      text-align: center;
    }
    .section-title {
      color: #fff;
      padding: 10px 15px;
      border-radius: 5px;
      margin-top: 20px;
      margin-bottom: 10px;
      font-weight: bold;
    }
    .section-motor {
      border-left: 4px solid #007bff;
    }
    .section-display {
      border-left: 4px solid #ff9800;
    }
    .section-servo {
      border-left: 4px solid #4caf50;
    }
    .motor-section {
      margin: 10px 0;
      padding: 15px;
      border: 2px solid #007bff;
      border-radius: 5px;
      background-color: #f9f9f9;
    }
    .motor-section h2 {
      color: #007bff;
      margin-top: 0;
    }
    .display-section {
      margin: 10px 0;
      padding: 15px;
      border: 2px solid #ff9800;
      border-radius: 5px;
      background-color: #fef5e7;
    }
    .servo-section {
      margin: 10px 0;
      padding: 15px;
      border: 2px solid #4caf50;
      border-radius: 5px;
      background-color: #f1f8e9;
    }
    .servo-section h3 {
      color: #4caf50;
      margin-top: 0;
    }
    .display-section h3 {
      color: #ff9800;
      margin-top: 0;
    }
    .input-group {
      margin: 10px 0;
      display: flex;
      gap: 20px;
      flex-wrap: wrap;
    }
    .input-item {
      flex: 1;
      min-width: 150px;
    }
    label {
      display: block;
      margin-bottom: 5px;
      font-weight: bold;
      color: #333;
    }
    input, select {
      width: 100%;
      padding: 8px;
      border: 1px solid #ddd;
      border-radius: 4px;
      box-sizing: border-box;
    }
    .slider-value {
      display: inline-block;
      margin-left: 10px;
      font-weight: bold;
      color: #007bff;
    }
    button {
      background-color: #28a745;
      color: white;
      padding: 12px 30px;
      border: none;
      border-radius: 5px;
      cursor: pointer;
      font-size: 16px;
      width: 100%;
      margin-top: 20px;
    }
    button:hover {
      background-color: #218838;
    }
    .status {
      margin-top: 20px;
      padding: 10px;
      border-radius: 5px;
      text-align: center;
      display: none;
    }
    .status.success {
      background-color: #d4edda;
      color: #155724;
      display: block;
    }
    .status.error {
      background-color: #f8d7da;
      color: #721c24;
      display: block;
    }
    .servo-button-group {
      display: flex;
      gap: 10px;
      margin-top: 10px;
    }
    .servo-button {
      flex: 1;
      padding: 10px;
      border: none;
      border-radius: 5px;
      cursor: pointer;
      font-weight: bold;
      color: white;
    }
    .servo-open {
      background-color: #4caf50;
    }
    .servo-open:hover {
      background-color: #45a049;
    }
    .servo-close {
      background-color: #f44336;
    }
    .servo-close:hover {
      background-color: #da190b;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>🤖 WIRAGORA Robot Configuration</h1>
    
    <form id="configForm">
      
      <!-- MOTOR CONFIGURATION -->
      <div class="section-title section-motor">⚙️ MOTOR CONFIGURATION</div>
      
      <div class="motor-section">
        <h2>Back Right Motor</h2>
        <div class="input-group">
          <div class="input-item">
            <label>Base Speed</label>
            <input type="number" name="motor_0_base" min="0" max="255" value="210">
          </div>
          <div class="input-item">
            <label>Min Speed</label>
            <input type="number" name="motor_0_min" min="0" max="255" value="140">
          </div>
          <div class="input-item">
            <label>Max Speed</label>
            <input type="number" name="motor_0_max" min="0" max="255" value="255">
          </div>
        </div>
      </div>

      <div class="motor-section">
        <h2>Back Left Motor</h2>
        <div class="input-group">
          <div class="input-item">
            <label>Base Speed</label>
            <input type="number" name="motor_1_base" min="0" max="255" value="210">
          </div>
          <div class="input-item">
            <label>Min Speed</label>
            <input type="number" name="motor_1_min" min="0" max="255" value="140">
          </div>
          <div class="input-item">
            <label>Max Speed</label>
            <input type="number" name="motor_1_max" min="0" max="255" value="255">
          </div>
        </div>
      </div>

      <div class="motor-section">
        <h2>Front Left Motor</h2>
        <div class="input-group">
          <div class="input-item">
            <label>Base Speed</label>
            <input type="number" name="motor_2_base" min="0" max="255" value="210">
          </div>
          <div class="input-item">
            <label>Min Speed</label>
            <input type="number" name="motor_2_min" min="0" max="255" value="140">
          </div>
          <div class="input-item">
            <label>Max Speed</label>
            <input type="number" name="motor_2_max" min="0" max="255" value="255">
          </div>
        </div>
      </div>

      <div class="motor-section">
        <h2>Front Right Motor</h2>
        <div class="input-group">
          <div class="input-item">
            <label>Base Speed</label>
            <input type="number" name="motor_3_base" min="0" max="255" value="210">
          </div>
          <div class="input-item">
            <label>Min Speed</label>
            <input type="number" name="motor_3_min" min="0" max="255" value="140">
          </div>
          <div class="input-item">
            <label>Max Speed</label>
            <input type="number" name="motor_3_max" min="0" max="255" value="255">
          </div>
        </div>
      </div>

      <!-- DISPLAY CONFIGURATION -->
      <div class="section-title section-display">📱 DISPLAY CONFIGURATION</div>
      
      <div class="display-section">
        <h3>Display Settings</h3>
        <div class="input-group">
          <div class="input-item">
            <label>Brightness <span class="slider-value" id="brightnessValue">255</span></label>
            <input type="range" name="brightness" min="0" max="255" value="255" 
                   oninput="document.getElementById('brightnessValue').textContent = this.value">
          </div>
          <div class="input-item">
            <label>Contrast <span class="slider-value" id="contrastValue">255</span></label>
            <input type="range" name="contrast" min="0" max="255" value="255"
                   oninput="document.getElementById('contrastValue').textContent = this.value">
          </div>
        </div>
        <div class="input-group">
          <div class="input-item">
            <label>Display Mode</label>
            <select name="displayMode">
              <option value="0">Normal</option>
              <option value="1">Dark</option>
              <option value="2">Minimal</option>
            </select>
          </div>
          <div class="input-item">
            <label>Font Size</label>
            <select name="fontSize">
              <option value="1">Small</option>
              <option value="2">Medium</option>
              <option value="3">Large</option>
            </select>
          </div>
        </div>
        <div class="input-group">
          <div class="input-item">
            <label>Display Timeout (ms)</label>
            <input type="number" name="displayTimeout" min="1000" max="30000" step="1000" value="5000">
          </div>
          <div class="input-item">
            <label>Update Rate (ms)</label>
            <input type="number" name="updateRate" min="100" max="5000" step="100" value="500">
          </div>
        </div>
      </div>

      <!-- SERVO CONFIGURATION -->
      <div class="section-title section-servo">🔧 SERVO CONFIGURATION</div>
      
      <div class="servo-section">
        <h3>Gripper Servo Settings</h3>
        <div class="input-group">
          <div class="input-item">
            <label>Open Angle (°)</label>
            <input type="number" name="openAngle" min="0" max="180" value="50">
          </div>
          <div class="input-item">
            <label>Close Angle (°)</label>
            <input type="number" name="closeAngle" min="0" max="180" value="150">
          </div>
          <div class="input-item">
            <label>Servo Speed (ms/°)</label>
            <input type="number" name="servoSpeed" min="5" max="50" value="15">
          </div>
        </div>
        
        <h3>Control Servo</h3>
        <div class="servo-button-group">
          <button type="button" class="servo-button servo-open" onclick="servoOpen()">🔓 OPEN</button>
          <button type="button" class="servo-button servo-close" onclick="servoClose()">🔒 CLOSE</button>
        </div>
      </div>

      <button type="submit">💾 SAVE ALL CONFIGURATION</button>
      <div class="status" id="status"></div>
    </form>
  </div>

  <script>
    document.getElementById('configForm').addEventListener('submit', function(e) {
      e.preventDefault();
      const formData = new FormData(this);
      const data = Object.fromEntries(formData);
      
      // Convert string values to numbers
      Object.keys(data).forEach(key => {
        data[key] = isNaN(data[key]) ? data[key] : parseInt(data[key]);
      });
      
      fetch('/api/config', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify(data)
      })
      .then(response => response.json())
      .then(result => {
        const status = document.getElementById('status');
        if (result.success) {
          status.textContent = '✅ Configuration saved successfully!';
          status.className = 'status success';
        } else {
          status.textContent = '❌ Error: ' + result.message;
          status.className = 'status error';
        }
      })
      .catch(error => {
        const status = document.getElementById('status');
        status.textContent = '❌ Error: ' + error;
        status.className = 'status error';
      });
    });
    
    // Load current values from server
    fetch('/api/config')
      .then(response => response.json())
      .then(data => {
        Object.keys(data).forEach(key => {
          const input = document.querySelector(`[name="${key}"]`);
          if (input) {
            input.value = data[key];
            // Update slider display values
            if (key === 'brightness') document.getElementById('brightnessValue').textContent = data[key];
            if (key === 'contrast') document.getElementById('contrastValue').textContent = data[key];
          }
        });
      });
    
    function servoOpen() {
      fetch('/api/servo/open', {method: 'POST'})
        .then(response => response.json())
        .then(result => {
          const status = document.getElementById('status');
          status.textContent = result.message;
          status.className = result.success ? 'status success' : 'status error';
        });
    }
    
    function servoClose() {
      fetch('/api/servo/close', {method: 'POST'})
        .then(response => response.json())
        .then(result => {
          const status = document.getElementById('status');
          status.textContent = result.message;
          status.className = result.success ? 'status success' : 'status error';
        });
    }
  </script>
</body>
</html>
  )";
  
  server.send(200, "text/html", html);
}

// Web Server - Handle API GET (read config)
void handleConfigGet() {
  String json = "{";
  
  // Motor config
  for (int i = 0; i < 4; i++) {
    if (i > 0) json += ",";
    json += "\"motor_" + String(i) + "_base\":" + motorConfigs[i].baseSpeed;
    json += ",\"motor_" + String(i) + "_min\":" + motorConfigs[i].minSpeed;
    json += ",\"motor_" + String(i) + "_max\":" + motorConfigs[i].maxSpeed;
  }
  
  // Display config
  json += ",\"brightness\":" + String(displayConfig.brightness);
  json += ",\"contrast\":" + String(displayConfig.contrast);
  json += ",\"displayMode\":" + String(displayConfig.displayMode);
  json += ",\"displayTimeout\":" + String(displayConfig.displayTimeout);
  json += ",\"fontSize\":" + String(displayConfig.fontSize);
  json += ",\"updateRate\":" + String(displayConfig.updateRate);
  
  // Servo config
  json += ",\"openAngle\":" + String(servoConfig.openAngle);
  json += ",\"closeAngle\":" + String(servoConfig.closeAngle);
  json += ",\"servoSpeed\":" + String(servoConfig.servoSpeed);
  
  json += "}";
  server.send(200, "application/json", json);
}

// Web Server - Handle API POST (save config)
void handleConfigPost() {
  if (server.hasArg("plain")) {
    String json = server.arg("plain");
    
    // Parse motor config
    for (int i = 0; i < 4; i++) {
      String baseKey = "\"motor_" + String(i) + "_base\":";
      String minKey = "\"motor_" + String(i) + "_min\":";
      String maxKey = "\"motor_" + String(i) + "_max\":";
      
      int baseIdx = json.indexOf(baseKey);
      int minIdx = json.indexOf(minKey);
      int maxIdx = json.indexOf(maxKey);
      
      if (baseIdx != -1) {
        motorConfigs[i].baseSpeed = json.substring(baseIdx + baseKey.length()).toInt();
      }
      if (minIdx != -1) {
        motorConfigs[i].minSpeed = json.substring(minIdx + minKey.length()).toInt();
      }
      if (maxIdx != -1) {
        motorConfigs[i].maxSpeed = json.substring(maxIdx + maxKey.length()).toInt();
      }
    }
    
    // Parse display config
    int brightnessIdx = json.indexOf("\"brightness\":");
    int contrastIdx = json.indexOf("\"contrast\":");
    int displayModeIdx = json.indexOf("\"displayMode\":");
    int displayTimeoutIdx = json.indexOf("\"displayTimeout\":");
    int fontSizeIdx = json.indexOf("\"fontSize\":");
    int updateRateIdx = json.indexOf("\"updateRate\":");
    
    if (brightnessIdx != -1) displayConfig.brightness = json.substring(brightnessIdx + 13).toInt();
    if (contrastIdx != -1) displayConfig.contrast = json.substring(contrastIdx + 11).toInt();
    if (displayModeIdx != -1) displayConfig.displayMode = json.substring(displayModeIdx + 14).toInt();
    if (displayTimeoutIdx != -1) displayConfig.displayTimeout = json.substring(displayTimeoutIdx + 17).toInt();
    if (fontSizeIdx != -1) displayConfig.fontSize = json.substring(fontSizeIdx + 11).toInt();
    if (updateRateIdx != -1) displayConfig.updateRate = json.substring(updateRateIdx + 12).toInt();
    
    // Parse servo config
    int openAngleIdx = json.indexOf("\"openAngle\":");
    int closeAngleIdx = json.indexOf("\"closeAngle\":");
    int servoSpeedIdx = json.indexOf("\"servoSpeed\":");
    
    if (openAngleIdx != -1) servoConfig.openAngle = json.substring(openAngleIdx + 12).toInt();
    if (closeAngleIdx != -1) servoConfig.closeAngle = json.substring(closeAngleIdx + 13).toInt();
    if (servoSpeedIdx != -1) servoConfig.servoSpeed = json.substring(servoSpeedIdx + 13).toInt();
    
    saveMotorConfig();
    saveDisplayConfig();
    saveServoConfig();
    
    server.send(200, "application/json", "{\"success\":true}");
  } else {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"No data received\"}");
  }
}

// Web Server - Handle servo open command
void handleServoOpen() {
  myServoR.write(servoConfig.openAngle);
  Serial.println("Servo OPEN");
  server.send(200, "application/json", "{\"success\":true,\"message\":\"✅ Servo opened!\"}");
}

// Web Server - Handle servo close command
void handleServoClose() {
  myServoR.write(servoConfig.closeAngle);
  Serial.println("Servo CLOSE");
  server.send(200, "application/json", "{\"success\":true,\"message\":\"✅ Servo closed!\"}");
}

// Initialize WiFi and Web Server
void initWiFiAndServer() {
  Serial.println("\nStarting WiFi...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  server.on("/", handleRoot);
  server.on("/api/config", HTTP_GET, handleConfigGet);
  server.on("/api/config", HTTP_POST, handleConfigPost);
  server.on("/api/servo/open", HTTP_POST, handleServoOpen);
  server.on("/api/servo/close", HTTP_POST, handleServoClose);
  server.begin();
  
  Serial.println("Web Server started");
}

#define FORWARD 1
  // Gambar frame baterai
  display.drawRect(90, 0, 30, 15, SSD1306_WHITE); // Body baterai
  display.fillRect(120, 4, 3, 7, SSD1306_WHITE); // Kepala baterai
  
  // Gambar level baterai berdasarkan parameter
  int width = map(level, 0, 100, 0, 28); // Level baterai (0-100) -> (0-28 pixel)
  display.fillRect(91, 1, width, 13, SSD1306_WHITE); // Isi baterai
}

void battStat() {
  int battstatus = Ps3.data.status.battery;
  String battMessage;
  int battLevel = 0; // Level baterai dalam persen

  if (battstatus == ps3_status_battery_charging) {
    battMessage = "Charging";
    battLevel = 100;
  } else if (battstatus == ps3_status_battery_full) {
    battMessage = "FULL";
    battLevel = 100;
  } else if (battstatus == ps3_status_battery_high) {
    battMessage = "HIGH";
    battLevel = 75;
  } else if (battstatus == ps3_status_battery_low) {
    battMessage = "LOW";
    battLevel = 25;
  } else if (battstatus == ps3_status_battery_dying) {
    battMessage = "DYING";
    battLevel = 10;
  } else if (battstatus == ps3_status_battery_shutdown) {
    battMessage = "SHUTDOWN";
    battLevel = 0;
  } else {
    battMessage = "UNKNOWN";
    battLevel = 0;
  }
   

  // Tampilkan ke OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.println("Battery Ps3:");
  display.setTextSize(1);
  display.setCursor(30, 30);
  display.println(battMessage);

  // Gambar indikator baterai
  drawBatteryIndicator(battLevel);
  display.stopscroll();

  display.display();
}

void NAME_TIM(){
  display.clearDisplay();

 display.setTextSize(2);
 display.setTextColor(SSD1306_WHITE);
 // Set posisi kursor dan menampilkan teks
  display.setCursor( 0, 20);
  display.println ("<WIRAGORA>");
  display.println ("<POLINDRA>");

  // Menampilkan teks di layar
  display.display();
}

void MEMBER_TEAM(){
  display.clearDisplay();

 display.setTextColor(SSD1306_WHITE);

   display.setCursor( 0, 0);
   display.setTextSize(2);

   display.println  ("<WIRAGORA>");

   display.setCursor( 0, 20);
   display.setTextSize(1);
   display.println  (" HERI (DRIVER) ");
   display.println (" RENDI (RETRYER) ");
   display.println (" FAJAR (KOMUNIKOM) ");

   display.display();
};

void logo_team (){
  display.clearDisplay();
  display.setTextColor(SSD1306_BLUE);
  display.drawBitmap(0, 0, epd_bitmap_Logo_Polindra, 128, 64, WHITE);
  display.startscrollright(0x00, 0x0F);
  delay(2000);
  display.startscrollleft(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  display.display();
}


// callback function that will be executed when data is received
void notify() {
  int lx = Ps3.data.analog.stick.lx;        // Analog kiri - x axis
  int ly = Ps3.data.analog.stick.ly;        // Analog kiri - y axis
  int rx = Ps3.data.analog.stick.rx;        // Analog kanan - x axis
  int ry = Ps3.data.analog.stick.ry;        // Analog kanan - y axis
  int r2Value = Ps3.data.analog.button.r2;  // Nilai analog R2
  int l2Value = Ps3.data.analog.button.l2;  // Nilai analog L2
  int l1 = Ps3.data.analog.button.l1;       // Nilai analog L2
  int r1 = Ps3.data.analog.button.r1;       // Nilai analog L2
  int tx = Ps3.data.button.cross;
  int tc = Ps3.data.button.circle;
  int ts = Ps3.data.button.square;
  int tt = Ps3.data.button.triangle;
  int tps = Ps3.event.button_down.ps;
  int str = Ps3.event.button_down.start;
  int ta = Ps3.data.button.up;
  int tb = Ps3.data.button.down;
  int tk1 = Ps3.data.button.right;
  int tk2 = Ps3.data.button.left;

  if (tx) {
  
    myServoR.write(servoConfig.closeAngle);
    Serial.println("tutup"); //tutup gripper
  } else if (ts) {
  
    myServoR.write(servoConfig.openAngle);
    Serial.println("buka");//buka gripper
  }

  if (tc) {
    ledcWrite(FanPin, 255);
    // digitalWrite(FanPin, HIGH);
    Serial.println("Fan ON");
  } else {
    ledcWrite(FanPin, 0);
    // digitalWrite(FanPin, LOW);
  }
  int medspeed = motorConfigs[0].maxSpeed;  // Gunakan max speed dari first motor
  int motorSpeed = motorConfigs[0].baseSpeed;  // Gunakan base speed dari first motor

  if (r2Value > 0) {
    motorSpeed += (MAX_MOTOR_SPEED - motorSpeed) * r2Value / 255;
  } else {
    motorSpeed = motorSpeed;  // menaikan motor speed dengan tombol r2
  }
  if (l2Value > 0) {
    motorSpeed -= (motorSpeed - MIN_MOTOR_SPEED) * l2Value / 255;
    motorSpeed = constrain(motorSpeed, MIN_MOTOR_SPEED, motorSpeed);
  }

    if (ry > -63 && ry < 63 && rx < -60) {
    processCarMovement(RIGHT_2, motorSpeed -40);  //==putar kiri==
    Serial.println("putar kiri");
    Serial.println(motorSpeed);
  } else if (ry > -63 && ry < 63 && rx > 60) {
    processCarMovement(LEFT_2, motorSpeed -40);  //==putar kanan==
    Serial.println("putar kanan");
  }else if (lx < -63 && ly < -63) {
    processCarMovement(FORWARD_LEFT, motorSpeed);  // serong kiri
    Serial.println("serong kiri");
  } else if (lx > 63 && ly < -63) {
    processCarMovement(FORWARD_RIGHT, motorSpeed);  // serong kanan
    Serial.println("serong kanan");
  } else if (rx < -63 && ry < -63) {
    processCarMovement(TURN_LEFT, motorSpeed -25);  // drift kiri
    Serial.println("serong kiri");
  } else if (rx > 63 && ry < -63) {
    processCarMovement(TURN_RIGHT, motorSpeed -25);  // drift kanan
    Serial.println("serong kanan");
  } else if (ly > 63 && lx < -63) {
    processCarMovement(BACKWARD_LEFT, motorSpeed);  // Mundur serong kiri
    Serial.println("mundur kiri");
  } else if (ly > 63 && lx > 63) {
    processCarMovement(BACKWARD_RIGHT, motorSpeed);  // Mundur serong kanan
    Serial.println("mundur kanan");
  } else if (lx > -63 && lx < 63 && ly > 60) {
    processCarMovement(BACKWARD, motorSpeed);  // Mundur
    Serial.println("mundur");
  } else if (lx > -63 && lx < 63 && ly < -60) {
    processCarMovement(FORWARD, motorSpeed);  // maju
    Serial.println("maju ");
  } else if (ly > -63 && ly < 63 && lx < -60) {
    processCarMovement(SPIN_RIGHT, motorSpeed -30);  //++geser kiri++
    Serial.println("geser kiri");
  } else if (ly > -63 && ly < 63 && lx > 60) {
    processCarMovement(SPIN_LEFT, motorSpeed -30);  //++geser kanan++
    Serial.println("geser kanan");
  } else if (ta) {
    processCarMovement(FORWARD, motorSpeed);
    Serial.println("Maju (d-pad)");
  } else if (tb) {
    processCarMovement(BACKWARD, motorSpeed);
    Serial.println("Mundur (d-pad)");
  } else if (tk1) {
    processCarMovement(SPIN_LEFT, motorSpeed);
    Serial.println("serong kiri (d-pad)");
  } else if (tk2) {
    processCarMovement(SPIN_RIGHT, motorSpeed);
    Serial.println("serong (d-pad)");
  } else {
    // Berhenti
    processCarMovement(STOP, 0);
  }

  // } else {
  //     // processCarMovement(STOP, 0);
  //     // Serial.println("Berhenti (d-pad)");
  // }

if (tt){
 MEMBER_TEAM();
}

if(tps){
 battStat();
}

if (str){
 NAME_TIM();
}

 if( Ps3.event.button_down.select ){
  logo_team();
 }




  if (r1) {
    lift_up(medspeed);
    Serial.println("Lift UP");
  } else if (l1) {
    lift_down(medspeed);
    Serial.println("Lift down");
  } else {
    lift_stop(STOP);
  }

  lastRecvTime = millis();
  // delay(10);
}

void onConnect() {
  Serial.println("Connected!.");
}

void onDisConnect() {
  processCarMovement(STOP, 0);
  lift_stop(0);
  Serial.println("Disconnected!.");
  ESP.restart();
}

void lift_up(int medspeed) {
  digitalWrite(LiftIN1, HIGH);
  digitalWrite(LiftIN2, LOW);
  ledcWrite(LiftENA, (medspeed));
}

void lift_down(int medspeed) {
  digitalWrite(LiftIN1, LOW);
  digitalWrite(LiftIN2, HIGH);
  ledcWrite(LiftENA, (medspeed));
}

void lift_stop(int motorSpeed) {
  digitalWrite(LiftIN1, LOW);
  digitalWrite(LiftIN2, LOW);
  ledcWrite(LiftENA, LOW);
}


void processCarMovement(int inputValue, int motorSpeed) {
  switch (inputValue) {
    case FORWARD:
      rotateMotor(FRONT_RIGHT_MOTOR, motorSpeed);
      rotateMotor(BACK_RIGHT_MOTOR, motorSpeed);
      rotateMotor(FRONT_LEFT_MOTOR, motorSpeed);
      rotateMotor(BACK_LEFT_MOTOR, motorSpeed);
      break;

    case BACKWARD:
      rotateMotor(FRONT_RIGHT_MOTOR, -motorSpeed);
      rotateMotor(BACK_RIGHT_MOTOR, -motorSpeed);
      rotateMotor(FRONT_LEFT_MOTOR, -motorSpeed);
      rotateMotor(BACK_LEFT_MOTOR, -motorSpeed);
      break;
    case LEFT_1:
      rotateMotor(FRONT_RIGHT_MOTOR,motorSpeed);
      rotateMotor(BACK_RIGHT_MOTOR, -motorSpeed);
      rotateMotor(FRONT_LEFT_MOTOR, -motorSpeed);
      rotateMotor(BACK_LEFT_MOTOR,  motorSpeed);
      break;

    case RIGHT_1:
      rotateMotor(FRONT_RIGHT_MOTOR, -motorSpeed);
      rotateMotor(BACK_RIGHT_MOTOR, motorSpeed);
      rotateMotor(FRONT_LEFT_MOTOR, motorSpeed);
      rotateMotor(BACK_LEFT_MOTOR, -motorSpeed);
      break;

    case LEFT_2:
      rotateMotor(FRONT_RIGHT_MOTOR, motorSpeed);
      rotateMotor(BACK_RIGHT_MOTOR, -motorSpeed);
      rotateMotor(FRONT_LEFT_MOTOR, -motorSpeed);
      rotateMotor(BACK_LEFT_MOTOR, motorSpeed);
      break;

    case RIGHT_2:
      rotateMotor(FRONT_RIGHT_MOTOR, -motorSpeed);
      rotateMotor(BACK_RIGHT_MOTOR, motorSpeed);
      rotateMotor(FRONT_LEFT_MOTOR, motorSpeed);
      rotateMotor(BACK_LEFT_MOTOR, -motorSpeed);
      break;

    case FORWARD_RIGHT:
      rotateMotor(FRONT_RIGHT_MOTOR,motorSpeed);
      rotateMotor(BACK_RIGHT_MOTOR, motorSpeed);
      rotateMotor(FRONT_LEFT_MOTOR, STOP);
      rotateMotor(BACK_LEFT_MOTOR,  STOP);
      break;

    case FORWARD_LEFT:
      rotateMotor(FRONT_RIGHT_MOTOR, STOP);
      rotateMotor(BACK_RIGHT_MOTOR, STOP);
      rotateMotor(FRONT_LEFT_MOTOR, motorSpeed);
      rotateMotor(BACK_LEFT_MOTOR,  motorSpeed);  
      break;

    case BACKWARD_LEFT:
      rotateMotor(FRONT_RIGHT_MOTOR, -motorSpeed);
      rotateMotor(BACK_RIGHT_MOTOR,  -motorSpeed);
      rotateMotor(FRONT_LEFT_MOTOR,  STOP);
      rotateMotor(BACK_LEFT_MOTOR,   STOP); 
      break;

    case BACKWARD_RIGHT:
      rotateMotor(FRONT_RIGHT_MOTOR, -motorSpeed);
      rotateMotor(BACK_RIGHT_MOTOR, -motorSpeed);
      rotateMotor(FRONT_LEFT_MOTOR, STOP);
      rotateMotor(BACK_LEFT_MOTOR,   STOP);
      break;
    case SPIN_LEFT:
      rotateMotor(FRONT_LEFT_MOTOR, -motorSpeed);
      rotateMotor(BACK_LEFT_MOTOR, -motorSpeed);
      rotateMotor(FRONT_RIGHT_MOTOR, motorSpeed);
      rotateMotor(BACK_RIGHT_MOTOR, motorSpeed);
      break;

    case SPIN_RIGHT:
      rotateMotor(FRONT_LEFT_MOTOR, motorSpeed);
      rotateMotor(BACK_LEFT_MOTOR, motorSpeed);
      rotateMotor(FRONT_RIGHT_MOTOR, -motorSpeed);
      rotateMotor(BACK_RIGHT_MOTOR, -motorSpeed);
      break;

    case TURN_LEFT:
      rotateMotor(FRONT_LEFT_MOTOR, -65);
      rotateMotor(BACK_LEFT_MOTOR, -motorSpeed);
      rotateMotor(FRONT_RIGHT_MOTOR,65);
      rotateMotor(BACK_RIGHT_MOTOR, motorSpeed);
      break;

    case TURN_RIGHT:
      rotateMotor(FRONT_RIGHT_MOTOR,-65);
      rotateMotor(BACK_RIGHT_MOTOR, -motorSpeed);
      rotateMotor(FRONT_LEFT_MOTOR, 65);
      rotateMotor(BACK_LEFT_MOTOR, motorSpeed);
      break;

    case STOP:
      rotateMotor(FRONT_RIGHT_MOTOR, STOP);
      rotateMotor(BACK_RIGHT_MOTOR, STOP);
      rotateMotor(FRONT_LEFT_MOTOR, STOP);
      rotateMotor(BACK_LEFT_MOTOR, STOP);
      break;

    default:
      rotateMotor(FRONT_RIGHT_MOTOR, STOP);
      rotateMotor(BACK_RIGHT_MOTOR, STOP);
      rotateMotor(FRONT_LEFT_MOTOR, STOP);
      rotateMotor(BACK_LEFT_MOTOR, STOP);
      break;
  }
}

void rotateMotor(int motorNumber, int motorSpeed) {
  if (motorSpeed < 0) {
    digitalWrite(motorPins[motorNumber].pinIN1, LOW);
    digitalWrite(motorPins[motorNumber].pinIN2, HIGH);
  } else if (motorSpeed > 0) {
    digitalWrite(motorPins[motorNumber].pinIN1, HIGH);
    digitalWrite(motorPins[motorNumber].pinIN2, LOW);
  } else {
    digitalWrite(motorPins[motorNumber].pinIN1, HIGH);
    digitalWrite(motorPins[motorNumber].pinIN2, HIGH);
  }

  ledcWrite(motorPins[motorNumber].pinEn, abs(motorSpeed));
}

void setUpPinModes() {
  for (int i = 0; i < motorPins.size(); i++) {
    pinMode(motorPins[i].pinIN1, OUTPUT);
    pinMode(motorPins[i].pinIN2, OUTPUT);
    //Set up PWM for motor speed
    ledcAttach(motorPins[i].pinEn, PWMFreq, PWMResolution);
    rotateMotor(i, STOP);
  }
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);


  // myServoL.attach(servoPinL, minp, maxp);  //grip kanan
  // myServoL.setPeriodHertz(50);
  // myServoL.write(80);

  myServoR.attach(servoPinR, minp, maxp);  // servo naik
  myServoR.setPeriodHertz(50);
  myServoR.write(80);

  // pinMode(FanPin, OUTPUT);
  pinMode(LiftIN1, OUTPUT);
  pinMode(LiftIN2, OUTPUT);
  pinMode(LiftENA, OUTPUT);
  ledcAttachChannel(LiftENA, PWMFreq, PWMResolution, 14);
  ledcAttachChannel(FanPin, 25000, PWMResolution, 15);
  
}

void setup() {
  
  setUpPinModes();
  Serial.begin(57600);
  delay(1000);
  
  // Load all configurations from preferences
  loadMotorConfig();
  loadDisplayConfig();
  loadServoConfig();
  
  // Initialize WiFi and Web Server
  initWiFiAndServer();
  
  Ps3.attach(notify);
  Ps3.attachOnConnect(onConnect);
  Ps3.attachOnDisconnect(onDisConnect);
  Ps3.begin("2c:81:58:2f:26:a9");
  Serial.println("Ready.");


  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);
    }
    
    display.clearDisplay();
    display.drawBitmap( 0, 0,epd_bitmap_Logo_Polindra, 128, 64, WHITE);
    display.display(); 
}


void loop() {

  // Handle web server requests
  server.handleClient();

  //Check Signal lost.
  unsigned long now = millis();
  if ( now - lastRecvTime > SIGNAL_TIMEOUT ) 
  {
    processCarMovement(STOP,0);
  }
}

