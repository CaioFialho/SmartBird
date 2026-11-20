#include "Comunicacao.h"
#include <string.h>

// Buffer interno de recepção.
static char bufferRecepcao[TAMANHO_BUFFER_MENSAGEM];

static size_t indiceRecepcao = 0;
static bool descartandoMensagem = false;
static bool overflowDetectado = false;

// ==========================================
// INICIALIZAÇÃO
// ==========================================

void inicializarComunicacao() {

#if SMARTBIRD_COMUNICACAO_USB
    // Serial USB já é inicializada no setup() do arquivo .ino.
    // Não chamar Serial.begin() novamente aqui.
#else
    // UART dedicada ao ESP8266.
    Serial1.begin(4800);
#endif

    indiceRecepcao = 0;
    descartandoMensagem = false;
    overflowDetectado = false;
}

// ==========================================
// RECEBER MENSAGEM
// ==========================================

bool receberMensagem(char* destino, size_t capacidade) {

    if (destino == nullptr || capacidade < TAMANHO_BUFFER_MENSAGEM) {
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

        // Ignora o CR usado em finais de linha CRLF.
        if (caractere == '\r') {
            continue;
        }

        // Se a mensagem exceder o limite, descarta até o próximo \n.
        if (descartandoMensagem) {
            continue;
        }

        if (indiceRecepcao >= MAX_BYTES_MENSAGEM) {

            indiceRecepcao = 0;
            descartandoMensagem = true;
            overflowDetectado = true;

            continue;
        }

        bufferRecepcao[indiceRecepcao] = caractere;
        indiceRecepcao++;
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

    if (tamanho == 0 || tamanho > MAX_BYTES_MENSAGEM) {
        return false;
    }

    // Não permite enviar várias mensagens em uma única chamada.
    if (strchr(mensagem, '\n') != nullptr ||
        strchr(mensagem, '\r') != nullptr) {
        return false;
    }

#if SMARTBIRD_COMUNICACAO_USB

    Serial.println("[COMUNICACAO] Enviando pela USB...");

    Serial.write(
        reinterpret_cast<const uint8_t*>(mensagem),
        tamanho
    );
    Serial.write('\n');

#else

    Serial.println();
    Serial.println("========== UART MEGA -> ESP8266 ==========");
    Serial.println("[UART TX] Mensagem que sera enviada:");
    Serial.println(mensagem);
    Serial.print("[UART TX] Bytes: ");
    Serial.println(tamanho);

    Serial1.write(
        reinterpret_cast<const uint8_t*>(mensagem),
        tamanho
    );

    Serial1.write('\n');

    Serial.println("[UART TX] Transmissao solicitada.");
    Serial.println("==========================================");

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