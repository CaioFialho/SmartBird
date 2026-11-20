#ifndef PROTOCOLO_H
#define PROTOCOLO_H

#include <Arduino.h>
#include "Estado_SB.h"

// Recebe uma mensagem JSON completa, valida sua estrutura e encaminha
// a ação solicitada para a rotina de resposta correspondente.
// A mensagem deve ter sido recebida previamente pela camada Comunicacao.
void processarMensagem(const char* mensagem, DadosSistema& dados);

#endif