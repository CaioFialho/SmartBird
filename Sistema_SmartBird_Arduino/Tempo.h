
#ifndef TEMPO_H
#define TEMPO_H

#include <Arduino.h>
#include <RTClib.h>
#include "Estado_SB.h"

// ==========================================
// GERENCIAMENTO DO RTC
// ==========================================

// Inicializa o módulo RTC
bool inicializarTempo();

// Consulta a data e o horário atuais
bool obterDataHora(DateTime& agora);

// Ajusta a data e o horário do RTC
bool ajustarDataHora(const DateTime& novaDataHora);

// ==========================================
// GERENCIAMENTO DE AGENDAS
// ==========================================

// Cadastra ou altera um horário
bool definirHorario(
  AgendaDiaria& agenda,
  uint8_t indice,
  uint8_t hora,
  uint8_t minuto
);

// Remove um horário
bool removerHorario(
  AgendaDiaria& agenda,
  uint8_t indice
);

// Verifica se um horário está ativo
bool horarioAtivo(
  const AgendaDiaria& agenda,
  uint8_t indice
);

// Verifica se a agenda possui horários cadastrados
bool agendaPossuiHorarios(
  const AgendaDiaria& agenda
);

// Gera uma referência numérica para a data (AAAAMMDD)
uint32_t obterDataReferencia(const DateTime& dataHora);

#endif