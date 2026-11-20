# SmartBird — Protocolo de Comunicação

**Projeto:** SmartBird  
**Versão:** 0.2.2  
**Status:** Proposta revisada — inclui telemetria dos reservatórios e dos recipientes de alimentação e hidratação; sujeita à validação e aos testes de integração  
**Controlador principal:** Arduino Mega 2560  
**Ponte de comunicação:** ESP8266  
**Transporte externo:** MQTT  
**Transporte interno:** UART TTL  
**Formato:** JSON

---

## 1. Objetivo e escopo

Este documento define o contrato de comunicação entre a Aplicação Web, o broker MQTT, o ESP8266 e o Arduino Mega 2560 do SmartBird.

São padronizados o encaminhamento de comandos, ACKs, respostas, telemetria, eventos, erros, sincronização do RTC, configuração de agendas e parâmetros operacionais, além do tratamento de falhas e perda de comunicação. O contrato também distingue os reservatórios de origem dos recipientes de consumo: reservatórios de alimento e água, pote de alimentação e bebedouro.

Os exemplos são ilustrativos. Parâmetros ainda não definidos pelo projeto estão identificados como pendentes de validação. A presença de um comando neste documento define o contrato pretendido, não significa que ele já esteja implementado.

## 2. Arquitetura da comunicação

```text
┌──────────────────┐
│   Aplicação Web  │
└────────┬─────────┘
         │ MQTT
         ▼
┌──────────────────┐
│   Broker MQTT    │
└────────┬─────────┘
         │ MQTT / Wi-Fi
         ▼
┌──────────────────┐
│      ESP8266       │
│ Ponte MQTT/UART  │
└────────┬─────────┘
         │ UART + JSON + \n
         ▼
┌──────────────────┐
│ Arduino Mega     │
│ Controle local   │
└────────┬─────────┘
         ├── Sensores
         ├── Servo de alimentação
         ├── Bomba de água
         ├── RTC
         ├── Display
         └── Avisos sonoros e visuais
```

### 2.1. Aplicação Web

A Web apresenta o estado do equipamento, monta mensagens JSON, publica comandos MQTT, recebe ACKs, respostas, telemetrias, eventos e erros, e correlaciona solicitações por identificador (`id`).

A Web também obtém a data e o horário de referência e os envia ao Arduino por meio do comando `sync_time`. A aplicação não deverá considerar uma operação concluída apenas porque publicou uma mensagem MQTT.

### 2.2. ESP8266

O ESP8266 atua como **ponte transparente MQTT/UART**. Suas responsabilidades são conectar-se ao Wi-Fi e ao broker MQTT, assinar os tópicos destinados ao dispositivo, retransmitir payloads MQTT pela UART e publicar no MQTT os payloads recebidos do Arduino.

O ESP8266 deverá preservar o conteúdo JSON: não deverá interpretar, reconstruir, alterar campos ou aplicar regras de negócio. Poderá tratar metadados necessários ao transporte, como tópico e estado da conexão, sem modificar o payload.

O ESP8266 não decide se uma operação é segura, não gerencia agendas, não atualiza o RTC por conta própria e não controla diretamente os atuadores.

### 2.3. Arduino Mega 2560

O Arduino é o controlador principal e a autoridade sobre o estado físico e operacional. Ele recebe mensagens completas, interpreta e valida JSON, verifica parâmetros e condições de segurança, executa ou recusa comandos, gerencia sensores, atuadores, agendas, manutenção e RTC, e produz ACKs, respostas, telemetria, eventos e erros.

As funções essenciais deverão continuar operando mesmo sem ESP8266, Wi-Fi ou broker MQTT.

### 2.4. Princípio de autoridade

O Arduino é a fonte de verdade sobre o estado operacional do SmartBird. A Web apresenta os dados recebidos, mas não presume que uma ação ocorreu sem confirmação do Arduino. O ESP8266 transporta mensagens, mas não interpreta seu significado operacional.

## 3. Camadas de comunicação

### 3.1. MQTT

A comunicação entre Web e ESP8266 ocorre por meio de um broker MQTT. O MQTT transporta os payloads, mas não substitui o protocolo de aplicação definido aqui.

A conexão deverá utilizar autenticação e, quando disponível no ambiente de implantação, TLS. Credenciais não deverão ser incluídas nos payloads JSON.

### 3.2. UART

A comunicação entre ESP8266 e Arduino será realizada por UART TTL.

| Parâmetro | Configuração proposta |
|---|---|
| Protocolo físico | UART TTL |
| Velocidade | 9600 baud |
| Bits de dados | 8 |
| Paridade | Nenhuma |
| Bits de parada | 1 |
| Formato | JSON |
| Delimitador | `\n` |
| UART do Mega | Serial1 |
| Pinos do Mega | TX1 (Pino 18) e RX1 (Pino 19) |
| Pinos do ESP8266 | RX em D5 (GPIO14) e TX em D6 (GPIO12) |

115200 baud é uma configuração inicial, sujeita a confirmação nos testes.

### 3.3. Conexões elétricas

| Arduino Mega 2560 | ESP8266 | Finalidade |
|---|---|---|
| TX1 — pino 18 | RX — pino D5 (GPIO14), por conversor de nível | Mega → ESP8266 |
| RX1 — pino 19 | TX — pino D6 (GPIO12), por conversor de nível | ESP8266 → Mega |
| GND | GND | Referência comum |

**Atenção:** o Mega utiliza lógica de 5 V e o ESP8266 utiliza lógica de 3,3 V. O TX1 do Mega deverá passar por conversor de nível lógico adequado antes de chegar ao RX do ESP8266. Não conectar diretamente 5 V à entrada do ESP8266. A UART principal (`Serial`) do Mega deverá permanecer disponível para depuração.

## 4. Enquadramento das mensagens UART

Cada mensagem deverá ser um objeto JSON completo, em uma única linha, terminado por `\n`.

```json
{"v":"1.0","type":"command","id":"cmd-001","source":"web","action":"get_status","data":{}}
```

O receptor acumula bytes até encontrar `\n`; somente então interpreta o JSON. Uma leitura serial não corresponde necessariamente a uma mensagem completa: mensagens podem chegar fragmentadas ou várias podem estar disponíveis na mesma leitura.

Regras:

- Uma linha contém exatamente um objeto JSON.
- Não enviar comentários nem texto de depuração na UART do protocolo.
- Chaves e identificadores não deverão conter acentos.
- Strings utilizam UTF-8.
- Quebras de linha em valores textuais deverão ser escapadas conforme JSON.
- O receptor valida a mensagem completa antes de executar qualquer ação.
- O firmware deverá impor um limite máximo de bytes por mensagem.

Ao exceder o limite do buffer, o receptor descarta os bytes até o próximo `\n`, registra a ocorrência e não executa conteúdo parcial. **O tamanho máximo numérico do buffer está estabelecido em 1024 bytes**, o que garante a leitura completa de payloads estruturados sem truncamento de dados ou estouro de memória.

## 5. Estrutura padrão JSON

### 5.1. Campos comuns

| Campo | Tipo | Obrigatório | Finalidade |
|---|---|---:|---|
| `v` | String | Sim | Versão do protocolo |
| `type` | String | Sim | Tipo da mensagem |
| `id` | String | Conforme tipo | Identificador da mensagem/solicitação |
| `source` | String | Sim | Origem lógica |
| `action` | String | Em comandos | Ação solicitada |
| `status` | String | Em ACK/response | Resultado da aceitação ou execução |
| `data` | Objeto | Conforme tipo | Parâmetros ou dados retornados |
| `error` | Objeto | Em erros | Detalhes padronizados da falha |
| `timestamp` | String | Em telemetria/eventos, quando disponível | Data e hora da ocorrência |

### 5.2. Versão

`v` identifica a versão do contrato. Os exemplos utilizam `1.0`. Mensagens de versão não suportada não deverão executar comandos; o Arduino deverá informar incompatibilidade quando for possível identificar a mensagem com segurança.

### 5.3. Origem (`source`)

`source` identifica quem originou logicamente a mensagem, não o último equipamento que a transportou. Valores previstos: `web` e `arduino`. O ESP8266 não substitui `source` por `ESP8266`, pois atua apenas como ponte.

### 5.4. Identificador (`id`)

Toda solicitação originada na Web deverá possuir identificador único. O mesmo `id` será preservado no ACK e na response final. Exemplos: `cmd-001`, `sync-001`. A estratégia de geração deverá evitar colisões entre solicitações pendentes.

Telemetrias periódicas e eventos espontâneos não precisam de `id` de solicitação; poderão ter identificador próprio de evento, se necessário.

### 5.5. Campo `data`

`data` agrupa parâmetros de comandos ou dados retornados. Comandos sem parâmetros utilizarão `data: {}`.

## 6. Tipos de mensagem e ciclo de solicitação

| Tipo | Direção usual | Finalidade |
|---|---|---|
| `command` | Web → Arduino | Solicitar ação ou consulta |
| `ack` | Arduino → Web | Confirmar recebimento e aceitação/recusa inicial |
| `response` | Arduino → Web | Informar resultado final |
| `telemetry` | Arduino → Web | Publicar leituras e estados |
| `event` | Arduino → Web | Notificar ocorrência relevante |
| `error` | Arduino → Web | Informar falha de protocolo/processamento ou ocorrência geral |

### 6.1. ACK não significa conclusão

Para operações demoradas, o fluxo será: Request, ACK `accepted`, execução local e response final `success` ou `failure`. Se a solicitação não puder ser aceita, o Arduino envia ACK `rejected` com o motivo e não inicia a operação.

Consultas rápidas também utilizarão ACK e response para manter um ciclo uniforme. Telemetrias e eventos espontâneos não exigem ACK ou response.

### 6.2. Exemplo genérico

**Request — Web → Arduino**

```json
{
  "v": "1.0",
  "type": "command",
  "id": "cmd-001",
  "source": "web",
  "action": "get_status",
  "data": {}
}
```

**ACK — Arduino → Web**

```json
{
  "v": "1.0",
  "type": "ack",
  "id": "cmd-001",
  "source": "arduino",
  "status": "accepted"
}
```

**Response — Arduino → Web**

```json
{
  "v": "1.0",
  "type": "response",
  "id": "cmd-001",
  "source": "arduino",
  "status": "success",
  "data": {"system": "operational"}
}
```

## 7. Catálogo de comandos

| `action` | Finalidade | Natureza |
|---|---|---|
| `get_status` | Consultar estado geral | Consulta |
| `get_telemetry` | Solicitar leituras atuais | Consulta |
| `feed_manual` | Solicitar alimentação manual | Operação |
| `water_manual` | Solicitar hidratação manual | Operação |
| `set_schedule` | Criar/substituir agendas | Configuração |
| `get_schedule` | Consultar agendas | Consulta |
| `remove_schedule` | Remover/desativar agendamento | Configuração |
| `set_feeding_config` | Atualizar parâmetros de alimentação | Configuração |
| `set_watering_config` | Atualizar parâmetros de hidratação | Configuração |
| `ack_cleaning` | Confirmar limpeza realizada | Manutenção |
| `sync_time` | Ajustar RTC com horário enviado pela Web | Sincronização |
| `get_system_info` | Consultar identificação e versão | Consulta |

## 8. Protocolos de consulta

### 8.1. `get_status` — consultar estado geral

**Request**

```json
{"v":"1.0","type":"command","id":"cmd-001","source":"web","action":"get_status","data":{}}
```

**ACK**

```json
{"v":"1.0","type":"ack","id":"cmd-001","source":"arduino","status":"accepted"}
```

**Response**

```json
{
  "v":"1.0","type":"response","id":"cmd-001","source":"arduino","status":"success",
  "data":{
    "system":"operational",
    "feeding":"idle",
    "watering":"idle",
    "reservoirs":{
      "food":{"level_percent":76.0,"valid":true,"low":false},
      "water":{"level_percent":62.0,"valid":true,"low":false}
    },
    "containers":{
      "food_bowl":{"weight_g":34.5,"valid":true},
      "water_bowl":{"level_state":"ok","low":false,"valid":true}
    },
    "maintenance":"normal",
    "rtc":"available"
  }
}
```

Os estados são ilustrativos e deverão corresponder às enumerações finais do firmware.

### 8.2. `get_telemetry` — consultar leituras

**Request**

```json
{"v":"1.0","type":"command","id":"cmd-002","source":"web","action":"get_telemetry","data":{}}
```

**ACK**

```json
{"v":"1.0","type":"ack","id":"cmd-002","source":"arduino","status":"accepted"}
```

**Response**

```json
{
  "v":"1.0",
  "type":"response",
  "id":"cmd-002",
  "source":"arduino",
  "status":"success",
  "data":{
    "reservoirs":{
      "food":{"level_percent":76.0,"distance_cm":12.4,"valid":true,"low":false},
      "water":{"level_percent":62.0,"distance_cm":15.1,"valid":true,"low":false}
    },
    "containers":{
      "food_bowl":{"weight_g":34.5,"valid":true},
      "water_bowl":{"level_state":"low","low":true,"valid":true}
    },
    "feeding":{"state":"idle","bowl_weight_g":34.5},
    "watering":{"state":"checking","pump_on":false,"bowl_level":"low"}
  }
}
```

Valores fictícios. `reservoirs` representa os recipientes de armazenamento que fornecem alimento e água aos mecanismos. `containers` representa os recipientes acessíveis às aves. Os campos `valid` indicam se a leitura correspondente pôde ser obtida e considerada utilizável pelo firmware; não devem ser substituídos por zero quando a leitura estiver indisponível.

O pote de alimentação é monitorado por célula de carga e seu valor é expresso em gramas (`weight_g`), não em porcentagem. O bebedouro é monitorado por uma boia de dois fios e fornece somente um estado binário: `low` ou `ok`. Não existe medição percentual do nível do bebedouro.

A boia utiliza entrada digital com `INPUT_PULLUP`: no hardware atualmente testado, contato fechado (`LOW`) significa nível baixo e contato aberto (`HIGH`) significa nível adequado. Como uma boia simples de dois fios não permite distinguir eletricamente um recipiente cheio de um fio rompido/ desconectado, `valid` não representa diagnóstico completo de integridade do cabo ou do sensor. Essa limitação deverá ser considerada na interface e nos testes.

O campo `watering.pump_on` informa o estado lógico do comando do relé/bomba, não uma confirmação elétrica ou hidráulica de que a bomba realmente está funcionando. Valores e estados são ilustrativos e deverão refletir as leituras efetivas do Arduino.

### 8.3. `get_schedule` — consultar agendas

**Request**

```json
{"v":"1.0","type":"command","id":"cmd-003","source":"web","action":"get_schedule","data":{}}
```

**ACK**

```json
{"v":"1.0","type":"ack","id":"cmd-003","source":"arduino","status":"accepted"}
```

**Response**

```json
{
  "v":"1.0","type":"response","id":"cmd-003","source":"arduino","status":"success",
  "data":{
    "feeding":{"enabled":true,"times":["08:00","18:00"]},
    "watering":{"enabled":true,"times":["08:00","18:00"]}
  }
}
```

Horários ilustrativos. Quantidade máxima, ordenação e persistência deverão respeitar as regras finais do firmware.

### 8.4. `get_system_info` — consultar informações do sistema

**Request**

```json
{"v":"1.0","type":"command","id":"cmd-004","source":"web","action":"get_system_info","data":{}}
```

**ACK**

```json
{"v":"1.0","type":"ack","id":"cmd-004","source":"arduino","status":"accepted"}
```

**Response**

```json
{
  "v":"1.0","type":"response","id":"cmd-004","source":"arduino","status":"success",
  "data":{"device":"smartbird","controller":"arduino_mega_2560","firmware_version":"0.1.0","protocol_version":"1.0"}
}
```

## 9. Protocolos de operação

### 9.1. `feed_manual` — alimentação manual

**Request**

```json
{"v":"1.0","type":"command","id":"cmd-010","source":"web","action":"feed_manual","data":{}}
```

**ACK**

```json
{"v":"1.0","type":"ack","id":"cmd-010","source":"arduino","status":"accepted"}
```

**Response — sucesso**

```json
{
  "v":"1.0","type":"response","id":"cmd-010","source":"arduino","status":"success",
  "data":{"module":"feeding","result":"completed"}
}
```

**Response — falha após aceitação**

```json
{
  "v":"1.0","type":"response","id":"cmd-010","source":"arduino","status":"failure",
  "error":{"code":"ACTUATOR_TIMEOUT","message":"Feeding operation exceeded its time limit"}
}
```

O painel Web bloqueia o envio de `feed_manual` quando o reservatório está em nível baixo ou quando a disponibilidade de ração no reservatório ou o peso no pote não podem ser confirmados por leituras válidas e atuais. O Arduino valida as condições de segurança antes do acionamento. O ciclo deverá respeitar limites de tempo e leituras dos sensores. O servo não poderá permanecer acionado indefinidamente em caso de falha. Recusa anterior ao início será comunicada por ACK `rejected`.

### 9.2. `water_manual` — hidratação manual

**Request**

```json
{"v":"1.0","type":"command","id":"cmd-011","source":"web","action":"water_manual","data":{}}
```

**ACK**

```json
{"v":"1.0","type":"ack","id":"cmd-011","source":"arduino","status":"accepted"}
```

**Response — sucesso**

```json
{
  "v":"1.0","type":"response","id":"cmd-011","source":"arduino","status":"success",
  "data":{"module":"watering","result":"completed"}
}
```

**Response — falha após aceitação**

```json
{
  "v":"1.0","type":"response","id":"cmd-011","source":"arduino","status":"failure",
  "error":{"code":"WATER_SENSOR_FAILURE","message":"Water level could not be verified safely"}
}
```

A bomba deverá ser interrompida quando a boia indicar nível adequado, quando o tempo máximo for excedido ou quando ocorrer falha que impeça a operação segura. O nível do bebedouro é binário (`low`/`ok`), não percentual. A disponibilidade de água no reservatório de origem é verificada separadamente pelo sensor ultrassônico.

### 9.3. `ack_cleaning` — confirmar limpeza

**Request**

```json
{"v":"1.0","type":"command","id":"cmd-012","source":"web","action":"ack_cleaning","data":{}}
```

**ACK**

```json
{"v":"1.0","type":"ack","id":"cmd-012","source":"arduino","status":"accepted"}
```

**Response**

```json
{
  "v":"1.0","type":"response","id":"cmd-012","source":"arduino","status":"success",
  "data":{"cleaning_due":false,"maintenance_timer_reset":true}
}
```

A confirmação reinicia o intervalo de manutenção conforme as regras do projeto; não inicia alimentação ou hidratação. O intervalo atualmente previsto é de dois dias, sujeito à configuração final do módulo.

## 10. Protocolos de configuração

### 10.1. `set_schedule` — configurar agendas

**Request**

```json
{
  "v":"1.0","type":"command","id":"cmd-020","source":"web","action":"set_schedule",
  "data":{
    "feeding":{"enabled":true,"times":["08:00","18:00"]},
    "watering":{"enabled":true,"times":["08:00","18:00"]}
  }
}
```

**ACK**

```json
{"v":"1.0","type":"ack","id":"cmd-020","source":"arduino","status":"accepted"}
```

**Response**

```json
{
  "v":"1.0","type":"response","id":"cmd-020","source":"arduino","status":"success",
  "data":{"schedule_updated":true}
}
```

O Arduino valida formato, quantidade máxima, duplicidade e demais regras. A alteração só será confirmada após atualização efetiva. A estrutura deverá ser compatível com o modelo de agenda do firmware.

### 10.2. `remove_schedule` — remover agendamento

**Request**

```json
{
  "v":"1.0","type":"command","id":"cmd-021","source":"web","action":"remove_schedule",
  "data":{"module":"feeding","time":"18:00"}
}
```

**ACK**

```json
{"v":"1.0","type":"ack","id":"cmd-021","source":"arduino","status":"accepted"}
```

**Response**

```json
{
  "v":"1.0","type":"response","id":"cmd-021","source":"arduino","status":"success",
  "data":{"module":"feeding","time":"18:00","removed":true}
}
```

**Pendente:** decidir se a remoção será identificada por horário, índice ou identificador próprio.

### 10.3. `set_feeding_config` — configurar alimentação

**Request — exemplo de proposta**

```json
{
  "v":"1.0","type":"command","id":"cmd-022","source":"web","action":"set_feeding_config",
  "data":{
    "target_weight_g":20.0,
    "max_duration_ms":5000,
    "reference_distance_cm":25.0,
    "useful_height_cm":20.0,
    "low_threshold_percent":20.0,
    "full_threshold_percent":95.0,
    "cleaning_interval_seconds":172800
  }
}
```

**ACK**

```json
{"v":"1.0","type":"ack","id":"cmd-022","source":"arduino","status":"accepted"}
```

**Response**

```json
{
  "v":"1.0","type":"response","id":"cmd-022","source":"arduino","status":"success",
  "data":{"configuration_updated":true}
}
```

O intervalo de limpeza via JSON usa `cleaning_interval_seconds`, em segundos inteiros. O valor de 172800 segundos corresponde a 48 horas. Os limites operacionais definitivos e os demais campos continuam sujeitos à configuração real do módulo e à validação no Arduino.

### 10.4. `set_watering_config` — configuração de hidratação

O bebedouro não possui medição percentual. Seu sensor de boia informa apenas se o nível está baixo ou adequado. Portanto, o protocolo não deverá receber nem enviar `target_level_percent` para o bebedouro.

Na implementação atual, o abastecimento é iniciado quando a boia indica nível baixo e o reservatório de origem possui água disponível. A bomba é desligada quando a boia indica nível adequado. O firmware possui um limite provisório fixo de 15000 ms para o bombeamento contínuo.

**Situação atual:** o limite de 15000 ms está definido no código do módulo e não é configurável por JSON. O comando `set_watering_config` permanece pendente de revisão: até que sejam definidos e implementados parâmetros configuráveis, a aplicação não deverá utilizá-lo nem enviar percentual-alvo para o bebedouro.

## 11. Sincronização temporal

### 11.1. Responsabilidade e funcionamento

A Aplicação Web envia data e horário ao Arduino por meio do comando `sync_time`. O ESP8266 recebe o payload MQTT e o retransmite pela UART sem interpretar ou modificar o JSON. O Arduino valida os dados e atualiza o RTC.

Após a sincronização, o RTC continua contando o tempo localmente. Agendas e rotinas não dependerão de conexão permanente com Web, Wi-Fi ou broker MQTT.

### 11.2. Formato

A proposta utiliza ISO 8601 com deslocamento de fuso explícito. Exemplo: `2026-11-10T14:30:00-03:00`. O deslocamento é ilustrativo e deverá corresponder ao horário enviado pela aplicação.

### 11.3. `sync_time` — sincronizar RTC

**Request**

```json
{
  "v":"1.0","type":"command","id":"sync-001","source":"web","action":"sync_time",
  "data":{"datetime":"2026-11-10T14:30:00-03:00"}
}
```

**ACK**

```json
{"v":"1.0","type":"ack","id":"sync-001","source":"arduino","status":"accepted"}
```

**Response — sucesso**

```json
{
  "v":"1.0","type":"response","id":"sync-001","source":"arduino","status":"success",
  "data":{"rtc_updated":true,"datetime":"2026-11-10T14:30:00-03:00"}
}
```

**Response — falha**

```json
{
  "v":"1.0","type":"response","id":"sync-001","source":"arduino","status":"failure",
  "error":{"code":"RTC_FAILURE","message":"RTC could not be updated"}
}
```

### 11.4. Regras

- A Web envia data e hora válidas, incluindo deslocamento de fuso.
- O Arduino valida formato e componentes antes de atualizar o RTC.
- A atualização ocorre somente após validação.
- O Arduino confirma se a atualização foi concluída.
- O RTC continua contando o tempo sem internet.
- A sincronização poderá ocorrer na inicialização da aplicação ou por solicitação manual.
- A frequência de sincronização automática ainda deverá ser definida.

**Ponto técnico a validar:** conversão do ISO 8601 com deslocamento para a representação do módulo RTC e tratamento de horário local versus UTC. O contrato externo está proposto, mas a conversão interna precisa ser implementada e testada.

## 12. Telemetria

Telemetria é uma mensagem espontânea do Arduino. Não está vinculada a um comando e não exige ACK ou response.

A estrutura deverá separar explicitamente os reservatórios de armazenamento dos recipientes de consumo. Os quatro pontos de medição do SmartBird são:

| Identificador JSON | Local | Sensor | Grandeza exposta |
|---|---|---|---|
| `reservoirs.food` | Reservatório de alimento | HC-SR04 | Percentual, distância e validade |
| `reservoirs.water` | Reservatório de água que abastece a bomba | HC-SR04 | Percentual, distância e validade |
| `containers.food_bowl` | Pote de alimento acessível à ave | Célula de carga + HX711 | Peso em gramas e validade |
| `containers.water_bowl` | Bebedouro acessível à ave | Boia de dois fios | Estado `low` ou `ok` e validade da leitura |

**Exemplo — Arduino → Web**

```json
{
  "v":"1.0",
  "type":"telemetry",
  "source":"arduino",
  "timestamp":"2026-11-10T14:35:00-03:00",
  "data":{
    "reservoirs":{
      "food":{"level_percent":76.0,"distance_cm":12.4,"valid":true,"low":false},
      "water":{"level_percent":62.0,"distance_cm":15.1,"valid":true,"low":false}
    },
    "containers":{
      "food_bowl":{"weight_g":34.5,"valid":true},
      "water_bowl":{"level_state":"ok","low":false,"valid":true}
    },
    "feeding":{"state":"idle","bowl_weight_g":34.5,"gate_open":false},
    "watering":{"state":"idle","bowl_level":"ok","pump_on":false},
    "maintenance":{"cleaning_due":false}
  }
}
```

Valores fictícios. `level_percent` aplica-se somente aos reservatórios monitorados por HC-SR04. `weight_g` aplica-se somente ao pote de alimentação. `level_state` do bebedouro aceita `low` ou `ok`; não deve ser representado como porcentagem, volume ou peso.

A boia atualmente é lida com `INPUT_PULLUP`: `LOW` corresponde a nível baixo e `HIGH` a nível adequado. A leitura binária não permite estimar quanto falta para encher o bebedouro. Além disso, um fio desconectado pode produzir a mesma leitura elétrica de nível adequado; o protocolo não deverá afirmar que esse tipo de falha foi diagnosticado.

Quando uma leitura não puder ser obtida ou validada, o firmware deverá informar `valid:false` e não deverá converter a ausência de leitura em zero, `low:true` ou `ok`. Para o bebedouro, a capacidade de detectar falha física da boia é limitada pelo circuito de dois fios atualmente utilizado.

`watering.pump_on` representa o estado lógico do acionamento. Não é sensor de fluxo e não comprova que a água está efetivamente sendo bombeada. A interface deverá distinguir estado comandado de confirmação física, caso futuramente seja instalado um sensor de fluxo.

A telemetria deverá manter os controles locais sem bloqueio durante a transmissão. O intervalo periódico e a publicação por mudança de estado ainda deverão ser definidos.

## 13. Eventos e notificações

Eventos são mensagens espontâneas emitidas pelo Arduino para comunicar ocorrências relevantes. Não exigem ACK ou response de solicitação.

### 13.1. Estrutura geral

```json
{
  "v":"1.0","type":"event","source":"arduino",
  "timestamp":"2026-11-10T14:35:00-03:00",
  "data":{"name":"event_name","details":{}}
}
```

### 13.2. Alimentação concluída

```json
{
  "v":"1.0","type":"event","source":"arduino",
  "timestamp":"2026-11-10T14:35:00-03:00",
  "data":{"name":"feeding_completed","origin":"scheduled"}
}
```

### 13.3. Hidratação concluída

```json
{
  "v":"1.0","type":"event","source":"arduino",
  "timestamp":"2026-11-10T14:35:00-03:00",
  "data":{"name":"watering_completed","origin":"automatic","container":"water_bowl","level_state":"ok"}
}
```

O evento indica que a boia passou a informar nível adequado e que o comando da bomba foi desligado. Não representa um percentual ou volume atingido. A origem poderá ser `automatic` ou `manual`, conforme a forma de início da operação, quando essa distinção estiver implementada.

### 13.4. Nível baixo em reservatório ou recipiente de consumo

Os eventos devem identificar claramente qual recipiente originou a condição. Reservatório de origem e recipiente de consumo não são intercambiáveis.

**Reservatório de alimento ou água com nível baixo (HC-SR04)**

```json
{
  "v":"1.0","type":"event","source":"arduino",
  "timestamp":"2026-11-10T14:35:00-03:00",
  "data":{"name":"reservoir_low","reservoir":"water","level_percent":15.0}
}
```

O campo `reservoir` aceita `food` ou `water`. `level_percent` é aplicável aos reservatórios monitorados por ultrassom.

**Bebedouro com nível baixo (boia)**

```json
{
  "v":"1.0","type":"event","source":"arduino",
  "timestamp":"2026-11-10T14:35:00-03:00",
  "data":{"name":"container_level_low","container":"water_bowl","level_state":"low"}
}
```

O evento do bebedouro não inclui percentual, distância ou volume, pois a boia fornece somente um estado binário.

**Pote de alimento**

O peso do pote deverá ser informado na telemetria em gramas. Um evento `food_bowl_low` somente poderá ser emitido quando o projeto definir e implementar um limite mínimo de peso; esse limite ainda não está definido no firmware atual.

Para evitar notificações repetidas, eventos de nível deverão preferencialmente ser emitidos na transição de estado (por exemplo, de `ok` para `low`) e não a cada leitura periódica. A estratégia final de deduplicação e reenvio permanece sujeita à implementação.

### 13.5. Limpeza pendente

```json
{
  "v":"1.0","type":"event","source":"arduino",
  "timestamp":"2026-11-10T14:35:00-03:00",
  "data":{"name":"cleaning_due"}
}
```

### 13.6. Execução programada perdida

Se o Arduino reiniciar e identificar que um horário programado passou sem execução, não deverá iniciar automaticamente a alimentação ou hidratação atrasada. Deverá manter o registro e notificar a Web para que o usuário decida se deseja executar a operação manualmente.

```json
{
  "v":"1.0","type":"event","source":"arduino",
  "timestamp":"2026-11-10T14:35:00-03:00",
  "data":{
    "name":"scheduled_execution_missed",
    "module":"feeding",
    "scheduled_time":"14:00",
    "action_required":"user_decision"
  }
}
```

A recusa da sugestão não deverá apagar indevidamente o histórico. A pendência operacional deverá ser resolvida conforme a regra do módulo, por exemplo, após uma execução posterior concluída.

### 13.7. Regras

Eventos terão nomes padronizados e timestamp quando o RTC estiver disponível. O Arduino continuará funcionando mesmo se não conseguir transmitir. A estratégia de armazenamento e reenvio após perda de conexão deverá ser definida.

**Pendente:** identificador único de evento e política de persistência/reenvio.

## 14. Erros e recusas

### 14.1. Estrutura de erro

```json
{
  "v":"1.0","type":"error","id":"cmd-010","source":"arduino",
  "error":{"code":"SAFETY_BLOCK","message":"Operation blocked by a safety condition"}
}
```

Quando associado a uma solicitação, o erro repete seu `id`. Erros gerais poderão omitir `id`.

Recusa antes da execução: ACK `rejected` com motivo. Falha após aceitação: response final `failure`. O tipo `error` fica reservado a falhas de protocolo/processamento ou ocorrências gerais que não sejam melhor representadas pela response da operação.

### 14.2. Códigos iniciais

| Código | Significado |
|---|---|
| `INVALID_JSON` | JSON inválido |
| `INVALID_MESSAGE` | Estrutura de mensagem inválida |
| `UNKNOWN_COMMAND` | Ação não reconhecida |
| `INVALID_PARAMETER` | Parâmetro ausente ou inválido |
| `UNSUPPORTED_VERSION` | Versão não suportada |
| `BUSY` | Módulo ocupado |
| `SAFETY_BLOCK` | Operação impedida por segurança |
| `WEIGHT_SENSOR_FAILURE` | Falha no sensor de peso |
| `WATER_SENSOR_FAILURE` | Falha no sensor de água |
| `RESERVOIR_SENSOR_FAILURE` | Falha na leitura de reservatório |
| `ACTUATOR_TIMEOUT` | Atuador excedeu tempo máximo |
| `RTC_FAILURE` | Falha no RTC |
| `INTERNAL_ERROR` | Falha interna não classificada |

A mensagem textual serve para diagnóstico; a lógica deverá depender principalmente do código.

### 14.3. Status de ACK

| Status | Significado |
|---|---|
| `accepted` | Solicitação válida e aceita para processamento |
| `rejected` | Solicitação recebida, mas recusada antes da execução |

### 14.4. Status de response

| Status | Significado |
|---|---|
| `success` | Operação ou consulta concluída com sucesso |
| `failure` | Operação aceita, mas não concluída com sucesso |

## 15. Estados operacionais expostos à Web

Convenção externa proposta:

| Módulo | Estado | Significado |
|---|---|---|
| Sistema | `initializing` | Inicialização em andamento |
| Sistema | `operational` | Operação normal |
| Sistema | `alert` | Alerta que requer atenção |
| Sistema | `error` | Falha que compromete uma função |
| Alimentação | `idle` | Sem operação em andamento |
| Alimentação | `checking` | Verificação prévia |
| Alimentação | `running` | Alimentação em andamento |
| Alimentação | `completed` | Ciclo concluído |
| Alimentação | `error` | Falha no ciclo |
| Hidratação | `idle` | Sem operação em andamento |
| Hidratação | `checking` | Verificação prévia |
| Hidratação | `running` | Bomba em operação |
| Hidratação | `completed` | Ciclo concluído |
| Hidratação | `error` | Falha no ciclo |
| RTC | `available` | Relógio disponível |
| RTC | `invalid` | Horário não validado |
| RTC | `failure` | Falha no RTC |

`idle` não significa ausência de alertas. Estado operacional e condição de alerta deverão ser apresentados separadamente quando necessário.

Além do estado de execução dos módulos, a Web deverá apresentar separadamente as condições dos recipientes: nível percentual dos reservatórios de origem, peso em gramas do pote de alimento e estado binário (`low`/`ok`) do bebedouro. `watering.completed` significa que a boia indicou nível adequado e o comando da bomba foi desligado; não representa um percentual-alvo atingido.

## 16. Tópicos MQTT

### 16.1. Convenção proposta

A arquitetura foi simplificada para utilizar estritamente dois tópicos principais, centralizando o fluxo e facilitando a manutenção:

```text
smartbird/{device_id}/comandos
smartbird/{device_id}/status
```

`{device_id}` identificará unicamente cada equipamento SmartBird; seu valor definitivo ainda deverá ser definido.

### 16.2. Direções

| Tópico | Publicador | Assinante principal | Conteúdo |
|---|---|---|---|
| `smartbird/{device_id}/comandos` | Web | ESP8266 | Comandos e solicitações originadas da Web |
| `smartbird/{device_id}/status` | ESP8266 (encaminhando Arduino) | Web | ACKs, respostas, telemetria, eventos e erros |

O ESP8266 não modifica o payload. O tópico identifica o canal de transporte; o JSON mantém o contrato de aplicação.

### 16.3. QoS e retained

O sistema utilizará **QoS 1 (At least once)** no canal de comandos (`/comandos`) para garantir a entrega confiável das solicitações críticas originadas pela aplicação Web. Para o canal de respostas e telemetria (`/status`), será utilizado **QoS 0 (At most once)**.

**Mensagens retidas (retained):** Nenhuma mensagem de comando, acionamento ou status utilizará a flag *retained*. Isso previne estritamente que comandos antigos fiquem agarrados no broker e sejam executados de forma indesejada pelo equipamento logo após uma reconexão de rede.

**QoS MQTT não substitui ACK de aplicação.** Confirmação de entrega MQTT não significa que o Arduino recebeu, validou ou executou o comando.

## 17. Segurança e validação

1. O Arduino valida todas as mensagens antes de executar ações.
2. Mensagens inválidas, incompletas ou incompatíveis não acionam atuadores.
3. Parâmetros são validados quanto a tipo, faixa e presença.
4. Operações físicas possuem limites de tempo e condições explícitas de interrupção.
5. A bomba não é acionada se não for possível verificar o nível com segurança.
6. O abastecimento utiliza o estado binário da boia do bebedouro e a disponibilidade percentual do reservatório de água de origem; esses dois sensores medem recipientes diferentes.
7. O pote de alimento é monitorado por peso em gramas, não por percentual. O bebedouro não fornece percentual.
8. A leitura HIGH da boia de dois fios não permite distinguir nível adequado de cabo desconectado; essa limitação deverá ser documentada.
9. O servo não permanece acionado indefinidamente em caso de falha do sensor.
10. Rotinas automáticas e comandos manuais respeitam as mesmas condições de segurança.
11. A Web apresenta o resultado informado pelo Arduino, não presume sucesso pela publicação MQTT.
12. Credenciais do broker não são incluídas nos payloads.
13. O broker utiliza autenticação e, quando disponível, transporte criptografado.
14. Funções essenciais continuam operando sem conexão externa.
15. Falha de comunicação é distinguida de falha de sensor ou atuador.

## 18. Duplicidade, timeout e recuperação

### 18.1. Comandos duplicados

O Arduino deverá evitar executar duas vezes uma operação física recebida com o mesmo `id`. A implementação deverá manter registro dos identificadores recentes e resultados correspondentes. Ao receber novamente um `id` processado, deverá retornar o resultado conhecido ou informar que já foi tratado, sem iniciar novamente a operação.

O período de retenção e a persistência após reinicialização ainda deverão ser definidos.

### 18.2. Timeout

A Web deverá considerar timeout se não receber ACK dentro do intervalo definido. Se recebeu ACK `accepted`, mas ainda não recebeu response, deverá apresentar operação em andamento ou resultado desconhecido, e não falha automática.

A Web não deverá reenviar automaticamente comandos de acionamento sem consultar o estado ou verificar o identificador original.

### 18.3. Perda de MQTT ou Wi-Fi

O Arduino continua executando agendas e controles locais. O ESP8266 tenta restabelecer a conexão. A Web indica desconexão ou dados desatualizados. A estratégia de armazenamento e reenvio de eventos deverá ser definida.

### 18.4. Perda de UART

O ESP8266 detecta ausência de comunicação conforme a estratégia implementada. O Arduino mantém funções locais. O ESP8266 não apresenta comandos como executados sem resposta do Arduino. Após recuperação, a Web poderá consultar o estado atual.

### 18.5. Mensagem incompleta ou buffer excedido

O receptor descarta conteúdo incompleto conforme timeout ou limite do buffer e recupera o enquadramento no próximo `\n`. Nenhum fragmento poderá ser executado.

## 19. Organização sugerida do software

### 19.1. Arduino Mega

- `Comunicacao`: leitura/escrita UART, buffer e delimitação por `\n`.
- `Protocolo`: interpretação JSON, validação e despacho das ações.
- `Controle`: coordenação dos módulos.
- `Reservatorios`: leitura e validação de níveis.
- `Alimentacao`: ciclo de alimentação.
- `Hidratacao`: ciclo de hidratação.
- `Tempo`: comunicação com RTC e referência temporal.
- `Manutencao`: lembretes e confirmações de limpeza.
- `Interface`: display, botões e avisos locais.

A camada de comunicação transporta mensagens; a camada de protocolo interpreta conteúdo; módulos operacionais executam suas regras.

### 19.2. ESP8266

- `WiFi_Manager`: conexão e reconexão Wi-Fi.
- `MQTT_Client`: conexão, assinatura e publicação MQTT.
- `UART_Bridge`: encaminhamento transparente entre MQTT e UART.
- `Connection_Manager`: acompanhamento das conexões.

O ESP8266 não implementa regras de alimentação, hidratação, agenda ou manutenção.

### 19.3. Aplicação Web

- `mqtt_client`: conexão com broker.
- `command_service`: montagem e publicação de comandos.
- `response_handler`: correlação de ACKs/responses por `id`.
- `telemetry_handler`: atualização dos dados.
- `event_handler`: notificações.
- `time_service`: obtenção e envio de horário por `sync_time`.

## 20. Critérios de validação

- [ ] Web publica comandos no tópico correto.
- [ ] ESP8266 retransmite payload MQTT sem alterações.
- [ ] Arduino recompõe corretamente mensagens UART fragmentadas.
- [ ] JSON válido é interpretado corretamente.
- [ ] Mensagens inválidas não provocam acionamentos.
- [ ] Versão, tipo, origem, ação e parâmetros são validados.
- [ ] ACK e response preservam o `id` do request.
- [ ] Web diferencia ACK de conclusão.
- [ ] Consultas retornam dados esperados.
- [ ] Alimentação e hidratação respeitam condições de segurança.
- [ ] Agendas são validadas e aplicadas.
- [ ] `sync_time` atualiza RTC e retorna confirmação.
- [ ] RTC continua contando sem internet.
- [ ] Telemetria indica validade das leituras.
- [ ] Telemetria distingue reservatórios de origem e recipientes de consumo.
- [ ] Reservatórios de alimento e água são apresentados em percentual, quando a leitura ultrassônica for válida.
- [ ] Pote de alimento é apresentado em gramas, com indicação de validade da leitura da célula de carga.
- [ ] Bebedouro é apresentado somente como nível baixo ou adequado, sem percentual inventado.
- [ ] Eventos de nível identificam corretamente o recipiente e não confundem reservatório de origem com bebedouro.
- [ ] Eventos e falhas são encaminhados corretamente.
- [ ] Comandos duplicados não provocam acionamentos repetidos.
- [ ] Buffer excedido não resulta em execução parcial.
- [ ] Perda de Wi-Fi, MQTT ou ESP8266 não interrompe rotinas locais.
- [ ] Web apresenta dados desatualizados ou indisponíveis corretamente.

## 21. Decisões pendentes

- [ ] Identificador definitivo (`device_id`).
- [ ] Biblioteca JSON e dimensionamento de memória no Mega.
- [ ] Campos e limites finais de `set_feeding_config`.
- [ ] Campos e limites finais de `set_watering_config`.
- [ ] Definir se `set_watering_config` será mantido e quais parâmetros configuráveis oferecerá; não utilizar percentual-alvo para bebedouro com boia binária.
- [ ] Definir limite mínimo de peso para eventual alerta de pote de alimento baixo.
- [ ] Avaliar forma de detectar fio rompido/falha da boia, pois o circuito atual não diferencia essa condição de nível adequado.
- [ ] Validar se o limite fixo de bombeamento de 15000 ms será mantido ou substituído por configuração.
- [ ] Estrutura final de `set_schedule` e `remove_schedule`.
- [ ] Persistência das agendas.
- [ ] Frequência de telemetria.
- [ ] Identificador único de evento e política de reenvio.
- [ ] Timeouts de ACK, response e UART.
- [ ] Retenção de identificadores para deduplicação.
- [ ] Persistência do registro de comandos após reinicialização.
- [ ] Frequência de sincronização automática do RTC.
- [ ] Conversão de fuso horário no Arduino.
- [ ] Autenticação, TLS e permissões do broker.
- [ ] Recuperação e apresentação de dados após reconexão.

## 22. Controle de versões

| Versão | Alterações |
|---|---|
| `0.1.0` | Estrutura inicial, tipos de mensagem, comandos e regras preliminares |
| `0.2.0` | Arquitetura de ponte ESP8266 transparente; tópicos MQTT; ciclo Request/ACK/Response; protocolos de configuração, eventos, telemetria e sincronização RTC iniciada pela Web |
| `0.2.1` | Adaptação para ESP8266 NodeMCU V3; redução da UART para 9600 baud via SoftwareSerial (pinos D5 e D6); fixação do buffer UART em 1024 bytes; consolidação para apenas 2 tópicos MQTT (`comandos` e `status`); definição de QoS (1 e 0) e proibição de mensagens retidas (retained) |
| `0.2.2` | Inclusão explícita dos níveis dos recipientes de consumo; distinção entre reservatórios ultrassônicos, pote de alimento medido em gramas e bebedouro monitorado por boia binária; revisão de telemetria, consulta, eventos, estados e configuração de hidratação |
| `1.0.0` | A definir após validação da equipe e testes de integração |

## 23. Manutenção do documento

Toda alteração incompatível deverá ser discutida antes de modificar os firmwares da Web, ESP8266 ou Arduino. Os três componentes deverão utilizar a mesma versão do protocolo.

A implementação deverá seguir este documento. Se os testes exigirem mudanças, elas deverão ser registradas aqui antes ou junto da atualização do código.

---

**Fim do documento — SmartBird Protocolo de Comunicação v0.2.2**




