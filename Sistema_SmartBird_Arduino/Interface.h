#ifndef INTERFACE_SB_H
#define INTERFACE_SB_H

#include <Arduino.h>
#include "Estado_SB.h"

// Inicializa LCD, botoes, LEDs e buzzer.
void inicializarInterface(DadosSistema& dados);

// Atualiza navegacao, telas, indicadores e alertas sem bloquear o loop.
void atualizarInterface(DadosSistema& dados);

#endif
