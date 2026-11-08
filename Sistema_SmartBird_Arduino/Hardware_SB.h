#ifndef HARDWARE_SB_H
#define HARDWARE_SB_H

#include <Arduino.h>

// ==========================================
// SENSOR DO RESERVATÓRIO DE ALIMENTO
// ==========================================

constexpr uint8_t TRIG_ALIMENTO = 22;
constexpr uint8_t ECHO_ALIMENTO = 23;

// ==========================================
// SENSOR DO RESERVATÓRIO DE ÁGUA
// ==========================================

constexpr uint8_t TRIG_AGUA = 24;
constexpr uint8_t ECHO_AGUA = 25;

#endif