#include "Protocolo.h"

#include <ArduinoJson.h>
#include <RTClib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#include "Comunicacao.h"
#include "Tempo.h"
#include "Alimentacao.h"
#include "Hidratacao.h"
#include "Manutencao.h"

namespace {

// =====================================================
// CONVERSÃO DE ESTADOS
// =====================================================

const char* estadoSistemaTexto(EstadoSistema e) {
    switch (e) {
        case EstadoSistema::INICIALIZANDO: return "initializing";
        case EstadoSistema::OPERACIONAL: return "operational";
        case EstadoSistema::ALERTA: return "alert";
        case EstadoSistema::ERRO: return "error";
        default: return "error";
    }
}

const char* estadoAlimentacaoTexto(EstadoAlimentacao e) {
    switch (e) {
        case EstadoAlimentacao::AGUARDANDO: return "idle";
        case EstadoAlimentacao::VERIFICANDO_PESO: return "checking";
        case EstadoAlimentacao::ALIMENTANDO: return "running";
        case EstadoAlimentacao::FINALIZADO: return "completed";
        case EstadoAlimentacao::ERRO: return "error";
        default: return "error";
    }
}

const char* estadoHidratacaoTexto(EstadoHidratacao e) {
    switch (e) {
        case EstadoHidratacao::AGUARDANDO: return "idle";
        case EstadoHidratacao::VERIFICANDO_NIVEL: return "checking";
        case EstadoHidratacao::ABASTECENDO: return "running";
        case EstadoHidratacao::FINALIZADO: return "completed";
        case EstadoHidratacao::ERRO: return "error";
        default: return "error";
    }
}

const char* estadoRecipienteAguaTexto(EstadoRecipienteAgua e) {
    switch (e) {
        case EstadoRecipienteAgua::BAIXO: return "low";
        case EstadoRecipienteAgua::OK: return "ok";
        default: return "unknown";
    }
}

// =====================================================
// ENVIO DE MENSAGENS
// =====================================================

bool enviarDocumento(JsonDocument& doc) {
    static char buffer[TAMANHO_BUFFER_MENSAGEM];

    size_t tamanho = measureJson(doc);

    if (tamanho == 0 || tamanho > MAX_BYTES_MENSAGEM) {
        return false;
    }

    size_t gravados = serializeJson(doc, buffer, sizeof(buffer));

    if (gravados == 0 || gravados > MAX_BYTES_MENSAGEM) {
        return false;
    }

    buffer[gravados] = '\0';

    return enviarMensagem(buffer);
}

void enviarErro(
    const char* id,
    const char* codigo,
    const char* mensagem
) {
    JsonDocument resposta;

    resposta["v"] = "1.0";
    resposta["type"] = "error";
    resposta["source"] = "arduino";

    if (id && *id) {
        resposta["id"] = id;
    }

    resposta["error"]["code"] = codigo;
    resposta["error"]["message"] = mensagem;

    enviarDocumento(resposta);
}

void enviarAck(const char* id, const char* status) {
    JsonDocument resposta;

    resposta["v"] = "1.0";
    resposta["type"] = "ack";
    resposta["id"] = id;
    resposta["source"] = "arduino";
    resposta["status"] = status;

    Serial.print("[ACK] Tentando enviar: ");
    Serial.println(status);

    bool enviado = enviarDocumento(resposta);

    if (enviado) {
      Serial.println("[ACK] Mensagem encaminhada para comunicacao.");
    } else {
      Serial.println("[ERRO] Falha ao montar ou enviar ACK!");
}
}

void enviarResponseDados(const char* id, JsonDocument& dados) {
    dados["v"] = "1.0";
    dados["type"] = "response";
    dados["id"] = id;
    dados["source"] = "arduino";
    dados["status"] = "success";

    enviarDocumento(dados);
}

// =====================================================
// RESERVATÓRIOS
// =====================================================

void adicionarReservatorio(JsonObject destino, const Reservatorio& reservatorio) {
    destino["level_percent"] = reservatorio.dados.nivelAtualPercentual;
    destino["distance_cm"] = reservatorio.dados.distanciaSensorCm;
    destino["valid"] = reservatorio.dados.leituraValida;
    destino["low"] = reservatorio.dados.nivelBaixo;
}

// =====================================================
// RECIPIENTES
// =====================================================

void adicionarRecipientes(JsonObject destino, const DadosSistema& dados) {

    // Recipiente de alimento
    JsonObject alimento = destino["food_bowl"].to<JsonObject>();

    alimento["weight_g"] = dados.alimentacao.pesoAtualGramas;
    alimento["valid"] = dados.alimentacao.leituraValida;

    // Recipiente de água
    JsonObject agua = destino["water_bowl"].to<JsonObject>();

    agua["level_state"] =
        estadoRecipienteAguaTexto(dados.hidratacao.estadoRecipiente);

    agua["low"] =
        dados.hidratacao.estadoRecipiente == EstadoRecipienteAgua::BAIXO;

    agua["valid"] =
        dados.hidratacao.estadoRecipiente != EstadoRecipienteAgua::INDEFINIDO;
}

// =====================================================
// AGENDAMENTOS
// =====================================================

void adicionarAgenda(JsonObject destino, const AgendaDiaria& agenda) {

    destino["enabled"] = agendaPossuiHorarios(agenda);

    JsonArray horarios = destino["times"].to<JsonArray>();

    for (uint8_t i = 0; i < MAX_HORARIOS_DIARIOS; i++) {

        if (!agenda.horarios[i].ativo) {
            continue;
        }

        char horario[6];

        snprintf(
            horario,
            sizeof(horario),
            "%02u:%02u",
            agenda.horarios[i].hora,
            agenda.horarios[i].minuto
        );

        horarios.add(horario);
    }
}

// =====================================================
// RESPOSTA: STATUS
// =====================================================

void responderStatus(const char* id, const DadosSistema& dados) {

    JsonDocument resposta;

    resposta["v"] = "1.0";
    resposta["type"] = "response";
    resposta["id"] = id;
    resposta["source"] = "arduino";
    resposta["status"] = "success";

    JsonObject data = resposta["data"].to<JsonObject>();

    data["system"] = estadoSistemaTexto(dados.estado);

    data["feeding"] =
        estadoAlimentacaoTexto(dados.alimentacao.estado);

    data["watering"] =
        estadoHidratacaoTexto(dados.hidratacao.estado);

    // Reservatórios de origem
    JsonObject reservatorios = data["reservoirs"].to<JsonObject>();

    adicionarReservatorio(
        reservatorios["food"].to<JsonObject>(),
        dados.reservatorioAlimento
    );

    adicionarReservatorio(
        reservatorios["water"].to<JsonObject>(),
        dados.reservatorioAgua
    );

    // Recipientes acessíveis à ave
    adicionarRecipientes(
        data["containers"].to<JsonObject>(),
        dados
    );

    data["maintenance"] =
        dados.manutencao.alertaAtivo ? "alert" : "normal";

    DateTime agora;

    data["rtc"] =
        obterDataHora(agora) ? "available" : "failure";

    enviarDocumento(resposta);
}

// =====================================================
// RESPOSTA: TELEMETRIA
// =====================================================

void responderTelemetria(const char* id, const DadosSistema& dados) {

    JsonDocument resposta;

    resposta["v"] = "1.0";
    resposta["type"] = "response";
    resposta["id"] = id;
    resposta["source"] = "arduino";
    resposta["status"] = "success";

    JsonObject data = resposta["data"].to<JsonObject>();

    // Reservatórios de origem
    JsonObject reservatorios = data["reservoirs"].to<JsonObject>();

    adicionarReservatorio(
        reservatorios["food"].to<JsonObject>(),
        dados.reservatorioAlimento
    );

    adicionarReservatorio(
        reservatorios["water"].to<JsonObject>(),
        dados.reservatorioAgua
    );

    // Recipientes
    adicionarRecipientes(
        data["containers"].to<JsonObject>(),
        dados
    );

    // Alimentação
    JsonObject alimentacao = data["feeding"].to<JsonObject>();

    alimentacao["state"] =
        estadoAlimentacaoTexto(dados.alimentacao.estado);

    alimentacao["bowl_weight_g"] =
        dados.alimentacao.pesoAtualGramas;

    alimentacao["weight_valid"] =
        dados.alimentacao.leituraValida;

    alimentacao["gate_open"] =
        dados.alimentacao.comportaAberta;

    // Hidratação
    JsonObject hidratacao = data["watering"].to<JsonObject>();

    hidratacao["state"] =
        estadoHidratacaoTexto(dados.hidratacao.estado);

    hidratacao["pump_on"] =
        dados.hidratacao.bombaLigada;

    hidratacao["bowl_level"] =
        estadoRecipienteAguaTexto(dados.hidratacao.estadoRecipiente);

    // Manutenção
    JsonObject manutencao = data["maintenance"].to<JsonObject>();

    manutencao["cleaning_due"] =
        dados.manutencao.limpezaPendente;

    manutencao["alert_active"] =
        dados.manutencao.alertaAtivo;

    enviarDocumento(resposta);
}

// =====================================================
// RESPOSTA: AGENDAMENTOS
// =====================================================

void responderAgenda(const char* id, const DadosSistema& dados) {

    JsonDocument resposta;

    resposta["v"] = "1.0";
    resposta["type"] = "response";
    resposta["id"] = id;
    resposta["source"] = "arduino";
    resposta["status"] = "success";

    JsonObject data = resposta["data"].to<JsonObject>();

    adicionarAgenda(
        data["feeding"].to<JsonObject>(),
        dados.agendaAlimentacao
    );

    adicionarAgenda(
        data["watering"].to<JsonObject>(),
        dados.agendaHidratacao
    );

    enviarDocumento(resposta);
}

// =====================================================
// RESPOSTA: INFORMAÇÕES DO SISTEMA
// =====================================================

void responderInfo(const char* id) {

    JsonDocument resposta;

    resposta["v"] = "1.0";
    resposta["type"] = "response";
    resposta["id"] = id;
    resposta["source"] = "arduino";
    resposta["status"] = "success";

    JsonObject data = resposta["data"].to<JsonObject>();

    data["device"] = "smartbird";
    data["controller"] = "arduino_mega_2560";
    data["firmware_version"] = "0.1.0";
    data["protocol_version"] = "1.0";

    enviarDocumento(resposta);
}

// =====================================================
// VALIDAÇÃO DE HORÁRIOS
// =====================================================

bool parseHorario(const char* texto, uint8_t& hora, uint8_t& minuto) {

    if (!texto || strlen(texto) != 5 || texto[2] != ':') {
        return false;
    }

    for (uint8_t i = 0; i < 5; i++) {

        if (i == 2) {
            continue;
        }

        if (texto[i] < '0' || texto[i] > '9') {
            return false;
        }
    }

    hora = (texto[0] - '0') * 10 + (texto[1] - '0');

    minuto = (texto[3] - '0') * 10 + (texto[4] - '0');

    return hora <= 23 && minuto <= 59;
}

// =====================================================
// PREENCHIMENTO DE AGENDAS
// =====================================================

bool preencherAgenda(
    JsonVariantConst objeto,
    AgendaDiaria& agenda
) {

    if (!objeto.is<JsonObjectConst>()) {
        return false;
    }

    bool habilitada = objeto["enabled"] | false;

    JsonArrayConst horarios =
        objeto["times"].as<JsonArrayConst>();

    if (!habilitada) {

        if (!horarios.isNull() && horarios.size() > 0) {
            return false;
        }

        return sincronizarAgenda(agenda, nullptr, nullptr, 0);
    }

    if (horarios.isNull() || horarios.size() > MAX_HORARIOS_DIARIOS) {
        return false;
    }

    uint8_t horas[MAX_HORARIOS_DIARIOS];
    uint8_t minutos[MAX_HORARIOS_DIARIOS];

    uint8_t quantidade = 0;

    for (JsonVariantConst valor : horarios) {

        if (!valor.is<const char*>()) {
            return false;
        }

        if (!parseHorario(
            valor.as<const char*>(),
            horas[quantidade],
            minutos[quantidade]
        )) {
            return false;
        }

        quantidade++;
    }

    return sincronizarAgenda(
        agenda,
        horas,
        minutos,
        quantidade
    );
}

// =====================================================
// VALIDAÇÃO DE DATA
// =====================================================

bool anoBissexto(int ano) {
    return (ano % 4 == 0 && ano % 100 != 0) || (ano % 400 == 0);
}

uint8_t diasNoMes(int ano, uint8_t mes) {

    const uint8_t dias[] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };

    if (mes < 1 || mes > 12) {
        return 0;
    }

    if (mes == 2 && anoBissexto(ano)) {
        return 29;
    }

    return dias[mes - 1];
}

// =====================================================
// CONVERSÃO DE DATA/HORA
// =====================================================

// Recebe YYYY-MM-DDTHH:MM:SS±HH:MM. Os componentes de data/hora
// representam o horário de parede local enviado pela Web; o offset é
// validado como metadado e não é convertido para UTC no RTC local.

bool parseDateTimeLocal(const char* texto, DateTime& dataHora) {

    if (!texto || strlen(texto) != 25) {
        return false;
    }

    if (
        texto[4] != '-' ||
        texto[7] != '-' ||
        texto[10] != 'T' ||
        texto[13] != ':' ||
        texto[16] != ':' ||
        (texto[19] != '+' && texto[19] != '-') ||
        texto[22] != ':'
    ) {
        return false;
    }

    const uint8_t indices[] = {
        0, 1, 2, 3,
        5, 6,
        8, 9,
        11, 12,
        14, 15,
        17, 18,
        20, 21,
        23, 24
    };

    for (uint8_t i = 0; i < sizeof(indices); i++) {

        char c = texto[indices[i]];

        if (c < '0' || c > '9') {
            return false;
        }
    }

    int ano =
        (texto[0] - '0') * 1000 +
        (texto[1] - '0') * 100 +
        (texto[2] - '0') * 10 +
        (texto[3] - '0');

    uint8_t mes =
        (texto[5] - '0') * 10 +
        (texto[6] - '0');

    uint8_t dia =
        (texto[8] - '0') * 10 +
        (texto[9] - '0');

    uint8_t hora =
        (texto[11] - '0') * 10 +
        (texto[12] - '0');

    uint8_t minuto =
        (texto[14] - '0') * 10 +
        (texto[15] - '0');

    uint8_t segundo =
        (texto[17] - '0') * 10 +
        (texto[18] - '0');

    if (ano < 2020 || ano > 2099) {
        return false;
    }

    if (mes < 1 || mes > 12) {
        return false;
    }

    if (dia < 1 || dia > diasNoMes(ano, mes)) {
        return false;
    }

    if (hora > 23 || minuto > 59 || segundo > 59) {
        return false;
    }

    const uint8_t offsetHora =
        (texto[20] - '0') * 10 + (texto[21] - '0');
    const uint8_t offsetMinuto =
        (texto[23] - '0') * 10 + (texto[24] - '0');
    if (offsetHora > 14 || offsetMinuto > 59 ||
        (offsetHora == 14 && offsetMinuto != 0)) {
        return false;
    }

    dataHora = DateTime(
        ano,
        mes,
        dia,
        hora,
        minuto,
        segundo
    );

    return true;
}

} // namespace

// =====================================================
// PROCESSAMENTO PRINCIPAL DO PROTOCOLO
// =====================================================

void processarMensagem(
    const char* mensagem,
    DadosSistema& dados
) {

    Serial.println("[PROTOCOLO] Iniciando analise da mensagem...");

    if (!mensagem || !*mensagem) {
        enviarErro(nullptr, "INVALID_MESSAGE", "Empty message");
        return;
    }

    JsonDocument requisicao;

    DeserializationError erroJson =
        deserializeJson(requisicao, mensagem);

    if (erroJson) {
      Serial.println("[ERRO] JSON invalido!");
      enviarErro(nullptr, "INVALID_JSON", "JSON could not be parsed");
      return;
    }

    Serial.println("[PROTOCOLO] JSON interpretado com sucesso.");

    const char* versao = requisicao["v"] | "";
    const char* tipo = requisicao["type"] | "";
    const char* id = requisicao["id"] | "";
    const char* origem = requisicao["source"] | "";
    const char* acao = requisicao["action"] | "";

    Serial.print("[PROTOCOLO] ID: ");
    Serial.println(id);

    Serial.print("[PROTOCOLO] Acao: ");
    Serial.println(acao);

    if (strcmp(versao, "1.0") != 0) {
        enviarErro(
            id,
            "UNSUPPORTED_VERSION",
            "Protocol version is not supported"
        );
        return;
    }

    if (
        strcmp(tipo, "command") != 0 ||
        !*id ||
        strcmp(origem, "web") != 0 ||
        !*acao ||
        !requisicao["data"].is<JsonObjectConst>()
    ) {
        enviarErro(
            id,
            "INVALID_MESSAGE",
            "Required command fields are missing or invalid"
        );
        return;
    }

    JsonObjectConst data =
        requisicao["data"].as<JsonObjectConst>();

    // =================================================
    // CONSULTAS
    // =================================================

    if (strcmp(acao, "get_status") == 0) {

        enviarAck(id, "accepted");
        responderStatus(id, dados);

        return;
    }

    if (strcmp(acao, "get_telemetry") == 0) {

        enviarAck(id, "accepted");
        responderTelemetria(id, dados);

        return;
    }

    if (strcmp(acao, "get_schedule") == 0) {

        enviarAck(id, "accepted");
        responderAgenda(id, dados);

        return;
    }

    if (strcmp(acao, "get_system_info") == 0) {

        enviarAck(id, "accepted");
        responderInfo(id);

        return;
    }

    // =================================================
    // ALIMENTAÇÃO MANUAL
    // =================================================

    if (strcmp(acao, "feed_manual") == 0) {

        if (!iniciarAlimentacao(dados)) {

            enviarAck(id, "rejected");
            return;
        }

        enviarAck(id, "accepted");

        JsonDocument resposta;

        resposta["data"]["module"] = "feeding";
        resposta["data"]["result"] = "started";

        enviarResponseDados(id, resposta);

        return;
    }

    // =================================================
    // HIDRATAÇÃO MANUAL
    // =================================================

    if (strcmp(acao, "water_manual") == 0) {

        if (!iniciarHidratacao(dados)) {

            enviarAck(id, "rejected");
            return;
        }

        enviarAck(id, "accepted");

        JsonDocument resposta;

        resposta["data"]["module"] = "watering";
        resposta["data"]["result"] = "started";

        enviarResponseDados(id, resposta);

        return;
    }

    // =================================================
    // CONFIRMAÇÃO DE LIMPEZA
    // =================================================

    if (strcmp(acao, "ack_cleaning") == 0) {

        if (!confirmarLimpeza(dados)) {

            enviarAck(id, "rejected");
            return;
        }

        enviarAck(id, "accepted");

        JsonDocument resposta;

        resposta["data"]["cleaning_due"] = false;
        resposta["data"]["maintenance_timer_reset"] = true;

        enviarResponseDados(id, resposta);

        return;
    }

    // =================================================
    // ATUALIZAÇÃO DE AGENDAMENTOS
    // =================================================

    if (strcmp(acao, "set_schedule") == 0) {

        AgendaDiaria novaAgendaAlimentacao =
            dados.agendaAlimentacao;

        AgendaDiaria novaAgendaHidratacao =
            dados.agendaHidratacao;

        bool alimentacaoValida = preencherAgenda(
            data["feeding"],
            novaAgendaAlimentacao
        );

        bool hidratacaoValida = preencherAgenda(
            data["watering"],
            novaAgendaHidratacao
        );

        if (!alimentacaoValida || !hidratacaoValida) {

            enviarAck(id, "rejected");
            return;
        }

        dados.agendaAlimentacao = novaAgendaAlimentacao;
        dados.agendaHidratacao = novaAgendaHidratacao;

        enviarAck(id, "accepted");

        JsonDocument resposta;

        resposta["data"]["schedule_updated"] = true;

        enviarResponseDados(id, resposta);

        return;
    }

    // =================================================
    // REMOÇÃO DE AGENDAMENTO
    // =================================================

    if (strcmp(acao, "remove_schedule") == 0) {

        const char* modulo = data["module"] | "";
        const char* horario = data["time"] | "";

        uint8_t hora;
        uint8_t minuto;

        if (!parseHorario(horario, hora, minuto)) {

            enviarAck(id, "rejected");
            return;
        }

        AgendaDiaria* agenda = nullptr;

        if (strcmp(modulo, "feeding") == 0) {
            agenda = &dados.agendaAlimentacao;
        }
        else if (strcmp(modulo, "watering") == 0) {
            agenda = &dados.agendaHidratacao;
        }
        else {

            enviarAck(id, "rejected");
            return;
        }

        int indice = -1;

        for (uint8_t i = 0; i < MAX_HORARIOS_DIARIOS; i++) {

            if (
                agenda->horarios[i].ativo &&
                agenda->horarios[i].hora == hora &&
                agenda->horarios[i].minuto == minuto
            ) {
                indice = i;
                break;
            }
        }

        if (
            indice < 0 ||
            !removerHorario(*agenda, (uint8_t)indice)
        ) {

            enviarAck(id, "rejected");
            return;
        }

        enviarAck(id, "accepted");

        JsonDocument resposta;

        resposta["data"]["module"] = modulo;
        resposta["data"]["time"] = horario;
        resposta["data"]["removed"] = true;

        enviarResponseDados(id, resposta);

        return;
    }

    // =================================================
    // CONFIGURAÇÃO DE ALIMENTAÇÃO
    // =================================================

    if (strcmp(acao, "set_feeding_config") == 0) {
        if ((!data["target_weight_g"].is<float>() &&
             !data["target_weight_g"].is<int>()) ||
            (!data["max_duration_ms"].is<uint32_t>() &&
             !data["max_duration_ms"].is<unsigned long>() &&
             !data["max_duration_ms"].is<int>())) {
            enviarAck(id, "rejected");
            return;
        }

        const float pesoAlvo = data["target_weight_g"].as<float>();
        const uint32_t tempoMaximo = data["max_duration_ms"].as<uint32_t>();

        if (!isfinite(pesoAlvo) || pesoAlvo < 1.0f || pesoAlvo > 500.0f ||
            tempoMaximo < 1000UL || tempoMaximo > 120000UL ||
            (dados.alimentacao.estado == EstadoAlimentacao::ALIMENTANDO ||
             dados.alimentacao.estado == EstadoAlimentacao::VERIFICANDO_PESO)) {
            enviarAck(id, "rejected");
            return;
        }

        dados.configuracaoAlimentacao.pesoAlvoGramas = pesoAlvo;
        dados.configuracaoAlimentacao.tempoMaximoMs = tempoMaximo;

        enviarAck(id, "accepted");
        JsonDocument resposta;
        resposta["data"]["configuration_updated"] = true;
        resposta["data"]["target_weight_g"] = pesoAlvo;
        resposta["data"]["max_duration_ms"] = tempoMaximo;
        enviarResponseDados(id, resposta);
        return;
    }

    // =================================================
    // CONFIGURAÇÃO DE HIDRATAÇÃO
    // =================================================

    if (strcmp(acao, "set_watering_config") == 0) {

        enviarErro(
            id,
            "CONFIG_NOT_SUPPORTED",
            "Watering configuration is not supported by the current hardware configuration"
        );

        return;
    }

    // =================================================
    // SINCRONIZAÇÃO DO RTC
    // =================================================

    if (strcmp(acao, "sync_time") == 0) {

        const char* valorDataHora =
            data["datetime"] | "";

        DateTime dataHora(2000, 1, 1, 0, 0, 0);

        if (
            !parseDateTimeLocal(valorDataHora, dataHora) ||
            !ajustarDataHora(dataHora)
        ) {

            enviarAck(id, "rejected");
            return;
        }

        enviarAck(id, "accepted");

        JsonDocument resposta;

        resposta["data"]["rtc_updated"] = true;
        resposta["data"]["datetime"] = valorDataHora;

        enviarResponseDados(id, resposta);

        return;
    }

    // =================================================
    // COMANDO DESCONHECIDO
    // =================================================

    enviarErro(
        id,
        "UNKNOWN_COMMAND",
        "Command is not recognized by this firmware"
    );
}