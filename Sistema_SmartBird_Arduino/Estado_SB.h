#ifndef ESTADO_SMARTBIRD_H
#define ESTADO_SMARTBIRD_H

#include <Arduino.h>

// ==========================================
// ENUMS - ESTADOS DOS MÓDULOS
// ==========================================

// Estados gerais do sistema
enum class EstadoSistema {
  INICIALIZANDO,
  OPERACIONAL,
  ALERTA,
  ERRO
};

// Estados da alimentação
enum class EstadoAlimentacao {
  AGUARDANDO,
  VERIFICANDO_PESO,
  ALIMENTANDO,
  FINALIZADO,
  ERRO
};

// Estados da hidratação
enum class EstadoHidratacao {
  AGUARDANDO,
  VERIFICANDO_NIVEL,
  ABASTECENDO,
  FINALIZADO,
  ERRO
};

// ==========================================
// STRUCTS - DADOS DOS MÓDULOS
// ==========================================

// Dados do módulo de alimentação
struct DadosAlimentacao {
  float pesoAtualGramas = 0.0;
  float pesoAlvoGramas = 0.0;

  bool comportaAberta = false;

  EstadoAlimentacao estado = EstadoAlimentacao::AGUARDANDO;

  unsigned long ultimaExecucao = 0;
};

// Dados do módulo de hidratação
struct DadosHidratacao {
  float nivelAtualPercentual = 0.0;
  float nivelAlvoPercentual = 0.0;

  bool bombaLigada = false;

  EstadoHidratacao estado = EstadoHidratacao::AGUARDANDO;

  unsigned long ultimaExecucao = 0;
};

// Configurações de manutenção
struct ConfiguracaoManutencao {
  uint32_t intervaloLimpezaSegundos = 172800UL;
};

// Dados do módulo de manutenção
struct DadosManutencao {
  uint32_t ultimaLimpezaEpoch = 0;

  bool limpezaRegistrada = false;
  bool limpezaPendente = false;
  bool alertaAtivo = false;
};

// Configurações de um reservatório
struct ConfiguracaoReservatorio {
  float distanciaNivelCheioCm = 0.0;
  float distanciaNivelVazioCm = 0.0;

  float nivelMinimoPercentual = 20.0;
};

// Dados medidos de um reservatório
struct DadosReservatorio {
  float distanciaSensorCm = 0.0;
  float nivelAtualPercentual = 0.0;

  bool leituraValida = false;
  bool nivelBaixo = false;
};

// Reservatório completo
struct Reservatorio {
  ConfiguracaoReservatorio configuracao;
  DadosReservatorio dados;
};

// Representação de um horário
struct Horario {
  uint8_t hora = 0;
  uint8_t minuto = 0;

  bool ativo = false;
};

// Limite de horários por módulo
constexpr uint8_t MAX_HORARIOS_DIARIOS = 4;

// Agenda diária de um módulo
struct AgendaDiaria {
  Horario horarios[MAX_HORARIOS_DIARIOS];

  uint8_t quantidadeHorarios = 0;
};

// Dados gerais do sistema
struct DadosSistema {
  EstadoSistema estado = EstadoSistema::INICIALIZANDO;

  Reservatorio reservatorioAlimento;
  Reservatorio reservatorioAgua;

  DadosAlimentacao alimentacao;
  DadosHidratacao hidratacao;

  AgendaDiaria agendaAlimentacao;
  AgendaDiaria agendaHidratacao;

  ConfiguracaoManutencao configuracaoManutencao;
  DadosManutencao manutencao;
};

#endif