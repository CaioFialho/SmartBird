
// =============================================================
// SMARTBIRD - FIRMWARE ESP8266 V2.4
// Ponte transparente Wi-Fi / MQTT / UART
// Versao simplificada com fila de JSONs
// =============================================================

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>
#include <string.h>

// =============================================================
// 1. CONFIGURACOES WI-FI
// =============================================================

const char* ssid = "Caio_2.4G";
const char* password = "984321505";

// =============================================================
// 2. CONFIGURACOES MQTT - EMQX
// =============================================================

const char* mqtt_server =
    "x1901c96.ala.eu-central-1.emqxsl.com";

const int mqtt_port = 8883;

const char* mqtt_user = "SmartBird";
const char* mqtt_pass = "trincaferro123";

const char* topico_comandos =
    "smartbird/gaiola01/comandos";

const char* topico_status =
    "smartbird/gaiola01/status";

// =============================================================
// 3. OBJETOS DE COMUNICACAO
// =============================================================

WiFiClientSecure espClient;
PubSubClient client(espClient);

// RX = D5 (GPIO14)
// TX = D6 (GPIO12)
SoftwareSerial portaMega(14, 12);

// Mesma velocidade configurada no Arduino Mega
const uint32_t UART_BAUD = 4800;

// =============================================================
// 4. BUFFER E FILA DE JSONS
// =============================================================

// Tamanho maximo de um JSON
const size_t MAX_JSON = 1024;

// Quantidade maxima de JSONs aguardando envio
const uint8_t TAMANHO_FILA = 3;

// Buffer que monta a mensagem ate encontrar '\n'
char bufferUART[MAX_JSON + 1];
size_t posicaoUART = 0;

bool descartando = false;
unsigned long ultimoByteUART = 0;

// Cada posicao guarda um JSON completo
struct Mensagem {
  char json[MAX_JSON + 1];
  uint16_t tamanho;
  bool exibida;
};

Mensagem fila[TAMANHO_FILA];

uint8_t inicioFila = 0;
uint8_t fimFila = 0;
uint8_t quantidadeFila = 0;

// Mensagem de erro pendente para o Monitor Serial
const char* avisoUART = nullptr;

// =============================================================
// 5. TEMPORIZACOES
// =============================================================

const unsigned long INTERVALO_MQTT = 5000;
const unsigned long INTERVALO_WIFI = 10000;
const unsigned long SILENCIO_UART = 150;
const unsigned long TIMEOUT_UART = 3000;

unsigned long ultimaTentativaMQTT = 0;
unsigned long ultimaTentativaWiFi = 0;
unsigned long ultimaFalhaPublicacao = 0;

bool primeiraTentativaMQTT = true;

// =============================================================
// 6. VERIFICACAO BASICA DO JSON
// =============================================================

// Verifica delimitadores e aspas.
// Nao substitui um parser JSON completo.

bool jsonIntegro(const char* json, size_t tamanho) {

  if (tamanho < 2 ||
      json[0] != '{' ||
      json[tamanho - 1] != '}') {
    return false;
  }

  char pilha[32];
  uint8_t profundidade = 0;

  bool dentroString = false;
  bool escape = false;

  for (size_t i = 0; i < tamanho; i++) {

    unsigned char c = (unsigned char)json[i];

    if (c < 32) return false;

    if (dentroString) {

      if (escape) {
        escape = false;
      } else if (c == '\\') {
        escape = true;
      } else if (c == '"') {
        dentroString = false;
      }

      continue;
    }

    if (c == '"') {
      dentroString = true;

    } else if (c == '{' || c == '[') {

      if (profundidade >= sizeof(pilha)) {
        return false;
      }

      pilha[profundidade++] =
          (c == '{') ? '}' : ']';

    } else if (c == '}' || c == ']') {

      if (profundidade == 0 ||
          pilha[profundidade - 1] != c) {
        return false;
      }

      profundidade--;
    }
  }

  return !dentroString &&
         !escape &&
         profundidade == 0;
}

// =============================================================
// 7. ADICIONAR JSON COMPLETO NA FILA
// =============================================================

void adicionarNaFila(const char* json, size_t tamanho) {

  if (quantidadeFila >= TAMANHO_FILA) {
    avisoUART = "[UART] Fila cheia! JSON descartado.";
    return;
  }

  Mensagem &msg = fila[fimFila];

  memcpy(msg.json, json, tamanho);
  msg.json[tamanho] = '\0';

  msg.tamanho = tamanho;
  msg.exibida = false;

  fimFila = (fimFila + 1) % TAMANHO_FILA;
  quantidadeFila++;
}

// =============================================================
// 8. RECEBER DADOS DO ARDUINO MEGA
// =============================================================

// Recebe continuamente.
// Um JSON so e colocado na fila depois de encontrar '\n'.
// Nenhuma publicacao MQTT acontece dentro desta funcao.

void lerDadosDoMega() {

  if (portaMega.overflow()) {
    posicaoUART = 0;
    descartando = true;
    avisoUART = "[UART] Overflow de recepcao!";
  }

  while (portaMega.available() > 0) {

    int recebido = portaMega.read();

    if (recebido < 0) break;

    char caractere = (char)recebido;

    ultimoByteUART = millis();

    // FINAL DA MENSAGEM
    if (caractere == '\n') {

      if (!descartando && posicaoUART > 0) {

        bufferUART[posicaoUART] = '\0';

        if (jsonIntegro(bufferUART, posicaoUART)) {

          adicionarNaFila(
            bufferUART,
            posicaoUART
          );

        } else {

          avisoUART =
              "[UART] JSON incompleto ou invalido descartado.";
        }
      }

      // Preparar para o proximo JSON
      posicaoUART = 0;
      descartando = false;

      continue;
    }

    // Descartar mensagem danificada
    if (descartando) continue;

    // Ignorar CR
    if (caractere == '\r') continue;

    // Montagem do JSON
    if (posicaoUART < MAX_JSON) {

      bufferUART[posicaoUART++] = caractere;

    } else {

      posicaoUART = 0;
      descartando = true;

      avisoUART =
          "[UART] JSON maior que o buffer!";
    }
  }

  // Verificar perda de bytes durante a leitura
  if (portaMega.overflow()) {
    posicaoUART = 0;
    descartando = true;
    avisoUART = "[UART] Overflow de recepcao!";
  }

  // Recuperar caso uma linha fique incompleta
  if ((posicaoUART > 0 || descartando) &&
      millis() - ultimoByteUART >= TIMEOUT_UART) {

    posicaoUART = 0;
    descartando = false;

    avisoUART = "[UART] Timeout de mensagem!";
  }
}

// =============================================================
// 9. VERIFICAR SE A UART TERMINOU A TRANSMISSAO
// =============================================================

bool uartLivre() {

  if (portaMega.available() > 0) return false;

  if (posicaoUART > 0 || descartando) return false;

  return millis() - ultimoByteUART >= SILENCIO_UART;
}

// =============================================================
// 10. MOSTRAR JSON RECEBIDO NO MONITOR SERIAL
// =============================================================

// O JSON so e impresso depois que a UART estiver
// em repouso, reduzindo o risco de perda de bytes.

void mostrarJSONRecebido() {

  if (!uartLivre()) return;
  if (quantidadeFila == 0) return;

  Mensagem &msg = fila[inicioFila];

  if (msg.exibida) return;

  Serial.println();
  Serial.println("[UART MEGA -> ESP8266]");
  Serial.println(msg.json);

  msg.exibida = true;
}

// =============================================================
// 11. PUBLICAR JSON DA FILA NO MQTT
// =============================================================

// Publica uma mensagem por chamada.
// O JSON so e removido depois de publish() retornar true.

void publicarFila() {

  if (quantidadeFila == 0) return;
  if (!client.connected()) return;
  if (!uartLivre()) return;

  if (ultimaFalhaPublicacao != 0 &&
      millis() - ultimaFalhaPublicacao < 1000) {
    return;
  }

  Mensagem &msg = fila[inicioFila];

  bool enviado = client.publish(
    topico_status,
    (const uint8_t*)msg.json,
    msg.tamanho,
    false
  );

  if (enviado) {

    Serial.println("[MQTT] JSON enviado ao EMQX!");

    inicioFila = (inicioFila + 1) % TAMANHO_FILA;
    quantidadeFila--;

    ultimaFalhaPublicacao = 0;

  } else {

    avisoUART =
        "[MQTT] Falha na publicacao. JSON mantido na fila.";

    ultimaFalhaPublicacao = millis();
  }
}

// =============================================================
// 12. SITE -> ARDUINO MEGA
// =============================================================

void callback(char* topic, byte* payload,
              unsigned int length) {

  if (strcmp(topic, topico_comandos) != 0) {
    return;
  }

  if (length == 0 || length > MAX_JSON) {
    Serial.println("[MQTT] Comando fora do limite!");
    return;
  }

  // Protocolo: um JSON por linha
  for (unsigned int i = 0; i < length; i++) {

    if (payload[i] == '\n' || payload[i] == '\r') {

      Serial.println("[MQTT] Comando com quebra de linha!");
      return;
    }
  }

  Serial.println();
  Serial.println("[MQTT -> MEGA] JSON recebido:");

  // Mostra o payload sem criar Strings adicionais
  Serial.write(payload, length);
  Serial.println();

  Serial.println("[UART] Enviando para o Mega...");

  size_t enviados = portaMega.write(payload, length);

  if (enviados == length &&
      portaMega.write((uint8_t)'\n') == 1) {

    Serial.println("[UART] Envio concluido!");

  } else {

    Serial.println("[UART] Falha ao enviar comando!");
  }
}

// =============================================================
// 13. INICIALIZAR WI-FI
// =============================================================

void setup_wifi() {

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  Serial.println("[WIFI] Conectando...");
  WiFi.begin(ssid, password);
}

// =============================================================
// 14. GERENCIAR WI-FI
// =============================================================

void gerenciarWiFi() {

  if (WiFi.status() == WL_CONNECTED) return;

  if (millis() - ultimaTentativaWiFi <
      INTERVALO_WIFI) {
    return;
  }

  if (!uartLivre()) return;

  ultimaTentativaWiFi = millis();

  Serial.println("[WIFI] Tentando reconectar...");
  WiFi.reconnect();
}

// =============================================================
// 15. RECONEXAO MQTT
// =============================================================

void reconnect() {

  if (WiFi.status() != WL_CONNECTED) return;
  if (client.connected()) return;

  if (!primeiraTentativaMQTT &&
      millis() - ultimaTentativaMQTT <
      INTERVALO_MQTT) {
    return;
  }

  if (!uartLivre()) return;

  primeiraTentativaMQTT = false;
  ultimaTentativaMQTT = millis();

  String clientId = "SmartBird-Gateway-";
  clientId += String(ESP.getChipId(), HEX);

  Serial.print("[MQTT] Conectando ao EMQX... ");

  if (client.connect(
        clientId.c_str(),
        mqtt_user,
        mqtt_pass)) {

    Serial.println("Conectado!");

    if (client.subscribe(topico_comandos, 1)) {
      Serial.println("[MQTT] Monitorando comandos");
    } else {
      Serial.println("[MQTT] Falha na assinatura!");
    }

  } else {

    Serial.print("[MQTT] Falha. Codigo: ");
    Serial.println(client.state());
  }
}

// =============================================================
// 16. MOSTRAR AVISOS EVENTUAIS
// =============================================================

void mostrarAvisos() {

  if (!uartLivre()) return;
  if (avisoUART == nullptr) return;

  Serial.println(avisoUART);
  avisoUART = nullptr;
}

// =============================================================
// 17. SETUP
// =============================================================

void setup() {

  Serial.begin(115200);
  Serial.println();

  // UART: mesmos buffers da versao funcional
  portaMega.begin(
    UART_BAUD,
    SWSERIAL_8N1,
    14,
    12,
    false,
    1024, // Buffer RX
    1024  // Buffer de interrupcoes
  );

  if (!portaMega) {
    Serial.println("[ERRO] Falha na SoftwareSerial!");
  }

  // TLS usado no prototipo
  espClient.setInsecure();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  client.setKeepAlive(60);
  client.setSocketTimeout(5);

  if (!client.setBufferSize(1536)) {
    Serial.println("[ERRO] Buffer MQTT indisponivel!");
  }

  setup_wifi();

  Serial.println();
  Serial.println("================================");
  Serial.println(" SMARTBIRD ESP8266 - V2.4");
  Serial.println(" Ponte MQTT/UART com fila JSON");
  Serial.println("================================");
}

// =============================================================
// 18. LOOP PRINCIPAL
// =============================================================

void loop() {

  // 1. Receber e separar mensagens do Mega
  lerDadosDoMega();

  // 2. Gerenciar Wi-Fi
  gerenciarWiFi();

  // 3. Processar mensagens MQTT
  if (client.connected()) {
    client.loop();
  }

  // 4. Ler novamente apos MQTT
  lerDadosDoMega();

  // 5. Mostrar JSON recebido, depois de concluir a UART
  mostrarJSONRecebido();

  // 6. Publicar mensagem completa no EMQX
  publicarFila();

  // 7. Verificar novos bytes
  lerDadosDoMega();

  // 8. Reconectar MQTT quando necessario
  reconnect();

  // 9. Mostrar apenas avisos eventuais
  mostrarAvisos();

  yield();
}

// =============================================================
// FIM - SMARTBIRD ESP8266 V2.4
// =============================================================

