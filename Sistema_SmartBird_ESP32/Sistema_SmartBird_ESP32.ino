// =========================================================================
// PROJETO SMARTBIRD - FIRMWARE ESP8266 (VERSÃO 2.2)
// Arquitetura: Ponte Transparente (Wi-Fi, MQTT EMQX, UART Bridge)
// =========================================================================

#include <ESP8266WiFi.h>      // Biblioteca Wi-Fi específica para o ESP8266
#include <WiFiClientSecure.h> // Essencial para comunicação na porta 8883 (TLS/SSL)
#include <PubSubClient.h>     // Biblioteca para gerenciar a conexão MQTT
#include <SoftwareSerial.h>   // Biblioteca para emular a porta UART no ESP8266

// =========================================================================
// 1. CONFIGURAÇÕES DE REDE (Wi-Fi)
// =========================================================================
const char* ssid = "NOME-DA-REDE-WIFI"; 
const char* password = "SENHA-DA-REDE-WIFI";

// =========================================================================
// 2. CONFIGURAÇÕES DO BROKER MQTT (EMQX CLOUD)
// =========================================================================
const char* mqtt_server = "URL-DO-SERVIDOR"; 
const int mqtt_port = 8883;                                         
const char* mqtt_user = "USUARIO-MQTT";               
const char* mqtt_pass = "SENHA-MQTT";                 

// Definição estrita dos canais (tópicos) de comunicação
const char* topico_comandos = "smartbird/gaiola01/comandos"; 
const char* topico_status   = "smartbird/gaiola01/status";   

// Objetos para instanciar a comunicação segura e o cliente MQTT
WiFiClientSecure espClient;
PubSubClient client(espClient);

// =========================================================================
// 3. MAPEAMENTO DE PINOS UART VIRTUAL (Comunicação Física com o Arduino Mega)
// =========================================================================
// O ESP8266 não possui 'Serial2' livre. Usamos a SoftwareSerial nos pinos D5 e D6.
// O pino D5 (GPIO14) será o RX -> Recebe os dados do TX1 do Mega (pino 18)
// O pino D6 (GPIO12) será o TX -> Envia os dados para o RX1 do Mega (pino 19)
SoftwareSerial portaMega(14, 12);

// =========================================================================
// 4. FUNÇÃO: INICIALIZAR E CONECTAR AO WI-FI
// =========================================================================
void setup_wifi() {
  delay(10);
  Serial.println("\n[WIFI] Tentando conexão com: " + String(ssid));
  
  WiFi.begin(ssid, password);
  
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
void callback(char* topic, byte* payload, unsigned int length) {
  
  String jsonRecebido = "";
  for (int i = 0; i < length; i++) {
    jsonRecebido += (char)payload[i];
  }

  Serial.println("\n[MQTT -> MEGA] JSON recebido da nuvem:");
  Serial.println(jsonRecebido);

  // PONTE TRANSPARENTE: Envia a string JSON completa inalterada pela porta virtual
  portaMega.println(jsonRecebido);
}

// =========================================================================
// 6. FUNÇÃO: ARDUINO MEGA ➔ SITE (Receber via UART e repassar para o EMQX)
// =========================================================================
void lerDadosDoMega() {
  static String jsonAcumulado = ""; 

  // Lê a porta virtual conectada ao Mega
  while (portaMega.available()) {
    char caractere = portaMega.read();
    
    if (caractere == '\n') {
      jsonAcumulado.trim(); 
      
      if (jsonAcumulado.length() > 0) {
        Serial.println("\n[MEGA -> MQTT] JSON recebido do Arduino:");
        Serial.println(jsonAcumulado);

        client.publish(topico_status, jsonAcumulado.c_str());
      }
      
      jsonAcumulado = ""; 
    } 
    else {
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
    
    String clientId = "SmartBird-Gateway-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("Conexão bem-sucedida!");
      client.subscribe(topico_comandos, 1); 
      Serial.println("[MQTT] Monitorando tópico de comandos");
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
  // Cabo USB
  Serial.begin(115200); 
  
  // Comunicação com o Mega reduzida para 9600 baud para garantir estabilidade via software
  portaMega.begin(9600); 
  
  setup_wifi();
  
  client.setBufferSize(1024); 
  client.setKeepAlive(60); 

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback); 
}

// =========================================================================
// 9. LOOP PRINCIPAL DE EXECUÇÃO
// =========================================================================
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  
  client.loop(); 
  lerDadosDoMega(); 
}
