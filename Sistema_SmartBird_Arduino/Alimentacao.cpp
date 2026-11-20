#include <HX711.h>
#include <Servo.h>
#include <math.h>

#include "Alimentacao.h"
#include "Hardware_SB.h"

namespace {
HX711 balanca;
Servo servoComporta;

constexpr float FATOR_CALIBRACAO_PADRAO = -1032.96f;
constexpr uint8_t ANGULO_FECHADO = 0;
constexpr uint8_t ANGULO_ABERTO = 90;
constexpr unsigned long INTERVALO_LEITURA_MS = 250UL;
constexpr unsigned long TEMPO_ESPERA_HX711_MS = 5000UL;

bool alimentacaoEmAndamento = false;
bool balancaDisponivel = false;
bool balancaCalibrada = false;
unsigned long inicioAlimentacaoMs = 0;
unsigned long ultimaLeituraMs = 0;

void fecharComporta(DadosSistema* dados = nullptr) {
    servoComporta.write(ANGULO_FECHADO);
    if (dados) dados->alimentacao.comportaAberta = false;
}
}

bool inicializarAlimentacao() {
    servoComporta.attach(SERVO_COMPORTA);
    servoComporta.write(ANGULO_FECHADO);

    balanca.begin(HX711_DT_ALIMENTO, HX711_SCK_ALIMENTO);
    balanca.set_scale(FATOR_CALIBRACAO_PADRAO);

    const unsigned long inicio = millis();
    while (!balanca.is_ready()) {
        if (millis() - inicio >= TEMPO_ESPERA_HX711_MS) {
            balancaDisponivel = false;
            balancaCalibrada = false;
            return false;
        }
        delay(20);
    }

    balanca.tare(10);
    balancaDisponivel = true;
    balancaCalibrada = true;
    return true;
}

bool iniciarAlimentacao(DadosSistema& dados) {
    if (alimentacaoEmAndamento || !balancaDisponivel || !balancaCalibrada)
        return false;

    if (!isfinite(dados.configuracaoAlimentacao.pesoAlvoGramas) ||
        dados.configuracaoAlimentacao.pesoAlvoGramas <= 0.0f ||
        dados.configuracaoAlimentacao.tempoMaximoMs == 0)
        return false;

    if (!balanca.is_ready()) {
        dados.alimentacao.leituraValida = false;
        return false;
    }

    alimentacaoEmAndamento = true;
    inicioAlimentacaoMs = millis();
    ultimaLeituraMs = 0;
    dados.alimentacao.estado = EstadoAlimentacao::VERIFICANDO_PESO;
    return true;
}

void atualizarAlimentacao(DadosSistema& dados) {
    if (!balancaDisponivel) {
        dados.alimentacao.leituraValida = false;
        if (alimentacaoEmAndamento) {
            fecharComporta(&dados);
            alimentacaoEmAndamento = false;
            dados.alimentacao.estado = EstadoAlimentacao::ERRO;
        }
        return;
    }

    const unsigned long agora = millis();

    // O timeout é verificado mesmo quando o HX711 não está pronto.
    if (alimentacaoEmAndamento &&
        agora - inicioAlimentacaoMs >= dados.configuracaoAlimentacao.tempoMaximoMs) {
        fecharComporta(&dados);
        alimentacaoEmAndamento = false;
        dados.alimentacao.estado = EstadoAlimentacao::ERRO;
        dados.alimentacao.leituraValida = false;
        return;
    }

    if (agora - ultimaLeituraMs < INTERVALO_LEITURA_MS) return;
    ultimaLeituraMs = agora;

    if (!balanca.is_ready()) {
        dados.alimentacao.leituraValida = false;
        if (alimentacaoEmAndamento) {
            fecharComporta(&dados);
            alimentacaoEmAndamento = false;
            dados.alimentacao.estado = EstadoAlimentacao::ERRO;
        }
        return;
    }

    const float peso = balanca.get_units(5);
    if (!isfinite(peso)) {
        dados.alimentacao.leituraValida = false;
        if (alimentacaoEmAndamento) {
            fecharComporta(&dados);
            alimentacaoEmAndamento = false;
            dados.alimentacao.estado = EstadoAlimentacao::ERRO;
        }
        return;
    }

    dados.alimentacao.pesoAtualGramas = peso;
    dados.alimentacao.leituraValida = true;

    if (!alimentacaoEmAndamento) return;

    if (peso >= dados.configuracaoAlimentacao.pesoAlvoGramas) {
        fecharComporta(&dados);
        alimentacaoEmAndamento = false;
        dados.alimentacao.estado = EstadoAlimentacao::FINALIZADO;
        dados.alimentacao.ultimaExecucaoMs = agora;
        return;
    }

    servoComporta.write(ANGULO_ABERTO);
    dados.alimentacao.comportaAberta = true;
    dados.alimentacao.estado = EstadoAlimentacao::ALIMENTANDO;
}

bool zerarBalanca() {
    if (alimentacaoEmAndamento || !balancaDisponivel || !balanca.is_ready())
        return false;
    balanca.tare(10);
    return true;
}

bool calibrarBalanca(float pesoConhecidoGramas) {
    if (alimentacaoEmAndamento || !balancaDisponivel ||
        !isfinite(pesoConhecidoGramas) || pesoConhecidoGramas <= 0.0f ||
        !balanca.is_ready())
        return false;

    // Compatível com a API usual da biblioteca bogde/HX711.
    const float leituraBruta = balanca.get_value(10);
    const float novoFator = leituraBruta / pesoConhecidoGramas;
    if (!isfinite(novoFator) || fabs(novoFator) < 0.001f) return false;

    balanca.set_scale(novoFator);
    balancaCalibrada = true;
    return true;
}

bool cancelarAlimentacao(DadosSistema& dados) {
    if (!alimentacaoEmAndamento) return false;
    fecharComporta(&dados);
    alimentacaoEmAndamento = false;
    dados.alimentacao.estado = EstadoAlimentacao::AGUARDANDO;
    return true;
}
