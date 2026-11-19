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
// CONFIGURAÇÕES
// ==========================================

// Fator obtido nos testes físicos da célula de carga.
static constexpr float FATOR_CALIBRACAO = -1032.96f;

// Posições do servo.
static constexpr uint8_t ANGULO_FECHADO = 0;
static constexpr uint8_t ANGULO_ABERTO = 90;

// Intervalo entre leituras.
static constexpr unsigned long INTERVALO_LEITURA_MS = 250UL;

// Tempo máximo de uma operação.
static constexpr unsigned long TEMPO_LIMITE_MS = 15000UL;

// Tempo máximo de espera pelo HX711 na inicialização.
static constexpr unsigned long TEMPO_ESPERA_HX711_MS = 5000UL;

// ==========================================
// CONTROLE INTERNO
// ==========================================

static bool alimentacaoEmAndamento = false;
static bool balancaDisponivel = false;
static bool balancaZerada = false;
static bool balancaCalibrada = false;

static unsigned long inicioAlimentacaoMs = 0;
static unsigned long ultimaLeituraMs = 0;

// ==========================================
// CONTROLE DA COMPORTA
// ==========================================

static void fecharComporta() {
    servoComporta.write(ANGULO_FECHADO);
}

// ==========================================
// INICIALIZAÇÃO
// ==========================================

bool inicializarAlimentacao() {

    // Inicializa o servo e mantém a comporta fechada.
    servoComporta.attach(SERVO_COMPORTA);
    fecharComporta();

    // Inicializa o HX711.
    balanca.begin(
        HX711_DT_ALIMENTO,
        HX711_SCK_ALIMENTO
    );

    balanca.set_scale(FATOR_CALIBRACAO);

    Serial.println("Inicializando balanca...");

    unsigned long inicioEspera = millis();

    // Aguarda o HX711 ficar disponível.
    while (!balanca.is_ready()) {

        if (millis() - inicioEspera >= TEMPO_ESPERA_HX711_MS) {
            Serial.println("ERRO: HX711 nao encontrado.");
            balancaDisponivel = false;
            balancaCalibrada = false;
            return false;
        }

        delay(50);
    }

    // Faz a tara sem nenhum peso sobre a balança.
    Serial.println("Realizando tara...");

    balanca.tare(10);

    balancaZerada = true;
    balancaCalibrada = true;
    balancaDisponivel = true;

    Serial.println("Balanca inicializada e calibrada.");
    Serial.println("Fator: -1032.96");

    return true;
}

// ==========================================
// INICIAR ALIMENTAÇÃO
// ==========================================

bool iniciarAlimentacao(DadosSistema& dados) {

    if (alimentacaoEmAndamento) {
        return false;
    }

    if (!balancaDisponivel || !balancaCalibrada) {
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
// ATUALIZAR ALIMENTAÇÃO E BALANÇA
// ==========================================

void atualizarAlimentacao(DadosSistema& dados) {

    if (!balancaDisponivel) {
        return;
    }

    unsigned long agoraMs = millis();

    // Respeita o intervalo entre leituras.
    if (agoraMs - ultimaLeituraMs < INTERVALO_LEITURA_MS) {
        return;
    }

    // Verifica se o HX711 está pronto.
    if (!balanca.is_ready()) {
        return;
    }

    ultimaLeituraMs = agoraMs;

    // Obtém o peso atual em gramas.
    float pesoAtual = balanca.get_units(5);

    // Verifica se a leitura é válida.
    if (isnan(pesoAtual) || isinf(pesoAtual)) {

        if (alimentacaoEmAndamento) {
            fecharComporta();

            dados.alimentacao.comportaAberta = false;
            dados.alimentacao.estado = EstadoAlimentacao::ERRO;

            alimentacaoEmAndamento = false;
        }

        return;
    }

    // Atualiza o peso mesmo quando não está alimentando.
    dados.alimentacao.pesoAtualGramas = pesoAtual;

    // Se não existe alimentação em andamento, encerra aqui.
    if (!alimentacaoEmAndamento) {
        return;
    }

    // Verifica o limite de tempo da operação.
    if (agoraMs - inicioAlimentacaoMs >= TEMPO_LIMITE_MS) {

        fecharComporta();

        dados.alimentacao.comportaAberta = false;
        dados.alimentacao.estado = EstadoAlimentacao::ERRO;

        alimentacaoEmAndamento = false;

        return;
    }

    switch (dados.alimentacao.estado) {

        case EstadoAlimentacao::VERIFICANDO_PESO:

        case EstadoAlimentacao::ALIMENTANDO:

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

    if (alimentacaoEmAndamento || !balancaDisponivel) {
        return false;
    }

    if (!balanca.is_ready()) {
        return false;
    }

    balanca.tare(10);

    balancaZerada = true;
    balancaCalibrada = true;

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