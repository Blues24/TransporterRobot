#ifndef PS3_HANDLER_H
#define PS3_HANDLER_H

#include "config.h"
#include <Ps3Controller.h>

// ============================================================
//  SETUP - panggil di setup()
// ============================================================
void ps3Setup();


// ============================================================
//  STATE - bisa dibaca dari luar (misal: Transporter.ino)
// ============================================================
extern volatile bool eStopActive; // Emergency stop, true = Emergency stop aktif

// ============================================================
//  CONFIG PERSISTENCE
// ============================================================
void loadPs3Config();
void savePs3Config();

#endif // PS3_HANDLER_H