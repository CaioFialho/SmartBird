#include <HX711.h>
#include <Servo.h>
#include <math.h>

#include "Alimentacao.h"
#include "Hardware_SB.h"

// ==========================================
// OBJETOS
// ==========================================

static HX711 balanca;
static Servo servoComporta;

// ==========================================
// CONFIGURAÇÕES PROVISÓRIAS
// ==========================================

// Fator inicial. Será definido durante a calibração.
static constexpr float FATOR_CALIBRACAO = 1.0f;

// Posições provisórias do servo.
static constexpr uint8_t ANGULO_FECHADO = 0;
static constexpr uint8_t ANGULO_ABERTO = 90;

// Intervalo entre leituras.
static constexpr unsigned long INTERVALO_LEITURA_MS = 250UL;

// Tempo máximo de uma operação.
static constexpr unsigned long TEMPO_LIMITE_MS = 15000UL;

// ==========================================
// CONTROLE INTERNO
// ==========================================

static bool alimentacaoEmAndamento = false;

static unsigned long inicioAlimentacaoMs = 0;
static unsigned long ultimaLeituraMs = 0;

static bool balancaZerada = false;
static bool balancaCalibrada = false;

// ==========================================
// FECHAR COMPORTA
// ==========================================

static void fecharComporta() {

    servoComporta.write(ANGULO_FECHADO);
}

// ==========================================
// INICIALIZAÇÃO
// ==========================================

bool inicializarAlimentacao() {

    balanca.begin(
        HX711_DT_ALIMENTO,
        HX711_SCK_ALIMENTO
    );

    balanca.set_scale(FATOR_CALIBRACAO);

    servoComporta.attach(SERVO_COMPORTA);
    fecharComporta();

    return true;
}

// ==========================================
// INICIAR ALIMENTAÇÃO
// ==========================================

bool iniciarAlimentacao(DadosSistema& dados) {

    if (alimentacaoEmAndamento) {
        return false;
    }

    if (!balancaCalibrada) {
        dados.alimentacao.estado = EstadoAlimentacao::ERRO;
        return false;
    }

    if (dados.configuracaoAlimentacao.pesoAlvoGramas <= 0) {
        dados.alimentacao.estado = EstadoAlimentacao::ERRO;
        return false;
    }

    if (!balanca.is_ready()) {
        dados.alimentacao.estado = EstadoAlimentacao::ERRO;
        return false;
    }

    alimentacaoEmAndamento = true;

    inicioAlimentacaoMs = millis();
    ultimaLeituraMs = 0;

    dados.alimentacao.estado = EstadoAlimentacao::VERIFICANDO_PESO;

    return true;
}

// ==========================================
// ATUALIZAR ALIMENTAÇÃO
// ==========================================

void atualizarAlimentacao(DadosSistema& dados) {

    if (!alimentacaoEmAndamento) {
        return;
    }

    unsigned long agoraMs = millis();

    // Verifica o limite de tempo da operação.
    if (agoraMs - inicioAlimentacaoMs >= TEMPO_LIMITE_MS) {

        fecharComporta();

        dados.alimentacao.comportaAberta = false;
        dados.alimentacao.estado = EstadoAlimentacao::ERRO;

        alimentacaoEmAndamento = false;

        return;
    }

    // Respeita o intervalo entre leituras.
    if (agoraMs - ultimaLeituraMs < INTERVALO_LEITURA_MS) {
        return;
    }

    ultimaLeituraMs = agoraMs;

    // Verifica disponibilidade do HX711.
    if (!balanca.is_ready()) {
        return;
    }

    float pesoAtual = balanca.get_units(3);

    // Verifica se a leitura é válida.
    if (isnan(pesoAtual) || isinf(pesoAtual)) {

        fecharComporta();

        dados.alimentacao.comportaAberta = false;
        dados.alimentacao.estado = EstadoAlimentacao::ERRO;

        alimentacaoEmAndamento = false;

        return;
    }

    dados.alimentacao.pesoAtualGramas = pesoAtual;

    switch (dados.alimentacao.estado) {

        // ----------------------------------
        // VERIFICANDO PESO
        // ----------------------------------

        case EstadoAlimentacao::VERIFICANDO_PESO:

            if (pesoAtual >= dados.configuracaoAlimentacao.pesoAlvoGramas) {

                fecharComporta();

                dados.alimentacao.comportaAberta = false;
                dados.alimentacao.estado = EstadoAlimentacao::FINALIZADO;
                dados.alimentacao.ultimaExecucaoMs = agoraMs;

                alimentacaoEmAndamento = false;
            }
            else {

                servoComporta.write(ANGULO_ABERTO);

                dados.alimentacao.comportaAberta = true;
                dados.alimentacao.estado = EstadoAlimentacao::ALIMENTANDO;
            }

            break;

        // ----------------------------------
        // ALIMENTANDO
        // ----------------------------------

        case EstadoAlimentacao::ALIMENTANDO:

            if (pesoAtual >= dados.configuracaoAlimentacao.pesoAlvoGramas) {

                fecharComporta();

                dados.alimentacao.comportaAberta = false;
                dados.alimentacao.estado = EstadoAlimentacao::FINALIZADO;
                dados.alimentacao.ultimaExecucaoMs = agoraMs;

                alimentacaoEmAndamento = false;
            }

            break;

        // ----------------------------------
        // ESTADO INVÁLIDO
        // ----------------------------------

        default:

            fecharComporta();

            dados.alimentacao.comportaAberta = false;
            dados.alimentacao.estado = EstadoAlimentacao::ERRO;

            alimentacaoEmAndamento = false;

            break;
    }
}

// ==========================================
// ZERAR BALANÇA
// ==========================================

bool zerarBalanca() {

    if (alimentacaoEmAndamento || !balanca.is_ready()) {
        return false;
    }

    balanca.tare(10);

    balancaZerada = true;
    balancaCalibrada = false;

    return true;
}

// ==========================================
// CALIBRAR BALANÇA
// ==========================================

bool calibrarBalanca(float pesoConhecidoGramas) {

    if (alimentacaoEmAndamento) {
        return false;
    }

    if (!balancaZerada || pesoConhecidoGramas <= 0) {
        return false;
    }

    if (!balanca.is_ready()) {
        return false;
    }

    balanca.calibrate_scale(pesoConhecidoGramas, 10);

    balancaCalibrada = true;

    return true;
}

// ==========================================
// CANCELAR ALIMENTAÇÃO
// ==========================================

bool cancelarAlimentacao(DadosSistema& dados) {

    if (!alimentacaoEmAndamento) {
        return false;
    }

    fecharComporta();

    dados.alimentacao.comportaAberta = false;
    dados.alimentacao.estado = EstadoAlimentacao::AGUARDANDO;

    alimentacaoEmAndamento = false;

    return true;
}
