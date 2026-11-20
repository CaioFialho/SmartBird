#include "Controle_SB.h"

#include "Estado_SB.h"
#include "Hardware_SB.h"
#include "Reservatorios.h"
#include "Alimentacao.h"
#include "Hidratacao.h"
#include "Tempo.h"
#include "Manutencao.h"
#include "Comunicacao.h"
#include "Protocolo.h"

// Estado compartilhado do equipamento.
static DadosSistema dados;

// Evita executar inicializações mais de uma vez.
static bool controleInicializado = false;

// ==========================================
// INICIALIZAÇÃO
// ==========================================

void inicializarControle() {

    if (controleInicializado) {
        return;
    }

    Serial.println("================================");
    Serial.println("       SMARTBIRD INICIANDO");
    Serial.println("================================");

    // Configurações dos reservatórios de origem.
    dados.reservatorioAlimento.configuracao.distanciaNivelCheioCm = 5.0f;
    dados.reservatorioAlimento.configuracao.distanciaNivelVazioCm = 35.0f;
    dados.reservatorioAlimento.configuracao.nivelMinimoPercentual = 20.0f;

    dados.reservatorioAgua.configuracao.distanciaNivelCheioCm = 5.0f;
    dados.reservatorioAgua.configuracao.distanciaNivelVazioCm = 30.0f;
    dados.reservatorioAgua.configuracao.nivelMinimoPercentual = 20.0f;

    bool inicializacaoComFalha = false;
    bool rtcDisponivel = false;

    // Reservatórios.
    inicializarReservatorios();
    Serial.println("Reservatorios inicializados.");

    // Alimentação.
    if (!inicializarAlimentacao()) {
        Serial.println("ERRO: falha ao inicializar alimentacao.");
        inicializacaoComFalha = true;
    } else {
        Serial.println("Modulo de alimentacao inicializado.");
    }

    // Hidratação.
    if (!inicializarHidratacao()) {
        Serial.println("ERRO: falha ao inicializar hidratacao.");
        inicializacaoComFalha = true;
    } else {
        Serial.println("Modulo de hidratacao inicializado.");
    }

    // Relógio.
    rtcDisponivel = inicializarTempo();

    if (rtcDisponivel) {
        Serial.println("RTC inicializado.");
    } else {
        Serial.println("AVISO: RTC indisponivel.");
    }

    // Manutenção.
    if (!inicializarManutencao()) {
        Serial.println("ERRO: falha ao inicializar manutencao.");
        inicializacaoComFalha = true;
    } else {
        Serial.println("Modulo de manutencao inicializado.");
    }

    // Comunicação.
    inicializarComunicacao();

#if SMARTBIRD_COMUNICACAO_USB
    Serial.println("Comunicacao USB de teste inicializada.");
#else
    Serial.println("Comunicacao UART inicializada.");
#endif

    // Define o estado inicial do sistema.
    if (inicializacaoComFalha) {
        dados.estado = EstadoSistema::ERRO;
    } else if (!rtcDisponivel) {
        dados.estado = EstadoSistema::ALERTA;
    } else {
        dados.estado = EstadoSistema::OPERACIONAL;
    }

    controleInicializado = true;

    Serial.println("Inicializacao do controle concluida.");
}

// ==========================================
// CICLO PRINCIPAL
// ==========================================

void atualizarControle() {

    if (!controleInicializado) {
        return;
    }

    // Recepção e processamento de comandos.
    char mensagem[TAMANHO_BUFFER_MENSAGEM];

    if (receberMensagem(mensagem, sizeof(mensagem))) {
        processarMensagem(mensagem, dados);
    }

    // Atualização dos módulos físicos.
    atualizarReservatorios(dados);
    atualizarAlimentacao(dados);
    atualizarHidratacao(dados);
    atualizarManutencao(dados);
}