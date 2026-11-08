#ifndef RESERVATORIOS_H
#define RESERVATORIOS_H

#include <Arduino.h>
#include "Estado_SB.h"

// Inicializa os sensores dos reservatórios
void inicializarReservatorios();

// Atualiza as leituras dos reservatórios
void atualizarReservatorios(DadosSistema& dados);

#endif
