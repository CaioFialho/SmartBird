#include "Hidratacao.h"
#include "Hardware_SB.h"

// ==========================================
// CONFIGURAÇÕES
// ==========================================

static constexpr unsigned long TEMPO_LIMITE_MS = 15000UL;

// ==========================================
// CONTROLE INTERNO
// ==========================================

static bool hidratacaoEmAndamento = false;
static bool falhaPorTempo = false;

static unsigned long inicioHidratacaoMs = 0;

// ==========================================
// CONTROLE DO RELÉ
// ==========================================

static void ligarBomba() {
    digitalWrite(RELE_BOMBA_AGUA, HIGH);
}

static void desligarBomba() {
    digitalWrite(RELE_BOMBA_AGUA, LOW);
}

// ==========================================
// INICIALIZAÇÃO
// ==========================================

bool inicializarHidratacao() {

    pinMode(RELE_BOMBA_AGUA, OUTPUT);
    desligarBomba();

    pinMode(SENSOR_BOIA_AGUA, INPUT_PULLUP);

    hidratacaoEmAndamento = false;
    falhaPorTempo = false;

    return true;
}

// ==========================================
// ATUALIZAR HIDRATAÇÃO
// ==========================================

void atualizarHidratacao(DadosSistema& dados) {

    unsigned long agoraMs = millis();

    // LOW = nível baixo
    // HIGH = nível adequado

    bool nivelBaixo = digitalRead(SENSOR_BOIA_AGUA) == LOW;

    // ----------------------------------
    // NÍVEL ADEQUADO
    // ----------------------------------

    if (!nivelBaixo) {

        desligarBomba();

        dados.hidratacao.bombaLigada = false;

        if (hidratacaoEmAndamento) {

            dados.hidratacao.estado = EstadoHidratacao::FINALIZADO;
            dados.hidratacao.ultimaExecucaoMs = agoraMs;

        } else {

            dados.hidratacao.estado = EstadoHidratacao::AGUARDANDO;
        }

        hidratacaoEmAndamento = false;
        falhaPorTempo = false;

        return;
    }

    // ----------------------------------
    // FALHA POR TEMPO EXCEDIDO
    // ----------------------------------

    if (falhaPorTempo) {

        desligarBomba();

        dados.hidratacao.bombaLigada = false;
        dados.hidratacao.estado = EstadoHidratacao::ERRO;

        return;
    }

    // ----------------------------------
    // VERIFICAR RESERVATÓRIO DE ORIGEM
    // ----------------------------------

    dados.hidratacao.estado = EstadoHidratacao::VERIFICANDO_NIVEL;

    bool leituraValida =
        dados.reservatorioAgua.dados.leituraValida;

    bool aguaDisponivel =
        !dados.reservatorioAgua.dados.nivelBaixo;

    if (!leituraValida || !aguaDisponivel) {

        desligarBomba();

        dados.hidratacao.bombaLigada = false;
        dados.hidratacao.estado = EstadoHidratacao::ERRO;

        hidratacaoEmAndamento = false;

        return;
    }

    // ----------------------------------
    // INICIAR ABASTECIMENTO
    // ----------------------------------

    if (!hidratacaoEmAndamento) {

        ligarBomba();

        hidratacaoEmAndamento = true;
        inicioHidratacaoMs = agoraMs;

        dados.hidratacao.bombaLigada = true;
        dados.hidratacao.estado = EstadoHidratacao::ABASTECENDO;

        return;
    }

    // ----------------------------------
    // MONITORAR ABASTECIMENTO
    // ----------------------------------

    dados.hidratacao.estado = EstadoHidratacao::ABASTECENDO;

    if (agoraMs - inicioHidratacaoMs >= TEMPO_LIMITE_MS) {

        desligarBomba();

        dados.hidratacao.bombaLigada = false;
        dados.hidratacao.estado = EstadoHidratacao::ERRO;

        hidratacaoEmAndamento = false;
        falhaPorTempo = true;
    }
}

// ==========================================
// CANCELAR HIDRATAÇÃO
// ==========================================

bool cancelarHidratacao(DadosSistema& dados) {

    if (!hidratacaoEmAndamento) {
        return false;
    }

    desligarBomba();

    dados.hidratacao.bombaLigada = false;
    dados.hidratacao.estado = EstadoHidratacao::AGUARDANDO;

    hidratacaoEmAndamento = false;
    falhaPorTempo = false;

    return true;
}