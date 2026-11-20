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
  if (horario.pendencia.ativa || horario.registro.estado == EstadoExecucao::PENDENTE || horario.registro.estado == EstadoExecucao::EXECUTANDO) return false;

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
  if (horario.pendencia.ativa || horario.registro.estado == EstadoExecucao::PENDENTE || horario.registro.estado == EstadoExecucao::EXECUTANDO) return false;

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
// SINCRONIZAR AGENDA COMPLETA
// ==========================================

bool sincronizarAgenda(
  AgendaDiaria& agenda,
  const uint8_t horas[],
  const uint8_t minutos[],
  uint8_t quantidade
) {

  // Valida a quantidade recebida
  if (quantidade > MAX_HORARIOS_DIARIOS) {
    return false;
  }

  // Verifica se os vetores foram fornecidos
  if (quantidade > 0 && (horas == nullptr || minutos == nullptr)) {
    return false;
  }

  // Valida os horários recebidos
  for (uint8_t i = 0; i < quantidade; i++) {

    if (horas[i] > 23 || minutos[i] > 59) {
      return false;
    }

    // Impede horários duplicados na lista recebida
    for (uint8_t j = i + 1; j < quantidade; j++) {

      if (horas[i] == horas[j] &&
          minutos[i] == minutos[j]) {
        return false;
      }
    }
  }

  // Copia a agenda atual para trabalhar sem alterar a original
  AgendaDiaria novaAgenda = agenda;

  // Identifica os horários que devem ser preservados
  bool manter[MAX_HORARIOS_DIARIOS] = {};

  for (uint8_t i = 0; i < quantidade; i++) {

    for (uint8_t j = 0; j < MAX_HORARIOS_DIARIOS; j++) {

      Horario& atual = novaAgenda.horarios[j];

      if (atual.ativo &&
          atual.hora == horas[i] &&
          atual.minuto == minutos[i]) {

        manter[j] = true;
        break;
      }
    }
  }

  // Remove os horários que não estão mais na lista
  for (uint8_t i = 0; i < MAX_HORARIOS_DIARIOS; i++) {

    Horario& horario = novaAgenda.horarios[i];

    if (!horario.ativo || manter[i]) {
      continue;
    }

    // Não permite perder uma pendência ativa
    if (horario.pendencia.ativa ||
        horario.registro.estado == EstadoExecucao::PENDENTE ||
        horario.registro.estado == EstadoExecucao::EXECUTANDO) {
      return false;
    }

    // Libera a posição
    horario = Horario();
  }

  // Adiciona os horários novos nas posições livres
  for (uint8_t i = 0; i < quantidade; i++) {

    bool jaExiste = false;

    // Verifica se o horário já foi preservado
    for (uint8_t j = 0; j < MAX_HORARIOS_DIARIOS; j++) {

      const Horario& horario = novaAgenda.horarios[j];

      if (horario.ativo &&
          horario.hora == horas[i] &&
          horario.minuto == minutos[i]) {

        jaExiste = true;
        break;
      }
    }

    if (jaExiste) {
      continue;
    }

    // Procura a primeira posição livre
    bool inserido = false;

    for (uint8_t j = 0; j < MAX_HORARIOS_DIARIOS; j++) {

      Horario& horario = novaAgenda.horarios[j];

      if (!horario.ativo) {

        horario.hora = horas[i];
        horario.minuto = minutos[i];
        horario.ativo = true;

        inserido = true;
        break;
      }
    }

    // Segurança: não permite inserção incompleta
    if (!inserido) {
      return false;
    }
  }

  // Recalcula a quantidade de horários ativos
  novaAgenda.quantidadeHorarios = 0;

  for (uint8_t i = 0; i < MAX_HORARIOS_DIARIOS; i++) {

    if (novaAgenda.horarios[i].ativo) {
      novaAgenda.quantidadeHorarios++;
    }
  }

  // Confirma a atualização
  agenda = novaAgenda;

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
  uint32_t instanteAnterior,
  uint32_t instanteAtual
) {

  // Identifica o início do dia da última verificação
  DateTime dataInicial(instanteAnterior);

  uint32_t inicioDia = DateTime(
    dataInicial.year(),
    dataInicial.month(),
    dataInicial.day(),
    0, 0, 0
  ).unixtime();

  // Percorre todos os dias dentro do intervalo
  while (inicioDia <= instanteAtual) {

    DateTime dataDia(inicioDia);

    for (uint8_t i = 0; i < MAX_HORARIOS_DIARIOS; i++) {

      Horario& horario = agenda.horarios[i];

      if (!horario.ativo) {
        continue;
      }

      RegistroExecucao& registro = horario.registro;

      // Não sobrescreve uma execução ainda pendente
      if (registro.estado == EstadoExecucao::PENDENTE ||
          registro.estado == EstadoExecucao::EXECUTANDO) {
        continue;
      }

      // Monta o instante programado para este dia
      DateTime horarioProgramado(
        dataDia.year(),
        dataDia.month(),
        dataDia.day(),
        horario.hora,
        horario.minuto,
        0
      );

      uint32_t instanteProgramado =
        horarioProgramado.unixtime();

      // Já existe um registro deste horário para esse dia
      uint32_t dataReferencia =
        obterDataReferencia(horarioProgramado);

      if (registro.dataReferencia == dataReferencia) {
        continue;
      }

      // Verifica se o horário chegou dentro do intervalo
      if (instanteProgramado > instanteAnterior &&
          instanteProgramado <= instanteAtual) {

        registro.dataReferencia = dataReferencia;
        registro.horarioExecucaoEpoch = 0;
        registro.estado = EstadoExecucao::PENDENTE;
      }
    }

    // Avança para o próximo dia
    inicioDia += 86400UL;
  }
}

// Função pública do módulo
void verificarAgendamentos(
  DadosSistema& dados,
  const DateTime& agora
) {

  uint32_t instanteAtual = agora.unixtime();

  // Primeira consulta: estabelece o ponto inicial
  if (ultimoInstanteVerificado == 0) {

    ultimoInstanteVerificado = instanteAtual;
    return;
  }

  // Se o relógio retroceder ou saltar muito à frente (por exemplo,
  // após sincronização do RTC), reinicia a referência sem varrer anos
  // de histórico e sem disparar todos os horários antigos.
  if (instanteAtual < ultimoInstanteVerificado ||
      instanteAtual - ultimoInstanteVerificado > 120UL) {
    ultimoInstanteVerificado = instanteAtual;
    return;
  }

  // Verifica os agendamentos de alimentação
  verificarAgenda(
    dados.agendaAlimentacao,
    ultimoInstanteVerificado,
    instanteAtual
  );

  // Verifica os agendamentos de hidratação
  verificarAgenda(
    dados.agendaHidratacao,
    ultimoInstanteVerificado,
    instanteAtual
  );

  // Atualiza o instante da última verificação
  ultimoInstanteVerificado = instanteAtual;
}


// ==========================================
// REGISTRAR EXECUÇÃO CONCLUÍDA
// ==========================================

bool registrarExecucaoConcluida(
  AgendaDiaria& agenda,
  uint8_t indice,
  const DateTime& instanteConclusao
) {
  if (indice >= MAX_HORARIOS_DIARIOS) {
    return false;
  }

  Horario& horario = agenda.horarios[indice];

  if (!horario.ativo) {
    return false;
  }

  // Só confirma um agendamento pendente
  if (horario.registro.estado != EstadoExecucao::PENDENTE &&
      horario.registro.estado != EstadoExecucao::EXECUTANDO) {
    return false;
  }

  // Preserva a data original do agendamento
  // Não altera registro.dataReferencia

  // Registra o instante real da conclusão
  horario.registro.horarioExecucaoEpoch =
    instanteConclusao.unixtime();

  horario.registro.estado = EstadoExecucao::EXECUTADO;

  // Resolve a pendência após a execução bem-sucedida
  horario.pendencia = RegistroPendencia();

  return true;
}