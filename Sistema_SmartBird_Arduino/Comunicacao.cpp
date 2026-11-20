#include "Comunicacao.h"
#include <string.h>

// ==========================================
// BUFFER DE RECEPÇÃO
// ==========================================

static char bufferRecepcao[TAMANHO_BUFFER_MENSAGEM];

static size_t indiceRecepcao = 0;
static bool descartandoMensagem = false;
static bool overflowDetectado = false;

// ==========================================
// INICIALIZAÇÃO
// ==========================================

void inicializarComunicacao() {

#if SMARTBIRD_COMUNICACAO_USB

    // A Serial USB é inicializada no setup() do .ino.

#else

    // UART dedicada ao ESP8266.
    Serial1.begin(9600);

#endif

    indiceRecepcao = 0;
    descartandoMensagem = false;
    overflowDetectado = false;
}

// ==========================================
// RECEBER MENSAGEM
// ==========================================

bool receberMensagem(char* destino, size_t capacidade) {

    if (destino == nullptr ||
        capacidade < TAMANHO_BUFFER_MENSAGEM) {
        return false;
    }

#if SMARTBIRD_COMUNICACAO_USB

    while (Serial.available() > 0) {
        char caractere = Serial.read();

#else

    while (Serial1.available() > 0) {
        char caractere = Serial1.read();

#endif

        // Final da mensagem.
        if (caractere == '\n') {

            if (descartandoMensagem) {
                descartandoMensagem = false;
                indiceRecepcao = 0;
                continue;
            }

            // Ignora mensagens vazias.
            if (indiceRecepcao == 0) {
                continue;
            }

            bufferRecepcao[indiceRecepcao] = '\0';

            memcpy(
                destino,
                bufferRecepcao,
                indiceRecepcao + 1
            );

            indiceRecepcao = 0;

            return true;
        }

        // Ignora CR.
        if (caractere == '\r') {
            continue;
        }

        // Descarta o restante de mensagens excedentes.
        if (descartandoMensagem) {
            continue;
        }

        // Proteção contra overflow.
        if (indiceRecepcao >= MAX_BYTES_MENSAGEM) {

            indiceRecepcao = 0;
            descartandoMensagem = true;
            overflowDetectado = true;

            continue;
        }

        bufferRecepcao[indiceRecepcao++] = caractere;
    }

    return false;
}

// ==========================================
// ENVIAR MENSAGEM
// ==========================================

bool enviarMensagem(const char* mensagem) {

    if (mensagem == nullptr) {
        return false;
    }

    size_t tamanho = strlen(mensagem);

    if (tamanho == 0 ||
        tamanho > MAX_BYTES_MENSAGEM) {
        return false;
    }

    // Não permite múltiplas mensagens na mesma chamada.
    if (strchr(mensagem, '\n') != nullptr ||
        strchr(mensagem, '\r') != nullptr) {
        return false;
    }

#if SMARTBIRD_COMUNICACAO_USB

    Serial.write(
        reinterpret_cast<const uint8_t*>(mensagem),
        tamanho
    );

    Serial.write('\n');

#else

    Serial1.write(
        reinterpret_cast<const uint8_t*>(mensagem),
        tamanho
    );

    Serial1.write('\n');

#endif

    return true;
}

// ==========================================
// CONSULTAR OVERFLOW
// ==========================================

bool houveOverflowComunicacao() {

    bool resultado = overflowDetectado;

    overflowDetectado = false;

    return resultado;
}