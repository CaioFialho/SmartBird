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
  if (indice >= MAX_HORARIOS_DIARIOS) return false;
  if (hora > 23 || minuto > 59) return false;

  Horario& horario = agenda.horarios[indice];

  // Não altera um horário com pendência ativa
  if (horario.pendencia.ativa) return false;

  // Impede horários duplicados na mesma agenda
  for (uint8_t i = 0; i < MAX_HORARIOS_DIARIOS; i++) {
    if (i == indice) continue;

    if (agenda.horarios[i].ativo &&
        agenda.horarios[i].hora == hora &&
        agenda.horarios[i].minuto == minuto) {
      return false;
    }
  }

  horario.hora = hora;
  horario.minuto = minuto;
  horario.ativo = true;

  // Reinicia somente o registro diário
  horario.registro = RegistroExecucao();

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

bool removerHorario(AgendaDiaria& agenda, uint8_t indice) {
  if (indice >= MAX_HORARIOS_DIARIOS) return false;

  Horario& horario = agenda.horarios[indice];

  if (!horario.ativo) return false;

  // Não permite remover horários com pendências
  if (horario.pendencia.ativa) return false;

  horario.ativo = false;
  horario.hora = 0;
  horario.minuto = 0;

  horario.registro = RegistroExecucao();
  horario.pendencia = RegistroPendencia();

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

// ==========================================
// VERIFICAÇÃO DE AGENDAMENTOS
// ==========================================

// Guarda o instante da última verificação
static uint32_t ultimoInstanteVerificado = 0;

// Verifica uma agenda individual
static void verificarAgenda(
  AgendaDiaria& agenda,
  const DateTime& agora,
  uint32_t instanteAnterior,
  uint32_t instanteAtual,
  uint32_t dataAtual
) {
  for (uint8_t i = 0; i < MAX_HORARIOS_DIARIOS; i++) {

    Horario& horario = agenda.horarios[i];

    if (!horario.ativo) continue;

    RegistroExecucao& registro = horario.registro;
    RegistroPendencia& pendencia = horario.pendencia;

    // Já existe um registro para este horário hoje
    if (registro.dataReferencia == dataAtual) {
      continue;
    }

    // Monta o instante do horário programado
    DateTime horarioProgramado(
      agora.year(),
      agora.month(),
      agora.day(),
      horario.hora,
      horario.minuto,
      0
    );

    uint32_t instanteProgramado = horarioProgramado.unixtime();

    // Verifica se o horário chegou durante o funcionamento
    if (instanteProgramado > instanteAnterior &&
        instanteProgramado <= instanteAtual) {

      // Registra a ocorrência do dia
      registro.dataReferencia = dataAtual;
      registro.horarioExecucaoEpoch = 0;
      registro.estado = EstadoExecucao::PENDENTE;
    }
  }
}

// Função pública do módulo
void verificarAgendamentos(
  DadosSistema& dados,
  const DateTime& agora
) {
  uint32_t instanteAtual = agora.unixtime();

  // Primeira consulta: estabelece o ponto inicial.
  // Não executa horários que já passaram antes da inicialização.
  if (ultimoInstanteVerificado == 0) {
    ultimoInstanteVerificado = instanteAtual;
    return;
  }

  // Se o relógio retroceder, reinicia a referência.
  if (instanteAtual < ultimoInstanteVerificado) {
    ultimoInstanteVerificado = instanteAtual;
    return;
  }

  uint32_t dataAtual = obterDataReferencia(agora);

  verificarAgenda(
    dados.agendaAlimentacao,
    agora,
    ultimoInstanteVerificado,
    instanteAtual,
    dataAtual
  );

  verificarAgenda(
    dados.agendaHidratacao,
    agora,
    ultimoInstanteVerificado,
    instanteAtual,
    dataAtual
  );

  ultimoInstanteVerificado = instanteAtual;
}


bool registrarExecucaoConcluida(
  AgendaDiaria& agenda,
  uint8_t indice,
  const DateTime& instanteConclusao
) {
  if (indice >= MAX_HORARIOS_DIARIOS) return false;

  Horario& horario = agenda.horarios[indice];

  if (!horario.ativo) return false;

  // Só confirma um agendamento que estava aguardando execução
  if (horario.registro.estado != EstadoExecucao::PENDENTE) {
    return false;
  }

  // Registra a conclusão do agendamento
  horario.registro.dataReferencia =
    obterDataReferencia(instanteConclusao);

  horario.registro.horarioExecucaoEpoch =
    instanteConclusao.unixtime();

  horario.registro.estado = EstadoExecucao::EXECUTADO;

  // Uma execução programada bem-sucedida resolve a pendência anterior
  horario.pendencia = RegistroPendencia();

  return true;
}