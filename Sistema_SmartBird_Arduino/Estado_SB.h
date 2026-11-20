#ifndef ESTADO_SB_H
#define ESTADO_SB_H

#include <Arduino.h>

// ==========================================
// ENUMS - ESTADOS DO SISTEMA
// ==========================================

// Estados gerais do equipamento.
enum class EstadoSistema {
    INICIALIZANDO,
    OPERACIONAL,
    ALERTA,
    ERRO
};

// Estados do módulo de alimentação.
enum class EstadoAlimentacao {
    AGUARDANDO,
    VERIFICANDO_PESO,
    ALIMENTANDO,
    FINALIZADO,
    ERRO
};

// Estados do módulo de hidratação.
enum class EstadoHidratacao {
    AGUARDANDO,
    VERIFICANDO_NIVEL,
    ABASTECENDO,
    FINALIZADO,
    ERRO
};

// Estado do nível de água no bebedouro.
// A medição é realizada por uma boia binária.
enum class EstadoRecipienteAgua {
    INDEFINIDO,
    BAIXO,
    OK
};

// Situação de um horário agendado.
enum class EstadoExecucao {
    AGUARDANDO,
    EXECUTADO,
    PENDENTE,
    RECUSADO
};

// ==========================================
// STRUCTS - REGISTROS E AGENDAMENTOS
// ==========================================

// Registro da última execução de um horário.
struct RegistroExecucao {
    uint32_t dataReferencia = 0;
    uint32_t horarioExecucaoEpoch = 0;

    EstadoExecucao estado = EstadoExecucao::AGUARDANDO;
};

// Guarda uma ocorrência que ainda não foi resolvida.
struct RegistroPendencia {
    uint32_t dataReferencia = 0;
    uint32_t horarioOcorrenciaEpoch = 0;

    EstadoExecucao estado = EstadoExecucao::AGUARDANDO;
    bool ativa = false;
};

// Representa um horário individual.
struct Horario {
    uint8_t hora = 0;
    uint8_t minuto = 0;

    bool ativo = false;

    RegistroExecucao registro;
    RegistroPendencia pendencia;
};

// Limite de horários por módulo.
constexpr uint8_t MAX_HORARIOS_DIARIOS = 4;

// Agenda diária de um módulo.
struct AgendaDiaria {
    Horario horarios[MAX_HORARIOS_DIARIOS];

    uint8_t quantidadeHorarios = 0;
};

// ==========================================
// STRUCTS - ALIMENTAÇÃO
// ==========================================

// Configurações de alimentação.
struct ConfiguracaoAlimentacao {
    float pesoAlvoGramas = 0.0f;
};

// Dados operacionais do módulo de alimentação.
struct DadosAlimentacao {
    float pesoAtualGramas = 0.0f;

    bool leituraValida = false;

    bool comportaAberta = false;

    EstadoAlimentacao estado = EstadoAlimentacao::AGUARDANDO;

    unsigned long ultimaExecucaoMs = 0;
};

// ==========================================
// STRUCTS - HIDRATAÇÃO
// ==========================================

// Configurações operacionais de hidratação.
struct ConfiguracaoHidratacao {
    // Tempo máximo permitido para funcionamento da bomba.
    uint32_t tempoMaximoBombaMs = 15000UL;
};

// Dados operacionais do módulo de hidratação.
struct DadosHidratacao {
    // Estado detectado pela boia do bebedouro.
    EstadoRecipienteAgua estadoRecipiente =
        EstadoRecipienteAgua::INDEFINIDO;

    // Estado físico da bomba.
    bool bombaLigada = false;

    // Estado atual do processo de hidratação.
    EstadoHidratacao estado = EstadoHidratacao::AGUARDANDO;

    // Momento da última execução, em milissegundos.
    unsigned long ultimaExecucaoMs = 0;
};

// ==========================================
// STRUCTS - RESERVATÓRIOS
// ==========================================

// Configurações de um reservatório.
struct ConfiguracaoReservatorio {
    // Distância medida quando o reservatório está cheio.
    float distanciaNivelCheioCm = 0.0f;

    // Distância medida quando o reservatório está vazio.
    float distanciaNivelVazioCm = 0.0f;

    // Percentual mínimo para indicar nível baixo.
    float nivelMinimoPercentual = 20.0f;
};

// Dados medidos de um reservatório.
struct DadosReservatorio {
    // Distância atual entre o sensor e a superfície.
    float distanciaSensorCm = 0.0f;

    // Nível calculado em percentual.
    float nivelAtualPercentual = 0.0f;

    // Indica se a leitura do sensor é válida.
    bool leituraValida = false;

    // Indica se o reservatório atingiu o nível mínimo.
    bool nivelBaixo = false;
};

// Reservatório completo.
struct Reservatorio {
    ConfiguracaoReservatorio configuracao;
    DadosReservatorio dados;
};

// ==========================================
// STRUCTS - MANUTENÇÃO
// ==========================================

// Configurações de manutenção.
struct ConfiguracaoManutencao {
    // Intervalo entre limpezas, em segundos.
    uint32_t intervaloLimpezaSegundos = 172800UL;
};

// Dados operacionais de manutenção.
struct DadosManutencao {
    // Data e horário da última limpeza registrada.
    uint32_t ultimaLimpezaEpoch = 0;

    // Indica se existe uma limpeza registrada.
    bool limpezaRegistrada = false;

    // Indica se a limpeza está pendente.
    bool limpezaPendente = false;

    // Indica se o alerta de manutenção está ativo.
    bool alertaAtivo = false;
};

// ==========================================
// STRUCTS - CONFIGURAÇÕES GERAIS
// ==========================================

struct ConfiguracaoSistema {
    // Intervalo de referência para verificações periódicas.
    uint32_t intervaloVerificacaoMs = 1000UL;
};

// ==========================================
// STRUCTS - SISTEMA
// ==========================================

// Estrutura central que reúne o estado completo do SmartBird.
struct DadosSistema {

    // Estado geral do equipamento.
    EstadoSistema estado = EstadoSistema::INICIALIZANDO;

    // Reservatórios de origem.
    Reservatorio reservatorioAlimento;
    Reservatorio reservatorioAgua;

    // Configurações.
    ConfiguracaoSistema configuracaoSistema;
    ConfiguracaoAlimentacao configuracaoAlimentacao;
    ConfiguracaoHidratacao configuracaoHidratacao;
    ConfiguracaoManutencao configuracaoManutencao;

    // Dados operacionais.
    DadosAlimentacao alimentacao;
    DadosHidratacao hidratacao;
    DadosManutencao manutencao;

    // Agendamentos.
    AgendaDiaria agendaAlimentacao;
    AgendaDiaria agendaHidratacao;
};

#endif