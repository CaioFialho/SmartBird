#include "Controle_SB.h"

#include "Estado_SB.h"
#include "Reservatorios.h"
#include "Alimentacao.h"
#include "Hidratacao.h"
#include "Tempo.h"
#include "Manutencao.h"
#include "Comunicacao.h"
#include "Protocolo.h"
#include "Interface.h"

static DadosSistema dados;
static bool controleInicializado = false;
static bool falhaInicializacao = false;
static unsigned long ultimaVerificacaoRTC = 0;
static bool rtcDisponivelAtual = false;
static int8_t horarioAlimentacaoEmExecucao = -1;
static int8_t horarioHidratacaoEmExecucao = -1;

namespace {
void atualizarEstadoGeral() {
    const bool erroModulo =
        dados.alimentacao.estado == EstadoAlimentacao::ERRO ||
        dados.hidratacao.estado == EstadoHidratacao::ERRO;

    const bool rtcOk = rtcDisponivelAtual;

    const bool alertaReservatorio =
        !dados.reservatorioAlimento.dados.leituraValida ||
        !dados.reservatorioAgua.dados.leituraValida ||
        dados.reservatorioAlimento.dados.nivelBaixo ||
        dados.reservatorioAgua.dados.nivelBaixo;

    if (falhaInicializacao || erroModulo) {
        dados.estado = EstadoSistema::ERRO;
    } else if (!rtcOk || alertaReservatorio || dados.manutencao.alertaAtivo) {
        dados.estado = EstadoSistema::ALERTA;
    } else {
        dados.estado = EstadoSistema::OPERACIONAL;
    }
}

void concluirAgendamento(AgendaDiaria& agenda, int8_t& indice) {
    if (indice < 0) return;
    DateTime agora;
    if (obterDataHora(agora)) {
        registrarExecucaoConcluida(agenda, static_cast<uint8_t>(indice), agora);
    }
    indice = -1;
}

void processarAgendamentos() {
    const unsigned long millisAgora = millis();
    if (ultimaVerificacaoRTC != 0 &&
        millisAgora - ultimaVerificacaoRTC < 1000UL) return;
    ultimaVerificacaoRTC = millisAgora;

    DateTime agora;
    rtcDisponivelAtual = obterDataHora(agora);
    if (!rtcDisponivelAtual) return;

    verificarAgendamentos(dados, agora);

    // Conclusão dos agendamentos que já iniciaram.
    if (horarioAlimentacaoEmExecucao >= 0) {
        if (dados.alimentacao.estado == EstadoAlimentacao::FINALIZADO) {
            concluirAgendamento(dados.agendaAlimentacao, horarioAlimentacaoEmExecucao);
        } else if (dados.alimentacao.estado == EstadoAlimentacao::ERRO) {
            dados.agendaAlimentacao.horarios[horarioAlimentacaoEmExecucao].registro.estado =
                EstadoExecucao::RECUSADO;
            horarioAlimentacaoEmExecucao = -1;
        }
    }

    if (horarioHidratacaoEmExecucao >= 0) {
        if (dados.hidratacao.estado == EstadoHidratacao::FINALIZADO) {
            concluirAgendamento(dados.agendaHidratacao, horarioHidratacaoEmExecucao);
        } else if (dados.hidratacao.estado == EstadoHidratacao::ERRO) {
            dados.agendaHidratacao.horarios[horarioHidratacaoEmExecucao].registro.estado =
                EstadoExecucao::RECUSADO;
            horarioHidratacaoEmExecucao = -1;
        }
    }

    // Alimentação: só inicia quando o módulo está livre.
    if (horarioAlimentacaoEmExecucao < 0 &&
        (dados.alimentacao.estado == EstadoAlimentacao::AGUARDANDO ||
         dados.alimentacao.estado == EstadoAlimentacao::FINALIZADO)) {
        for (uint8_t i = 0; i < MAX_HORARIOS_DIARIOS; ++i) {
            Horario& h = dados.agendaAlimentacao.horarios[i];
            if (!h.ativo || h.registro.estado != EstadoExecucao::PENDENTE) continue;
            if (iniciarAlimentacao(dados)) {
                h.registro.estado = EstadoExecucao::EXECUTANDO;
                horarioAlimentacaoEmExecucao = static_cast<int8_t>(i);
            } else if (dados.alimentacao.estado == EstadoAlimentacao::ERRO) {
                h.registro.estado = EstadoExecucao::RECUSADO;
            }
            break;
        }
    }

    // Hidratação: a boia tem prioridade; se o nível já estiver adequado,
    // o horário é considerado atendido sem ligar a bomba.
    if (horarioHidratacaoEmExecucao < 0) {
        for (uint8_t i = 0; i < MAX_HORARIOS_DIARIOS; ++i) {
            Horario& h = dados.agendaHidratacao.horarios[i];
            if (!h.ativo || h.registro.estado != EstadoExecucao::PENDENTE) continue;

            if (dados.hidratacao.estadoRecipiente == EstadoRecipienteAgua::OK) {
                h.registro.estado = EstadoExecucao::EXECUTANDO;
                registrarExecucaoConcluida(dados.agendaHidratacao, i, agora);
            } else if (dados.hidratacao.estadoRecipiente == EstadoRecipienteAgua::BAIXO &&
                       dados.hidratacao.estado == EstadoHidratacao::ABASTECENDO) {
                // O módulo pode já ter iniciado o abastecimento automaticamente.
                h.registro.estado = EstadoExecucao::EXECUTANDO;
                horarioHidratacaoEmExecucao = static_cast<int8_t>(i);
            } else if (dados.hidratacao.estadoRecipiente == EstadoRecipienteAgua::BAIXO &&
                       dados.hidratacao.estado == EstadoHidratacao::AGUARDANDO) {
                if (iniciarHidratacao(dados)) {
                    h.registro.estado = EstadoExecucao::EXECUTANDO;
                    horarioHidratacaoEmExecucao = static_cast<int8_t>(i);
                }
            }
            break;
        }
    }
}
}

void inicializarControle() {
    if (controleInicializado) return;

    dados.reservatorioAlimento.configuracao.distanciaNivelCheioCm = 5.0f;
    dados.reservatorioAlimento.configuracao.distanciaNivelVazioCm = 35.0f;
    dados.reservatorioAlimento.configuracao.nivelMinimoPercentual = 20.0f;

    dados.reservatorioAgua.configuracao.distanciaNivelCheioCm = 5.0f;
    dados.reservatorioAgua.configuracao.distanciaNivelVazioCm = 30.0f;
    dados.reservatorioAgua.configuracao.nivelMinimoPercentual = 20.0f;

    falhaInicializacao = false;
    inicializarReservatorios();

    if (!inicializarAlimentacao()) falhaInicializacao = true;
    if (!inicializarHidratacao()) falhaInicializacao = true;
    if (!inicializarTempo()) {
        // RTC indisponível gera alerta, não impede funções locais manuais.
    } else {
        DateTime agora;
        rtcDisponivelAtual = obterDataHora(agora);
    }
    if (!inicializarManutencao()) falhaInicializacao = true;

    inicializarComunicacao();
    inicializarInterface(dados);
    controleInicializado = true;

    atualizarReservatorios(dados);
    atualizarAlimentacao(dados);
    atualizarHidratacao(dados);
    atualizarManutencao(dados);
    atualizarEstadoGeral();
}

void atualizarControle() {
    if (!controleInicializado) return;

    char mensagem[TAMANHO_BUFFER_MENSAGEM];

if (receberMensagem(mensagem, sizeof(mensagem))) {

    Serial.println();
    Serial.println("[UART] Mensagem recebida:");
    Serial.println(mensagem);

    Serial.println("[PROTOCOLO] Encaminhando para processamento...");

    processarMensagem(mensagem, dados);

    Serial.println("[PROTOCOLO] Processamento finalizado.");
}

if (houveOverflowComunicacao()) {
    Serial.println("[ERRO] Overflow na comunicacao!");
}

    if (houveOverflowComunicacao()) {
#if !SMARTBIRD_COMUNICACAO_USB
        Serial.println(F("AVISO: mensagem serial excedeu o limite."));
#endif
    }

    atualizarReservatorios(dados);
    atualizarAlimentacao(dados);
    atualizarHidratacao(dados);
    atualizarManutencao(dados);
    processarAgendamentos();
    atualizarEstadoGeral();
    atualizarInterface(dados);
}
