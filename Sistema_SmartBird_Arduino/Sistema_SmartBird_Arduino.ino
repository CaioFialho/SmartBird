
#include "Estado_SB.h"
#include "Reservatorios.h"
#include "Alimentacao.h"
#include "Comunicacao.h"

DadosSistema dados;

void setup() {

    Serial.begin(115200);

    // Inicializa comunicação com o ESP8266.
    inicializarComunicacao();

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

    inicializarReservatorios();

    if (!inicializarAlimentacao()) {
        Serial.println("ERRO NA INICIALIZACAO DA BALANCA.");
        dados.estado = EstadoSistema::ERRO;
    }

    Serial.println("Comunicacao UART inicializada.");
    Serial.println("Inicializacao concluida.");
}

void loop() {

    // Atualiza os módulos físicos.
    atualizarReservatorios(dados);
    atualizarAlimentacao(dados);

    // Verifica mensagens recebidas do ESP8266.
    char mensagem[TAMANHO_BUFFER_MENSAGEM];

    if (receberMensagem(mensagem, sizeof(mensagem))) {

        Serial.println("[ESP8266 -> MEGA]");
        Serial.println(mensagem);
    }

    // Verifica se alguma mensagem excedeu o limite.
    if (houveOverflowComunicacao()) {
        Serial.println("ERRO: mensagem UART excedeu 1024 bytes.");
    }
}
