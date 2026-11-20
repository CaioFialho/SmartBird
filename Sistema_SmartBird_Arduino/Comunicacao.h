#ifndef COMUNICACAO_H
#define COMUNICACAO_H

#include <Arduino.h>

// Seleção da comunicação:
//
// 0 = Serial1 (ESP8266, 9600 baud)
// 1 = Serial USB (Monitor Serial, 115200 baud)

#define SMARTBIRD_COMUNICACAO_USB 0

// Limite máximo de uma mensagem.
constexpr size_t MAX_BYTES_MENSAGEM = 1024;
constexpr size_t TAMANHO_BUFFER_MENSAGEM = MAX_BYTES_MENSAGEM + 1;

// Inicializa a comunicação.
void inicializarComunicacao();

// Recebe uma mensagem completa terminada em \n.
bool receberMensagem(char* destino, size_t capacidade);

// Envia uma mensagem terminada em \n.
bool enviarMensagem(const char* mensagem);

// Consulta e limpa o indicador de overflow.
bool houveOverflowComunicacao();

#endif