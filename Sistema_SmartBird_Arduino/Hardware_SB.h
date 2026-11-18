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

// ==========================================
// MÓDULO ALIMENTAÇÃO
// ==========================================

constexpr uint8_t HX711_DT_ALIMENTO = 26;
constexpr uint8_t HX711_SCK_ALIMENTO = 27;
constexpr uint8_t SERVO_COMPORTA = 28;

#endif