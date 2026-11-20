#ifndef ESTADO_SB_H
#define ESTADO_SB_H

#include <Arduino.h>

// Estados gerais.
enum class EstadoSistema : uint8_t {
    INICIALIZANDO, OPERACIONAL, ALERTA, ERRO
};

enum class EstadoAlimentacao : uint8_t {
    AGUARDANDO, VERIFICANDO_PESO, ALIMENTANDO, FINALIZADO, ERRO
};

enum class EstadoHidratacao : uint8_t {
    AGUARDANDO, VERIFICANDO_NIVEL, ABASTECENDO, FINALIZADO, ERRO
};

enum class EstadoRecipienteAgua : uint8_t {
    INDEFINIDO, BAIXO, OK
};

enum class EstadoExecucao : uint8_t {
    AGUARDANDO, PENDENTE, EXECUTANDO, EXECUTADO, RECUSADO
};

struct RegistroExecucao {
    uint32_t dataReferencia = 0;       // AAAAMMDD
    uint32_t horarioExecucaoEpoch = 0;
    EstadoExecucao estado = EstadoExecucao::AGUARDANDO;
};

struct RegistroPendencia {
    uint32_t dataReferencia = 0;
    uint32_t horarioOcorrenciaEpoch = 0;
    EstadoExecucao estado = EstadoExecucao::AGUARDANDO;
    bool ativa = false;
};

struct Horario {
    uint8_t hora = 0;
    uint8_t minuto = 0;
    bool ativo = false;
    RegistroExecucao registro;
    RegistroPendencia pendencia;
};

constexpr uint8_t MAX_HORARIOS_DIARIOS = 4;

struct AgendaDiaria {
    Horario horarios[MAX_HORARIOS_DIARIOS];
    uint8_t quantidadeHorarios = 0;
};

struct ConfiguracaoAlimentacao {
    float pesoAlvoGramas = 0.0f;
    uint32_t tempoMaximoMs = 15000UL;
};

struct DadosAlimentacao {
    float pesoAtualGramas = 0.0f;
    bool leituraValida = false;
    bool comportaAberta = false;
    EstadoAlimentacao estado = EstadoAlimentacao::AGUARDANDO;
    unsigned long ultimaExecucaoMs = 0;
};

struct ConfiguracaoHidratacao {
    uint32_t tempoMaximoBombaMs = 15000UL;
};

struct DadosHidratacao {
    EstadoRecipienteAgua estadoRecipiente = EstadoRecipienteAgua::INDEFINIDO;
    bool bombaLigada = false;
    EstadoHidratacao estado = EstadoHidratacao::AGUARDANDO;
    unsigned long ultimaExecucaoMs = 0;
};

struct ConfiguracaoReservatorio {
    float distanciaNivelCheioCm = 0.0f;
    float distanciaNivelVazioCm = 0.0f;
    float nivelMinimoPercentual = 20.0f;
};

struct DadosReservatorio {
    float distanciaSensorCm = 0.0f;
    float nivelAtualPercentual = 0.0f;
    bool leituraValida = false;
    bool nivelBaixo = false;
};

struct Reservatorio {
    ConfiguracaoReservatorio configuracao;
    DadosReservatorio dados;
};

struct ConfiguracaoManutencao {
    uint32_t intervaloLimpezaSegundos = 172800UL;
};

struct DadosManutencao {
    uint32_t ultimaLimpezaEpoch = 0;
    bool limpezaRegistrada = false;
    bool limpezaPendente = false;
    bool alertaAtivo = false;
};

struct ConfiguracaoSistema {
    uint32_t intervaloVerificacaoMs = 1000UL;
};

struct DadosSistema {
    EstadoSistema estado = EstadoSistema::INICIALIZANDO;

    Reservatorio reservatorioAlimento;
    Reservatorio reservatorioAgua;

    ConfiguracaoSistema configuracaoSistema;
    ConfiguracaoAlimentacao configuracaoAlimentacao;
    ConfiguracaoHidratacao configuracaoHidratacao;
    ConfiguracaoManutencao configuracaoManutencao;

    DadosAlimentacao alimentacao;
    DadosHidratacao hidratacao;
    DadosManutencao manutencao;

    AgendaDiaria agendaAlimentacao;
    AgendaDiaria agendaHidratacao;
};

#endif
