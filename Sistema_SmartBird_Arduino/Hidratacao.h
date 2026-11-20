#ifndef HIDRATACAO_H
#define HIDRATACAO_H
#include <Arduino.h>
#include "Estado_SB.h"

bool inicializarHidratacao();
bool iniciarHidratacao(DadosSistema& dados);
void atualizarHidratacao(DadosSistema& dados);
bool cancelarHidratacao(DadosSistema& dados);

#endif
