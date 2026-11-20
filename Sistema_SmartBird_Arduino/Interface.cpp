#include "Interface.h"

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <stdio.h>
#include <string.h>

#include "Hardware_SB.h"
#include "Tempo.h"
#include "Manutencao.h"
#include "Alimentacao.h"
#include "Hidratacao.h"

namespace {
// Endereco comum de LCD I2C. Ajustar para 0x3F se o modulo usar esse endereco.
LiquidCrystal_I2C lcd(0x27, 16, 2);


constexpr unsigned long DEBOUNCE_MS = 35UL;
constexpr unsigned long INTERVALO_TELA_MS = 250UL;
constexpr unsigned long INTERVALO_LED_MS = 500UL;
constexpr unsigned long DURACAO_BIP_MS = 65UL;

enum class Tela : uint8_t { HOME, RESERVATORIOS, POTINHOS, PROXIMA_ACAO, MANUTENCAO, CONECTIVIDADE };
enum class ModoMenu : uint8_t { NENHUM, MODULO_AGENDA, LISTA_HORARIOS, EDITAR_HORARIO, CONFIRMAR_HORARIO, CONFIRMAR_LIMPEZA, MENSAGEM };
enum class ModuloAgenda : uint8_t { ALIMENTACAO, HIDRATACAO };

struct Botao {
    uint8_t pino;
    bool leituraAnterior = HIGH;
    bool estadoEstavel = HIGH;
    unsigned long ultimaMudanca = 0;
    bool pressionado = false;
    explicit Botao(uint8_t p) : pino(p) {}
};

Botao botoes[3] = {Botao(BOTAO_ANTERIOR), Botao(BOTAO_OK), Botao(BOTAO_PROXIMO)};
Tela telaAtual = Tela::HOME;
ModoMenu modoMenu = ModoMenu::NENHUM;
ModuloAgenda moduloSelecionado = ModuloAgenda::ALIMENTACAO;
uint8_t indiceHorario = 0;
uint8_t horaEditada = 8;
uint8_t minutoEditado = 0;
uint8_t opcaoConfirmacao = 0;
unsigned long ultimaAtualizacaoTela = 0;
unsigned long ultimaAtualizacaoLed = 0;
unsigned long inicioMensagem = 0;
unsigned long inicioBip = 0;
unsigned long proximoBip = 0;
uint8_t quantidadeBips = 0;
uint8_t bipsRestantes = 0;
bool buzzerEmite = false;
bool ledPisca = false;
bool telaSuja = true;
char mensagemLinha1[17] = "";
char mensagemLinha2[17] = "";

AgendaDiaria& agendaSelecionada(DadosSistema& dados) {
    return moduloSelecionado == ModuloAgenda::ALIMENTACAO ? dados.agendaAlimentacao : dados.agendaHidratacao;
}

const char* nomeModulo() {
    return moduloSelecionado == ModuloAgenda::ALIMENTACAO ? "Alimentacao" : "Hidratacao";
}

void bip(uint16_t frequencia, uint8_t quantidade) {
    noTone(BUZZER_INTERFACE);
    quantidadeBips = quantidade;
    bipsRestantes = quantidade;
    proximoBip = millis();
    tone(BUZZER_INTERFACE, frequencia);
    buzzerEmite = true;
    inicioBip = millis();
}

void atualizarBuzzer() {
    if (bipsRestantes == 0) return;
    unsigned long agora = millis();
    if (buzzerEmite && agora - inicioBip >= DURACAO_BIP_MS) {
        noTone(BUZZER_INTERFACE);
        buzzerEmite = false;
        bipsRestantes--;
        proximoBip = agora + 90UL;
        return;
    }
    if (!buzzerEmite && bipsRestantes > 0 && agora >= proximoBip) {
        tone(BUZZER_INTERFACE, 2200);
        buzzerEmite = true;
        inicioBip = agora;
    }
}

bool lerBotao(uint8_t indice) {
    Botao& b = botoes[indice];
    bool leitura = digitalRead(b.pino);
    unsigned long agora = millis();
    if (leitura != b.leituraAnterior) {
        b.leituraAnterior = leitura;
        b.ultimaMudanca = agora;
    }
    if (agora - b.ultimaMudanca >= DEBOUNCE_MS && leitura != b.estadoEstavel) {
        b.estadoEstavel = leitura;
        if (b.estadoEstavel == LOW) {
            b.pressionado = true;
        }
    }
    bool resultado = b.pressionado;
    b.pressionado = false;
    return resultado;
}

void escreverLinha(uint8_t linha, const char* texto) {
    char buffer[17];
    snprintf(buffer, sizeof(buffer), "%-16.16s", texto ? texto : "");
    lcd.setCursor(0, linha);
    lcd.print(buffer);
}

void mostrarMensagem(const char* linha1, const char* linha2) {
    snprintf(mensagemLinha1, sizeof(mensagemLinha1), "%-16.16s", linha1);
    snprintf(mensagemLinha2, sizeof(mensagemLinha2), "%-16.16s", linha2);
    modoMenu = ModoMenu::MENSAGEM;
    inicioMensagem = millis();
    telaSuja = true;
}

bool haOperacaoAtiva(const DadosSistema& d) {
    return d.alimentacao.estado == EstadoAlimentacao::ALIMENTANDO ||
           d.alimentacao.estado == EstadoAlimentacao::VERIFICANDO_PESO ||
           d.hidratacao.estado == EstadoHidratacao::ABASTECENDO ||
           d.hidratacao.estado == EstadoHidratacao::VERIFICANDO_NIVEL;
}

bool haFalhaCritica(const DadosSistema& d) {
    return d.estado == EstadoSistema::ERRO ||
           d.alimentacao.estado == EstadoAlimentacao::ERRO ||
           d.hidratacao.estado == EstadoHidratacao::ERRO;
}

bool haAviso(const DadosSistema& d) {
    return d.estado == EstadoSistema::ALERTA || d.manutencao.alertaAtivo ||
           d.reservatorioAlimento.dados.nivelBaixo || d.reservatorioAgua.dados.nivelBaixo ||
           !d.reservatorioAlimento.dados.leituraValida || !d.reservatorioAgua.dados.leituraValida;
}

void atualizarLeds(const DadosSistema& d) {
    unsigned long agora = millis();
    if (agora - ultimaAtualizacaoLed >= INTERVALO_LED_MS) {
        ultimaAtualizacaoLed = agora;
        ledPisca = !ledPisca;
    }
    digitalWrite(LED_SISTEMA, d.estado == EstadoSistema::OPERACIONAL ? HIGH : LOW);
    digitalWrite(LED_ACAO, haOperacaoAtiva(d) ? HIGH : LOW);
    digitalWrite(LED_AVISO, haAviso(d) ? (ledPisca ? HIGH : LOW) : LOW);
    digitalWrite(LED_CRITICO, haFalhaCritica(d) ? (ledPisca ? HIGH : LOW) : LOW);
}

void desenharHome(const DadosSistema& d) {
    DateTime agora;
    char l1[17], l2[17];
    if (obterDataHora(agora)) snprintf(l1, sizeof(l1), "%02u:%02u RTC OK", agora.hour(), agora.minute());
    else snprintf(l1, sizeof(l1), "--:-- RTC FALHA");
    const char* estado = d.estado == EstadoSistema::OPERACIONAL ? "Operacional" :
                         d.estado == EstadoSistema::ALERTA ? "Em Alerta" :
                         d.estado == EstadoSistema::ERRO ? "Erro Critico" : "Iniciando";
    snprintf(l2, sizeof(l2), "SB %-13.13s", estado);
    escreverLinha(0, l1); escreverLinha(1, l2);
}

void desenharReservatorios(const DadosSistema& d) {
    char l1[17], l2[17];
    if (d.reservatorioAlimento.dados.leituraValida)
        snprintf(l1, sizeof(l1), "Racao:%3.0f%%", d.reservatorioAlimento.dados.nivelAtualPercentual);
    else snprintf(l1, sizeof(l1), "Racao: --%%");
    if (d.reservatorioAgua.dados.leituraValida)
        snprintf(l2, sizeof(l2), "Agua :%3.0f%%", d.reservatorioAgua.dados.nivelAtualPercentual);
    else snprintf(l2, sizeof(l2), "Agua : --%%");
    escreverLinha(0, l1); escreverLinha(1, l2);
}

void desenharPotinhos(const DadosSistema& d) {
    char l1[17], l2[17];
    if (d.alimentacao.leituraValida) snprintf(l1, sizeof(l1), "Com:%5.1fg", d.alimentacao.pesoAtualGramas);
    else snprintf(l1, sizeof(l1), "Com: --.-g");
    const char* agua = d.hidratacao.estadoRecipiente == EstadoRecipienteAgua::OK ? "OK" :
                       d.hidratacao.estadoRecipiente == EstadoRecipienteAgua::BAIXO ? "BAIXA" : "?";
    snprintf(l2, sizeof(l2), "Agua:%-11s", agua);
    escreverLinha(0, l1); escreverLinha(1, l2);
}

void desenharProximaAcao(const DadosSistema& d) {
    DateTime agora;
    if (!obterDataHora(agora)) { escreverLinha(0, "Prox. Acao"); escreverLinha(1, "RTC indisponivel"); return; }
    uint16_t agoraMin = agora.hour() * 60 + agora.minute();
    int melhorDelta = 1441;
    int melhorMin = -1;
    const char* nome = "--";
    const AgendaDiaria* agendas[2] = {&d.agendaAlimentacao, &d.agendaHidratacao};
    const char* nomes[2] = {"Alim", "Hidr"};
    for (uint8_t a = 0; a < 2; ++a) {
        for (uint8_t i = 0; i < MAX_HORARIOS_DIARIOS; ++i) {
            const Horario& h = agendas[a]->horarios[i];
            if (!h.ativo) continue;
            int m = h.hora * 60 + h.minuto;
            int delta = m - agoraMin;
            if (delta < 0) delta += 1440;
            if (delta < melhorDelta) { melhorDelta = delta; melhorMin = m; nome = nomes[a]; }
        }
    }
    char l2[17];
    if (melhorMin < 0) snprintf(l2, sizeof(l2), "Sem agendamentos");
    else snprintf(l2, sizeof(l2), "%02d:%02d %-4s", melhorMin / 60, melhorMin % 60, nome);
    escreverLinha(0, "Prox. Acao"); escreverLinha(1, l2);
}

void desenharManutencao(const DadosSistema& d) {
    escreverLinha(0, "Limpeza");
    if (d.manutencao.limpezaPendente) { escreverLinha(1, "PENDENTE OK >"); return; }
    if (!d.manutencao.limpezaRegistrada) { escreverLinha(1, "PENDENTE OK >"); return; }
    DateTime agora;
    if (!obterDataHora(agora) || agora.unixtime() < d.manutencao.ultimaLimpezaEpoch) {
        escreverLinha(1, "Tempo indisponivel"); return;
    }
    uint32_t intervalo = d.configuracaoManutencao.intervaloLimpezaSegundos;
    uint32_t decorrido = agora.unixtime() - d.manutencao.ultimaLimpezaEpoch;
    uint32_t restante = decorrido >= intervalo ? 0 : intervalo - decorrido;
    uint32_t horas = restante / 3600UL;
    snprintf(mensagemLinha2, sizeof(mensagemLinha2), "%lu h restantes", (unsigned long)horas);
    escreverLinha(1, mensagemLinha2);
}

void desenharConectividade() {
    // O firmware atual ainda nao recebe estados Wi-Fi/MQTT do ESP8266.
    escreverLinha(0, "WiFi: --");
    escreverLinha(1, "MQTT: --");
}

void desenharMenu(DadosSistema& d) {
    char l1[17], l2[17];
    AgendaDiaria& agenda = agendaSelecionada(d);
    switch (modoMenu) {
        case ModoMenu::MODULO_AGENDA:
            escreverLinha(0, "Editar horarios");
            escreverLinha(1, moduloSelecionado == ModuloAgenda::ALIMENTACAO ? ">Alim  Hidr" : " Alim >Hidr");
            break;
        case ModoMenu::LISTA_HORARIOS: {
            snprintf(l1, sizeof(l1), "%s horarios", moduloSelecionado == ModuloAgenda::ALIMENTACAO ? "Alim." : "Hidr.");
            const Horario& selecionado = agenda.horarios[indiceHorario];
            if (selecionado.ativo) snprintf(l2, sizeof(l2), ">%u) %02u:%02u", indiceHorario + 1, selecionado.hora, selecionado.minuto);
            else snprintf(l2, sizeof(l2), ">%u) --:--", indiceHorario + 1);
            escreverLinha(0, l1); escreverLinha(1, l2);
            break;
        }
        case ModoMenu::EDITAR_HORARIO:
            snprintf(l1, sizeof(l1), "Horario %u", indiceHorario + 1);
            snprintf(l2, sizeof(l2), "%02u:%02u  < > OK", horaEditada, minutoEditado);
            escreverLinha(0, l1); escreverLinha(1, l2);
            break;
        case ModoMenu::CONFIRMAR_HORARIO:
            escreverLinha(0, "Salvar horario?");
            escreverLinha(1, opcaoConfirmacao == 0 ? ">Salvar Cancelar" : " Salvar >Cancelar");
            break;
        case ModoMenu::CONFIRMAR_LIMPEZA:
            escreverLinha(0, "Confirmar limpeza?");
            escreverLinha(1, opcaoConfirmacao == 0 ? ">Sim  Nao" : " Sim >Nao");
            break;
        case ModoMenu::MENSAGEM:
            escreverLinha(0, mensagemLinha1); escreverLinha(1, mensagemLinha2);
            break;
        default: break;
    }
}

void iniciarEdicao(DadosSistema& d) {
    AgendaDiaria& agenda = agendaSelecionada(d);
    Horario& h = agenda.horarios[indiceHorario];
    horaEditada = h.ativo ? h.hora : 8;
    minutoEditado = h.ativo ? h.minuto : 0;
    modoMenu = ModoMenu::EDITAR_HORARIO;
    telaSuja = true;
}

void confirmarEdicao(DadosSistema& d) {
    AgendaDiaria& agenda = agendaSelecionada(d);
    // definirHorario valida duplicidade e bloqueia horarios com pendencia.
    bool ok = definirHorario(agenda, indiceHorario, horaEditada, minutoEditado);
    if (ok) { bip(2400, 2); mostrarMensagem("Horario salvo", "Com sucesso"); }
    else { bip(500, 2); mostrarMensagem("Nao foi possivel", "Horario ocupado/dup."); }
}

void tratarBotoes(DadosSistema& d, bool ant, bool ok, bool prox) {
    if (modoMenu == ModoMenu::MENSAGEM) {
        if (millis() - inicioMensagem >= 2000UL) { modoMenu = ModoMenu::NENHUM; telaSuja = true; }
        return;
    }
    if (modoMenu == ModoMenu::NENHUM) {
        if (ant) { uint8_t t = static_cast<uint8_t>(telaAtual); telaAtual = static_cast<Tela>((t + 5) % 6); telaSuja = true; bip(2200, 1); }
        if (prox) { uint8_t t = static_cast<uint8_t>(telaAtual); telaAtual = static_cast<Tela>((t + 1) % 6); telaSuja = true; bip(2200, 1); }
        if (ok) {
            bip(2400, 1);
            if (telaAtual == Tela::PROXIMA_ACAO) modoMenu = ModoMenu::MODULO_AGENDA;
            else if (telaAtual == Tela::MANUTENCAO) { modoMenu = ModoMenu::CONFIRMAR_LIMPEZA; opcaoConfirmacao = 0; }
            telaSuja = true;
        }
        return;
    }
    switch (modoMenu) {
        case ModoMenu::MODULO_AGENDA:
            if (ant) { modoMenu = ModoMenu::NENHUM; telaSuja = true; }
            else if (prox) moduloSelecionado = moduloSelecionado == ModuloAgenda::ALIMENTACAO ? ModuloAgenda::HIDRATACAO : ModuloAgenda::ALIMENTACAO;
            if (ok) { indiceHorario = 0; modoMenu = ModoMenu::LISTA_HORARIOS; }
            break;
        case ModoMenu::LISTA_HORARIOS:
            if (ant) modoMenu = ModoMenu::MODULO_AGENDA;
            else if (prox) indiceHorario = (indiceHorario + 1) % 4;
            else if (ok) iniciarEdicao(d);
            break;
        case ModoMenu::EDITAR_HORARIO:
            if (ant) { if (minutoEditado < 30) { minutoEditado += 30; horaEditada = (horaEditada + 23) % 24; } else minutoEditado -= 30; }
            if (prox) { if (minutoEditado >= 30) { minutoEditado -= 30; horaEditada = (horaEditada + 1) % 24; } else minutoEditado += 30; }
            if (ok) { modoMenu = ModoMenu::CONFIRMAR_HORARIO; opcaoConfirmacao = 0; }
            break;
        case ModoMenu::CONFIRMAR_HORARIO:
            if (ant || prox) opcaoConfirmacao = !opcaoConfirmacao;
            if (ok) { if (opcaoConfirmacao == 0) confirmarEdicao(d); else { modoMenu = ModoMenu::LISTA_HORARIOS; telaSuja = true; } }
            break;
        case ModoMenu::CONFIRMAR_LIMPEZA:
            if (ant || prox) opcaoConfirmacao = !opcaoConfirmacao;
            if (ok) {
                if (opcaoConfirmacao == 0) {
                    if (confirmarLimpeza(d)) { bip(2400, 2); mostrarMensagem("Limpeza", "Registrada"); }
                    else { bip(500, 2); mostrarMensagem("Falha no RTC", "Limpeza nao salva"); }
                } else { modoMenu = ModoMenu::NENHUM; telaSuja = true; }
            }
            break;
        default: break;
    }
    telaSuja = true;
}

void desenharTela(DadosSistema& d) {
    if (modoMenu != ModoMenu::NENHUM) { desenharMenu(d); return; }
    switch (telaAtual) {
        case Tela::HOME: desenharHome(d); break;
        case Tela::RESERVATORIOS: desenharReservatorios(d); break;
        case Tela::POTINHOS: desenharPotinhos(d); break;
        case Tela::PROXIMA_ACAO: desenharProximaAcao(d); break;
        case Tela::MANUTENCAO: desenharManutencao(d); break;
        case Tela::CONECTIVIDADE: desenharConectividade(); break;
    }
}
}

void inicializarInterface(DadosSistema& dados) {
    (void)dados;
    pinMode(BOTAO_ANTERIOR, INPUT_PULLUP);
    pinMode(BOTAO_OK, INPUT_PULLUP);
    pinMode(BOTAO_PROXIMO, INPUT_PULLUP);
    pinMode(LED_SISTEMA, OUTPUT);
    pinMode(LED_ACAO, OUTPUT);
    pinMode(LED_AVISO, OUTPUT);
    pinMode(LED_CRITICO, OUTPUT);
    pinMode(BUZZER_INTERFACE, OUTPUT);
    digitalWrite(LED_SISTEMA, LOW); digitalWrite(LED_ACAO, LOW);
    digitalWrite(LED_AVISO, LOW); digitalWrite(LED_CRITICO, LOW);
    noTone(BUZZER_INTERFACE);
    Wire.begin();
    lcd.init();
    lcd.backlight();
    escreverLinha(0, "SmartBird v1.0");
    escreverLinha(1, "Iniciando...");
    bip(2200, 1);
    telaSuja = true;
}

void atualizarInterface(DadosSistema& dados) {
    bool ant = lerBotao(0);
    bool ok = lerBotao(1);
    bool prox = lerBotao(2);
    tratarBotoes(dados, ant, ok, prox);
    atualizarLeds(dados);
    atualizarBuzzer();
    unsigned long agora = millis();
    if (telaSuja || agora - ultimaAtualizacaoTela >= INTERVALO_TELA_MS) {
        ultimaAtualizacaoTela = agora;
        desenharTela(dados);
        telaSuja = false;
    }
}
