#ifndef COMUNICACAO_H
#define COMUNICACAO_H

#include <Arduino.h>

// Limite definido no protocolo: 1024 bytes.
constexpr size_t MAX_BYTES_MENSAGEM = 1024;
constexpr size_t TAMANHO_BUFFER_MENSAGEM = MAX_BYTES_MENSAGEM + 1;

// Inicializa a UART entre Mega e ESP8266.
void inicializarComunicacao();

// Verifica se chegou uma mensagem completa.
// Retorna true quando uma linha completa estiver disponível.
bool receberMensagem(char* destino, size_t capacidade);

// Envia uma mensagem terminada em \n.
bool enviarMensagem(const char* mensagem);

// Informa se ocorreu estouro do buffer desde a última consulta.
bool houveOverflowComunicacao();

#endif