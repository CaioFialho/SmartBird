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

// ==========================================
// MÓDULO HIDRATAÇÃO
// ==========================================

constexpr uint8_t RELE_BOMBA_AGUA = 29;
constexpr uint8_t SENSOR_BOIA_AGUA = 30;

// ==========================================
// INTERFACE FISICA (LCD I2C usa SDA=20 e SCL=21 no Mega)
// ==========================================

constexpr uint8_t BOTAO_ANTERIOR = 31;
constexpr uint8_t BOTAO_OK = 32;
constexpr uint8_t BOTAO_PROXIMO = 33;
constexpr uint8_t LED_SISTEMA = 34;
constexpr uint8_t LED_ACAO = 35;
constexpr uint8_t LED_AVISO = 36;
constexpr uint8_t LED_CRITICO = 37;
constexpr uint8_t BUZZER_INTERFACE = 38;

#endif