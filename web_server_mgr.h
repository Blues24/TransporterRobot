#ifndef WEB_SERVER_MGR_H
#define WEB_SERVER_MGR_H

#include "config.h"
#include <WiFi.h>
#include <WebServer.h>

#define WIFI_SSID     "Transporter 17"
#define WIFI_PASSWORD "fajarganteng"

void webServerSetup();
void webServerHandle();     // panggil di loop()

void loadMotorConfig();
void saveMotorConfig();

#endif // WEB_SERVER_MGR_H