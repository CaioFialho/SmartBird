#ifndef MANUTENCAO_H
#define MANUTENCAO_H

#include <Arduino.h>
#include "Estado_SB.h"

// Inicializa o módulo
bool inicializarManutencao();

// Verifica se a limpeza está pendente
void atualizarManutencao(DadosSistema& dados);

// Registra uma limpeza realizada
bool confirmarLimpeza(DadosSistema& dados);

#endif
