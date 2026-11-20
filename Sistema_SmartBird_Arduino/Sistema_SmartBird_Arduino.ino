#include "Controle_SB.h"

void setup() {
    Serial.begin(115200);

    inicializarControle();
}

void loop() {
    atualizarControle();
}