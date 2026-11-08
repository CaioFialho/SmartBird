
#include "Estado_SB.h"
#include "Reservatorios.h"

DadosSistema dados;

void setup() {

    Serial.begin(115200);

    // Configuracao do reservatorio de alimento
    dados.reservatorioAlimento.configuracao.distanciaNivelCheioCm = 5.0;
    dados.reservatorioAlimento.configuracao.distanciaNivelVazioCm = 35.0;
    dados.reservatorioAlimento.configuracao.nivelMinimoPercentual = 20.0;

    // Configuracao do reservatorio de agua
    dados.reservatorioAgua.configuracao.distanciaNivelCheioCm = 5.0;
    dados.reservatorioAgua.configuracao.distanciaNivelVazioCm = 30.0;
    dados.reservatorioAgua.configuracao.nivelMinimoPercentual = 20.0;

    // Inicializacao dos reservatorios
    inicializarReservatorios();

    Serial.println("================================");
    Serial.println("       SMARTBIRD INICIADO");
    Serial.println("================================");

    Serial.println("Inicializacao do sistema concluida.");
}

void loop() {

    atualizarReservatorios(dados);

    // Futuramente, o Controle_SB sera chamado aqui.
}
