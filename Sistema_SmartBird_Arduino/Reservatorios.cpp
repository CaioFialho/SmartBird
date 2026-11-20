#include <Arduino.h>
#include <math.h>
#include "Reservatorios.h"
#include "Hardware_SB.h"

namespace {
constexpr unsigned long INTERVALO_LEITURA_MS = 2000UL;
constexpr unsigned long TIMEOUT_ECHO_US = 25000UL;
constexpr unsigned long INTERVALO_ANTICROSSTALK_MS = 60UL;
unsigned long ultimaAtualizacaoMs = 0;

float lerDistancia(uint8_t trig, uint8_t echo) {
    digitalWrite(trig, LOW);
    delayMicroseconds(3);
    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, LOW);

    const unsigned long duracao = pulseIn(echo, HIGH, TIMEOUT_ECHO_US);
    if (duracao == 0) return -1.0f;

    const float distancia = (duracao * 0.0343f) / 2.0f;
    return isfinite(distancia) ? distancia : -1.0f;
}

void invalidarLeitura(Reservatorio& r) {
    // Mantém a última medição como histórico, mas invalida sua confiabilidade.
    // O consumidor deve sempre verificar leituraValida antes de usar nivelBaixo.
    r.dados.leituraValida = false;
}

void processarLeitura(Reservatorio& r, float distancia) {
    const float cheio = r.configuracao.distanciaNivelCheioCm;
    const float vazio = r.configuracao.distanciaNivelVazioCm;
    const float minimo = r.configuracao.nivelMinimoPercentual;

    if (!isfinite(distancia) || distancia < 2.0f || distancia > 400.0f ||
        !isfinite(cheio) || !isfinite(vazio) || !isfinite(minimo) ||
        cheio <= 0.0f || vazio <= cheio || minimo < 0.0f || minimo > 100.0f) {
        invalidarLeitura(r);
        return;
    }

    float nivel = ((vazio - distancia) / (vazio - cheio)) * 100.0f;
    nivel = constrain(nivel, 0.0f, 100.0f);

    r.dados.distanciaSensorCm = distancia;
    r.dados.nivelAtualPercentual = nivel;
    r.dados.leituraValida = true;
    r.dados.nivelBaixo = nivel <= minimo;
}
}

void inicializarReservatorios() {
    pinMode(TRIG_ALIMENTO, OUTPUT);
    pinMode(ECHO_ALIMENTO, INPUT);
    pinMode(TRIG_AGUA, OUTPUT);
    pinMode(ECHO_AGUA, INPUT);
    digitalWrite(TRIG_ALIMENTO, LOW);
    digitalWrite(TRIG_AGUA, LOW);
    ultimaAtualizacaoMs = 0;
}

void atualizarReservatorios(DadosSistema& dados) {
    const unsigned long agora = millis();
    if (ultimaAtualizacaoMs != 0 &&
        agora - ultimaAtualizacaoMs < INTERVALO_LEITURA_MS) return;

    ultimaAtualizacaoMs = agora;

    processarLeitura(
        dados.reservatorioAlimento,
        lerDistancia(TRIG_ALIMENTO, ECHO_ALIMENTO)
    );

    delay(INTERVALO_ANTICROSSTALK_MS);

    processarLeitura(
        dados.reservatorioAgua,
        lerDistancia(TRIG_AGUA, ECHO_AGUA)
    );
}
