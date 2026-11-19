#include "Estado_SB.h"
#include "Reservatorios.h"
#include "Alimentacao.h"

DadosSistema dados;

void setup() {

    Serial.begin(115200);

    // Configuração do reservatório de alimento.
    dados.reservatorioAlimento.configuracao.distanciaNivelCheioCm = 5.0;
    dados.reservatorioAlimento.configuracao.distanciaNivelVazioCm = 35.0;
    dados.reservatorioAlimento.configuracao.nivelMinimoPercentual = 20.0;

    // Configuração do reservatório de água.
    dados.reservatorioAgua.configuracao.distanciaNivelCheioCm = 5.0;
    dados.reservatorioAgua.configuracao.distanciaNivelVazioCm = 30.0;
    dados.reservatorioAgua.configuracao.nivelMinimoPercentual = 20.0;

    Serial.println("================================");
    Serial.println("       SMARTBIRD INICIANDO");
    Serial.println("================================");

    // Inicialização dos reservatórios.
    inicializarReservatorios();

    // Inicialização da alimentação e da balança.
    if (!inicializarAlimentacao()) {
        Serial.println("ERRO NA INICIALIZACAO DA BALANCA.");
        dados.estado = EstadoSistema::ERRO;
    }

    Serial.println("Inicializacao concluida.");
}

void loop() {

    // Atualiza os sensores dos reservatórios.
    atualizarReservatorios(dados);

    // Atualiza a balança e o módulo de alimentação.
    atualizarAlimentacao(dados);

    // Futuramente, o Controle_SB será chamado aqui.
}