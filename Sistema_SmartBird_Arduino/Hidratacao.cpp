#include "Hidratacao.h"
#include "Hardware_SB.h"

namespace {
bool hidratacaoEmAndamento = false;
bool falhaPorTempo = false;
unsigned long inicioHidratacaoMs = 0;

void ligarBomba() { digitalWrite(RELE_BOMBA_AGUA, HIGH); }
void desligarBomba() { digitalWrite(RELE_BOMBA_AGUA, LOW); }

bool reservatorioOrigemDisponivel(const DadosSistema& dados) {
    return dados.reservatorioAgua.dados.leituraValida &&
           !dados.reservatorioAgua.dados.nivelBaixo;
}
}

bool inicializarHidratacao() {
    pinMode(RELE_BOMBA_AGUA, OUTPUT);
    desligarBomba();

    pinMode(SENSOR_BOIA_AGUA, INPUT_PULLUP);
    hidratacaoEmAndamento = false;
    falhaPorTempo = false;
    return true;
}

bool iniciarHidratacao(DadosSistema& dados) {
    if (hidratacaoEmAndamento || falhaPorTempo) return false;
    if (dados.hidratacao.estadoRecipiente != EstadoRecipienteAgua::BAIXO)
        return false;
    if (!reservatorioOrigemDisponivel(dados)) return false;
    if (dados.configuracaoHidratacao.tempoMaximoBombaMs == 0) return false;

    inicioHidratacaoMs = millis();
    hidratacaoEmAndamento = true;
    ligarBomba();
    dados.hidratacao.bombaLigada = true;
    dados.hidratacao.estado = EstadoHidratacao::ABASTECENDO;
    return true;
}

void atualizarHidratacao(DadosSistema& dados) {
    const unsigned long agora = millis();

    // INPUT_PULLUP: contato fechado para GND = LOW = nível baixo.
    const bool nivelBaixo = digitalRead(SENSOR_BOIA_AGUA) == LOW;
    dados.hidratacao.estadoRecipiente =
        nivelBaixo ? EstadoRecipienteAgua::BAIXO : EstadoRecipienteAgua::OK;

    if (!nivelBaixo) {
        desligarBomba();
        dados.hidratacao.bombaLigada = false;
        if (hidratacaoEmAndamento) {
            dados.hidratacao.estado = EstadoHidratacao::FINALIZADO;
            dados.hidratacao.ultimaExecucaoMs = agora;
        } else if (!falhaPorTempo) {
            dados.hidratacao.estado = EstadoHidratacao::AGUARDANDO;
        }
        hidratacaoEmAndamento = false;
        falhaPorTempo = false;
        return;
    }

    if (falhaPorTempo) {
        desligarBomba();
        dados.hidratacao.bombaLigada = false;
        dados.hidratacao.estado = EstadoHidratacao::ERRO;
        return;
    }

    if (!reservatorioOrigemDisponivel(dados)) {
        if (hidratacaoEmAndamento) {
            desligarBomba();
            hidratacaoEmAndamento = false;
        }
        dados.hidratacao.bombaLigada = false;
        dados.hidratacao.estado = EstadoHidratacao::ERRO;
        return;
    }

    // Reposição automática sempre que a boia indicar nível baixo.
    if (!hidratacaoEmAndamento) {
        if (dados.configuracaoHidratacao.tempoMaximoBombaMs == 0) {
            dados.hidratacao.estado = EstadoHidratacao::ERRO;
            return;
        }
        inicioHidratacaoMs = agora;
        hidratacaoEmAndamento = true;
        ligarBomba();
        dados.hidratacao.bombaLigada = true;
        dados.hidratacao.estado = EstadoHidratacao::ABASTECENDO;
        return;
    }

    if (agora - inicioHidratacaoMs >=
        dados.configuracaoHidratacao.tempoMaximoBombaMs) {
        desligarBomba();
        hidratacaoEmAndamento = false;
        falhaPorTempo = true;
        dados.hidratacao.bombaLigada = false;
        dados.hidratacao.estado = EstadoHidratacao::ERRO;
    } else {
        dados.hidratacao.estado = EstadoHidratacao::ABASTECENDO;
        dados.hidratacao.bombaLigada = true;
    }
}

bool cancelarHidratacao(DadosSistema& dados) {
    if (!hidratacaoEmAndamento) return false;
    desligarBomba();
    hidratacaoEmAndamento = false;
    falhaPorTempo = false;
    dados.hidratacao.bombaLigada = false;
    dados.hidratacao.estado = EstadoHidratacao::AGUARDANDO;
    return true;
}
