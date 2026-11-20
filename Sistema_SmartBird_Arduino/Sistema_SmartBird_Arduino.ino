#include "Controle_SB.h"
#include "Comunicacao.h"

void setup() {
#if SMARTBIRD_COMUNICACAO_USB
    Serial.begin(115200);
#else
    Serial.begin(115200); // Porta USB exclusiva para diagnóstico.
#endif
    inicializarControle();
}

void loop() {
    atualizarControle();
}
