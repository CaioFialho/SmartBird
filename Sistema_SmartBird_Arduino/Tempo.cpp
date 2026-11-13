#include <Wire.h>
#include "Tempo.h"

// ==========================================
// OBJETO RTC
// ==========================================

static RTC_DS1307 rtc;

static bool rtcInicializado = false;

// ==========================================
// INICIALIZAÇÃO
// ==========================================

bool inicializarTempo() {

  Wire.begin();

  rtcInicializado = rtc.begin();

  return rtcInicializado;
}

// ==========================================
// CONSULTA DE DATA E HORA
// ==========================================

bool obterDataHora(DateTime& agora) {

  if (!rtcInicializado) {
    return false;
  }

  // Verifica se o oscilador do RTC está funcionando
  if (!rtc.isrunning()) {
    return false;
  }

  agora = rtc.now();

  return true;
}

// ==========================================
// AJUSTE DE DATA E HORA
// ==========================================

bool ajustarDataHora(const DateTime& novaDataHora) {

  if (!rtcInicializado) {
    return false;
  }

  rtc.adjust(novaDataHora);

  return rtc.isrunning();
}


// ==========================================
// GERENCIAMENTO DE AGENDAS
// ==========================================

// Cadastra ou altera um horário
bool definirHorario(
  AgendaDiaria& agenda,
  uint8_t indice,
  uint8_t hora,
  uint8_t minuto
) {

  // Valida a posição
  if (indice >= MAX_HORARIOS_DIARIOS) {
    return false;
  }

  // Valida o horário
  if (hora > 23 || minuto > 59) {
    return false;
  }

  // Impede horários duplicados
  for (uint8_t i = 0; i < MAX_HORARIOS_DIARIOS; i++) {

    if (i == indice) {
      continue;
    }

    if (agenda.horarios[i].ativo &&
        agenda.horarios[i].hora == hora &&
        agenda.horarios[i].minuto == minuto) {

      return false;
    }
  }

  // Configura o horário
  agenda.horarios[indice].hora = hora;
  agenda.horarios[indice].minuto = minuto;
  agenda.horarios[indice].ativo = true;

  // Atualiza a quantidade de horários ativos
  agenda.quantidadeHorarios = 0;

  for (uint8_t i = 0; i < MAX_HORARIOS_DIARIOS; i++) {

    if (agenda.horarios[i].ativo) {
      agenda.quantidadeHorarios++;
    }
  }

  return true;
}

// ==========================================
// REMOVER HORÁRIO
// ==========================================

bool removerHorario(
  AgendaDiaria& agenda,
  uint8_t indice
) {

  if (indice >= MAX_HORARIOS_DIARIOS) {
    return false;
  }

  if (!agenda.horarios[indice].ativo) {
    return false;
  }

  // Desativa o horário
  agenda.horarios[indice].ativo = false;

  // Reinicia os dados do horário
  agenda.horarios[indice].hora = 0;
  agenda.horarios[indice].minuto = 0;
  agenda.horarios[indice].registro = RegistroExecucao();

  // Atualiza a quantidade de horários ativos
  agenda.quantidadeHorarios = 0;

  for (uint8_t i = 0; i < MAX_HORARIOS_DIARIOS; i++) {

    if (agenda.horarios[i].ativo) {
      agenda.quantidadeHorarios++;
    }
  }

  return true;
}

// ==========================================
// VERIFICAR SE HORÁRIO ESTÁ ATIVO
// ==========================================

bool horarioAtivo(
  const AgendaDiaria& agenda,
  uint8_t indice
) {

  if (indice >= MAX_HORARIOS_DIARIOS) {
    return false;
  }

  return agenda.horarios[indice].ativo;
}

// ==========================================
// VERIFICAR SE A AGENDA POSSUI HORÁRIOS
// ==========================================

bool agendaPossuiHorarios(
  const AgendaDiaria& agenda
) {

  for (uint8_t i = 0; i < MAX_HORARIOS_DIARIOS; i++) {

    if (agenda.horarios[i].ativo) {
      return true;
    }
  }

  return false;
}

// ==========================================
// REFERÊNCIA DE DATA
// ==========================================

uint32_t obterDataReferencia(const DateTime& dataHora) {
  uint32_t ano = dataHora.year();
  uint32_t mes = dataHora.month();
  uint32_t dia = dataHora.day();

  return (ano * 10000UL) + (mes * 100UL) + dia;
}
