// =========================================================================
// PROJETO SMARTBIRD - FIRMWARE ESP32 (VERSÃO 2.2)
// Arquitetura: Ponte Transparente (Wi-Fi, MQTT EMQX, UART Bridge)
// =========================================================================

#include <WiFi.h>
#include <WiFiClientSecure.h> // Essencial para comunicação na porta 8883 (TLS/SSL)
#include <PubSubClient.h>     // Biblioteca para gerenciar a conexão MQTT

// =========================================================================
// 1. CONFIGURAÇÕES DE REDE (Wi-Fi)
// =========================================================================
// Recomenda-se utilizar as credenciais do seu roteador móvel (Hotspot 4G) 
// para evitar firewalls universitários no dia da banca.
const char* ssid = "Wokwi-GUEST"; 
const char* password = "";

// =========================================================================
// 2. CONFIGURAÇÕES DO BROKER MQTT (EMQX CLOUD)
// =========================================================================
const char* mqtt_server = "x1901c96.ala.eu-central-1.emqxsl.com"; // URL gerada no painel do EMQX
const int mqtt_port = 8883;                          // Porta para conexão segura
const char* mqtt_user = "SmartBird";               // Credencial de segurança 
const char* mqtt_pass = "trincaferro123";                 // Credencial de segurança

// Definição estrita dos canais (tópicos) de comunicação
const char* topico_comandos = "smartbird/gaiola01/comandos"; // Onde o site envia requisições
const char* topico_status   = "smartbird/gaiola01/status";   // Onde o ESP32 publica a telemetria do Mega

// Objetos para instanciar a comunicação segura e o cliente MQTT
WiFiClientSecure espClient;
PubSubClient client(espClient);

// =========================================================================
// 3. MAPEAMENTO DE PINOS UART (Comunicação Física com o Arduino Mega)
// =========================================================================
// Utiliza a porta 'Serial2' do ESP32 para garantir um canal exclusivo e limpo
// O pino TXD2 (17) envia dados para o RX1 (19) do Arduino Mega
// O pino RXD2 (16) recebe dados do TX1 (18) do Arduino Mega
#define RXD2 16
#define TXD2 17

// =========================================================================
// 4. FUNÇÃO: INICIALIZAR E CONECTAR AO WI-FI
// =========================================================================
void setup_wifi() {
  delay(10);
  Serial.println("\n[WIFI] Tentando conexão com: " + String(ssid));
  
  WiFi.begin(ssid, password);
  
  // Aguarda em loop até que a conexão seja estabelecida
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\n[WIFI] Conectado com sucesso!");
  Serial.print("[WIFI] IP Atribuído: ");
  Serial.println(WiFi.localIP());

  // Ignora a verificação rigorosa de certificados do servidor,
  // permitindo conexão rápida e segura com a nuvem na fase de protótipo.
  espClient.setInsecure(); 
}

// =========================================================================
// 5. FUNÇÃO: SITE ➔ ARDUINO MEGA (Receber do EMQX e repassar via UART)
// =========================================================================
// Engatilhada automaticamente pela biblioteca quando o site publica no tópico de comandos
void callback(char* topic, byte* payload, unsigned int length) {
  
  // Converte a matriz de bytes bruta em uma String legível
  String jsonRecebido = "";
  for (int i = 0; i < length; i++) {
    jsonRecebido += (char)payload[i];
  }

  // Depuração local para acompanhamento via cabo USB no computador
  Serial.println("\n[MQTT -> MEGA] JSON recebido da nuvem:");
  Serial.println(jsonRecebido);

  // PONTE TRANSPARENTE: O ESP32 envia a string JSON completa inalterada 
  // pela porta Serial2 diretamente para o Arduino Mega decodificar.
  Serial2.println(jsonRecebido);
}

// =========================================================================
// 6. FUNÇÃO: ARDUINO MEGA ➔ SITE (Receber via UART e repassar para o EMQX)
// =========================================================================
// Processamento não-bloqueante para evitar que o ESP32 se desconecte do EMQX
void lerDadosDoMega() {
  static String jsonAcumulado = ""; // Mantém o estado da mensagem entre os ciclos de loop

  // Lê todos os caracteres disponíveis na porta Serial2 velozmente
  while (Serial.available()) {
    char caractere = Serial.read();
    
    // O caractere '\n' indica que o Mega terminou de transmitir a mensagem JSON
    if (caractere == '\n') {
      jsonAcumulado.trim(); 
      
      if (jsonAcumulado.length() > 0) {
        Serial.println("\n[MEGA -> MQTT] JSON recebido do Arduino:");
        Serial.println(jsonAcumulado);

        // Publica o JSON intacto no tópico de telemetria/status para o site consumir
        client.publish(topico_status, jsonAcumulado.c_str());
      }
      
      // Limpa a variável estática para aguardar o próximo pacote
      jsonAcumulado = ""; 
    } 
    else {
      // Acumula o caractere na string enquanto a mensagem não chega ao fim
      jsonAcumulado += caractere;
    }
  }
}

// =========================================================================
// 7. FUNÇÃO: CONECTAR E GERENCIAR SESSÃO NO BROKER EMQX
// =========================================================================
void reconnect() {
  while (!client.connected()) {
    Serial.print("[MQTT] Solicitando conexão ao servidor EMQX... ");
    
    // Gera um identificador aleatório para evitar conflitos de sessão na nuvem
    String clientId = "SmartBird-Gateway-";
    clientId += String(random(0xffff), HEX);
    
    // Tenta autenticar usando as credenciais configuradas
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("Conexão bem-sucedida!");
      
      // Inscreve-se no tópico de comandos exigindo Qualidade de Serviço (QoS 1)
      // Garante que o Broker retenha comandos caso o 4G oscile momentaneamente.
      client.subscribe(topico_comandos, 1); 
      Serial.println("[MQTT] Monitorando tópico de comandos com QoS 1.");
    } else {
      Serial.print("Falha na autenticação. Código do erro: ");
      Serial.print(client.state());
      Serial.println(" -> Nova tentativa em 5 segundos.");
      delay(5000);
    }
  }
}

// =========================================================================
// 8. SETUP DO MICROCONTROLADOR
// =========================================================================
void setup() {
  // Serial0: reservada para enviar logs de depuração ao monitor do computador
  Serial.begin(115200); 
  
  // Serial2: reservada exclusivamente para o cabo cruzado com o Arduino Mega
  // Operando a 115200 baud, 8 bits de dados, sem paridade, 1 bit de parada.
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2); 
  
  setup_wifi();
  
  // Aumenta o tamanho do buffer de memória para 1024 bytes.
  // Previne desconexões instantâneas ao receber strings JSON longas do painel web.
  client.setBufferSize(1024); 
  
  // Aumenta a tolerância de falhas de latência de rede (KeepAlive) para 60 segundos.
  client.setKeepAlive(60); 

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback); 
}

// =========================================================================
// 9. LOOP PRINCIPAL DE EXECUÇÃO
// =========================================================================
void loop() {
  // Se o sinal de Wi-Fi ou a conexão MQTT cair, paralisa e reconecta
  if (!client.connected()) {
    reconnect();
  }
  
  // Mantém os batimentos cardíacos (PING) da biblioteca ativos para o servidor EMQX
  client.loop(); 
  
  // Monitora a porta UART em busca de respostas do Arduino Mega de forma não-bloqueante
  lerDadosDoMega(); 
}