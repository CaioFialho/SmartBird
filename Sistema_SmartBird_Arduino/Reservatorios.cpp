
#include <Arduino.h>

#include "Reservatorios.h"
#include "Hardware_SB.h"

// ==========================================
// INICIALIZACAO DOS RESERVATORIOS
// ==========================================

void inicializarReservatorios() {

    pinMode(TRIG_ALIMENTO, OUTPUT);
    pinMode(ECHO_ALIMENTO, INPUT);

    pinMode(TRIG_AGUA, OUTPUT);
    pinMode(ECHO_AGUA, INPUT);

    digitalWrite(TRIG_ALIMENTO, LOW);
    digitalWrite(TRIG_AGUA, LOW);
}

// ==========================================
// LEITURA DA DISTANCIA DOS SENSORES
// ==========================================

static float lerDistancia(uint8_t pinoTrig, uint8_t pinoEcho) {

    digitalWrite(pinoTrig, LOW);
    delayMicroseconds(2);

    digitalWrite(pinoTrig, HIGH);
    delayMicroseconds(10);

    digitalWrite(pinoTrig, LOW);

    unsigned long duracao = pulseIn(pinoEcho, HIGH, 30000UL);

    if (duracao == 0) {
        return -1.0;
    }

    float distancia = (duracao * 0.0343) / 2.0;

    return distancia;
}

// ==========================================
// PROCESSAMENTO DA LEITURA
// ==========================================

static void processarLeitura(Reservatorio& reservatorio, float distancia) {

    // Verifica se a leitura do sensor e valida
    if (distancia < 2.0 || distancia > 400.0) {
        reservatorio.dados.leituraValida = false;
        return;
    }

    // Recupera os valores de calibracao
    float distanciaCheio =
        reservatorio.configuracao.distanciaNivelCheioCm;

    float distanciaVazio =
        reservatorio.configuracao.distanciaNivelVazioCm;

    float nivelMinimo =
        reservatorio.configuracao.nivelMinimoPercentual;

    // Verifica se a calibracao e valida
    if (distanciaCheio <= 0 ||
        distanciaVazio <= distanciaCheio ||
        nivelMinimo < 0 ||
        nivelMinimo > 100) {

        reservatorio.dados.leituraValida = false;
        return;
    }

    // Calcula o percentual de preenchimento
    float nivel = ((distanciaVazio - distancia) /
                  (distanciaVazio - distanciaCheio)) * 100.0;

    // Limita o resultado entre 0% e 100%
    nivel = constrain(nivel, 0.0, 100.0);

    // Atualiza os dados do reservatorio
    reservatorio.dados.distanciaSensorCm = distancia;
    reservatorio.dados.nivelAtualPercentual = nivel;
    reservatorio.dados.leituraValida = true;

    // Identifica se o nivel esta baixo
    reservatorio.dados.nivelBaixo =
        nivel <= nivelMinimo;
}

// ==========================================
// ATUALIZACAO DOS RESERVATORIOS
// ==========================================

void atualizarReservatorios(DadosSistema& dados) {

    // Leitura do reservatorio de alimento
    float distanciaAlimento = lerDistancia(
        TRIG_ALIMENTO,
        ECHO_ALIMENTO
    );

    processarLeitura(
        dados.reservatorioAlimento,
        distanciaAlimento
    );

    // Intervalo para evitar interferencia entre sensores
    delay(60);

    // Leitura do reservatorio de agua
    float distanciaAgua = lerDistancia(
        TRIG_AGUA,
        ECHO_AGUA
    );

    processarLeitura(
        dados.reservatorioAgua,
        distanciaAgua
    );
}
