SmartBird — Protocolo de Comunicação

Projeto: SmartBird
Versão do documento: 0.1.0
Status: Proposta inicial — aguardando validação da equipe
Comunicação: UART entre Arduino Mega 2560 e ESP32
Formato das mensagens: JSON

⸻

1. Objetivo

Este documento define o protocolo de comunicação entre o Arduino Mega 2560 e o ESP32 do projeto SmartBird.

O objetivo é estabelecer um padrão para envio de comandos, recebimento de respostas, transmissão de dados dos sensores e comunicação de erros, permitindo que os dois dispositivos sejam desenvolvidos separadamente e integrados posteriormente.

Este protocolo é uma especificação inicial. Os nomes dos campos e comandos deverão ser mantidos consistentes entre os firmwares e atualizados neste documento quando houver alterações aprovadas pela equipe.

2. Arquitetura da comunicação

O SmartBird será composto por dois microcontroladores com responsabilidades distintas.

2.1. Arduino Mega 2560

Responsável pelo controle local do sistema:

* Leitura dos sensores de peso e nível de água.
* Controle do servo motor de alimentação.
* Controle da bomba de água.
* Gerenciamento das rotinas programadas.
* Controle dos avisos sonoros e visuais.
* Validação e execução dos comandos recebidos do ESP32.
* Envio do estado dos módulos e das leituras dos sensores.

O Arduino deverá continuar executando as funções essenciais mesmo quando o ESP32 ou a conexão Wi-Fi estiver indisponível.

2.2. ESP32

Responsável pela comunicação e conectividade:

* Conexão Wi-Fi.
* Hospedagem ou integração com o painel web.
* Recebimento de comandos da interface.
* Conversão das solicitações em mensagens do protocolo.
* Envio e recebimento de mensagens JSON pela UART.
* Encaminhamento de estados, leituras e erros ao painel.

O ESP32 não deverá controlar diretamente os atuadores ligados ao Arduino. Os comandos deverão ser validados pelo Arduino antes de sua execução.

2.3. Fluxo geral

Painel Web
    |
    | HTTP / interface web
    v
ESP32
    |
    | UART + JSON
    v
Arduino Mega 2560
    |
    +-- Sensor de peso
    +-- Sensor de nível de água
    +-- Servo motor
    +-- Bomba de água
    +-- RTC
    +-- Display e avisos

⸻

3. Configuração da comunicação serial

A comunicação entre os dispositivos será realizada por UART.

Parâmetro	Configuração proposta
Protocolo físico	UART TTL
Velocidade	115200 baud
Bits de dados	8
Paridade	Nenhuma
Bits de parada	1
Formato	JSON
Delimitador de mensagem	\n
Codificação	UTF-8, preferencialmente sem caracteres acentuados nas chaves e nos identificadores

A velocidade de 115200 baud é uma configuração inicial e poderá ser alterada caso os testes indiquem necessidade.

3.1. Conexões físicas

Arduino Mega 2560	ESP32	Finalidade
TX1 — pino 18	RX configurado no ESP32	Envio Mega → ESP32
RX1 — pino 19	TX configurado no ESP32	Envio ESP32 → Mega
GND	GND	Referência elétrica comum

Atenção elétrica: o Arduino Mega utiliza lógica de 5 V e o ESP32 utiliza lógica de 3,3 V. O sinal TX1 do Mega deverá passar por um conversor de nível lógico adequado antes de chegar ao RX do ESP32. Não conectar diretamente um sinal de 5 V à entrada do ESP32.

A UART1 do Mega será usada para o ESP32, preservando a UART principal (Serial) para depuração pelo computador.

⸻

4. Regras gerais do formato JSON

Cada mensagem deverá ocupar uma única linha, terminada por \n.

Exemplo:

{"v":"1.0","type":"command","id":"cmd-001","source":"esp32","action":"get_status"}

4.1. Campos padronizados

Campo	Tipo	Obrigatório	Descrição
v	String	Sim	Versão do protocolo
type	String	Sim	Categoria da mensagem
id	String	Conforme o tipo	Identificador único da mensagem ou solicitação
source	String	Sim	Dispositivo que originou a mensagem
action	String	Em comandos	Ação solicitada
data	Objeto	Conforme o tipo	Dados associados à mensagem
status	String	Em respostas de execução	Estado da operação
error	Objeto	Em mensagens de erro	Informações sobre a falha

Os campos específicos de cada tipo de mensagem serão detalhados nas próximas seções. Campos não definidos neste documento não deverão ser utilizados sem alinhamento entre os responsáveis pelos firmwares.

4.2. Identificadores

O campo id deverá identificar a mensagem ou solicitação.

Para comandos, o ESP32 deverá gerar um identificador único, que será preservado nas respostas correspondentes. Isso permitirá associar a resposta ao comando original.

Exemplo: cmd-001, cmd-002.

Esses identificadores são exemplos. A implementação poderá usar uma estratégia própria de geração, desde que não gere conflitos entre solicitações pendentes.

4.3. Separação das mensagens

Cada mensagem deverá ser enviada como JSON válido, seguida de uma quebra de linha.

Exemplo de duas mensagens consecutivas:

{"v":"1.0","type":"command","id":"cmd-001","source":"esp32","action":"get_status"}
{"v":"1.0","type":"command","id":"cmd-002","source":"esp32","action":"get_telemetry"}

O receptor deverá acumular os bytes recebidos até encontrar \n, interpretar a linha completa e só então processar o JSON.

Não se deve presumir que uma chamada de leitura serial corresponderá a uma mensagem JSON completa. Uma mensagem pode chegar em várias leituras ou várias mensagens podem estar disponíveis na mesma leitura.

⸻

5. Tipos de mensagem

Serão utilizados os seguintes tipos iniciais:

Tipo	Finalidade
command	Solicitar uma ação ao dispositivo receptor
ack	Confirmar o recebimento e a aceitação inicial de uma solicitação
response	Informar o resultado de uma solicitação
telemetry	Transmitir leituras e estados dos módulos
event	Informar uma ocorrência relevante do sistema
error	Informar uma falha de comunicação ou processamento

A confirmação ack não significa necessariamente que a operação física foi concluída. A conclusão deverá ser comunicada por uma response ou por um evento definido para a operação.

⸻

6. Comandos disponíveis

Os comandos a seguir constituem a proposta inicial do protocolo.

Comando (action)	Finalidade	Destino
get_status	Consultar o estado geral do sistema	Arduino
get_telemetry	Solicitar as leituras disponíveis	Arduino
feed_manual	Solicitar alimentação manual	Arduino
water_manual	Solicitar reposição manual de água	Arduino
ack_cleaning	Confirmar a realização da limpeza	Arduino
set_schedule	Configurar horários de funcionamento	Arduino, após definição dos parâmetros

Os comandos de acionamento deverão ser tratados como solicitações, não como ordens incondicionais. O Arduino poderá recusá-los se houver uma condição insegura, uma operação em andamento ou uma falha relevante.

Os parâmetros de horário, quantidade de alimento e nível de água ainda precisam ser especificados.

⸻

7. Exemplos de mensagens

7.1. Consultar o estado geral

ESP32 → Arduino

{
  "v": "1.0",
  "type": "command",
  "id": "cmd-001",
  "source": "esp32",
  "action": "get_status"
}

Arduino → ESP32 — confirmação de recebimento

{
  "v": "1.0",
  "type": "ack",
  "id": "cmd-001",
  "source": "arduino",
  "status": "accepted"
}

Arduino → ESP32 — resposta

{
  "v": "1.0",
  "type": "response",
  "id": "cmd-001",
  "source": "arduino",
  "status": "success",
  "data": {
    "feeding": "idle",
    "watering": "idle",
    "rtc": "available",
    "system": "operational"
  }
}

Os estados são ilustrativos. Os valores definitivos deverão ser padronizados na implementação.

7.2. Solicitar alimentação manual

ESP32 → Arduino

{
  "v": "1.0",
  "type": "command",
  "id": "cmd-002",
  "source": "esp32",
  "action": "feed_manual",
  "data": {}
}

Arduino → ESP32 — solicitação aceita

{
  "v": "1.0",
  "type": "ack",
  "id": "cmd-002",
  "source": "arduino",
  "status": "accepted"
}

Arduino → ESP32 — operação concluída

{
  "v": "1.0",
  "type": "response",
  "id": "cmd-002",
  "source": "arduino",
  "status": "success",
  "data": {
    "module": "feeding",
    "result": "completed"
  }
}

Se a alimentação for recusada, o Arduino deverá responder com status: "rejected" e um código de motivo.

A operação física deverá possuir limites de tempo e critérios de interrupção. O Arduino não deverá manter o servo acionado indefinidamente caso o sensor de peso falhe.

7.3. Solicitar reposição de água

ESP32 → Arduino

{
  "v": "1.0",
  "type": "command",
  "id": "cmd-003",
  "source": "esp32",
  "action": "water_manual",
  "data": {}
}

Arduino → ESP32 — operação concluída

{
  "v": "1.0",
  "type": "response",
  "id": "cmd-003",
  "source": "arduino",
  "status": "success",
  "data": {
    "module": "watering",
    "result": "completed"
  }
}

O exemplo representa uma operação bem-sucedida. A implementação deverá verificar as condições de segurança antes de acionar a bomba e interrompê-la caso o nível máximo seja atingido, o tempo limite seja excedido ou ocorra uma falha.

7.4. Enviar telemetria

Arduino → ESP32

{
  "v": "1.0",
  "type": "telemetry",
  "source": "arduino",
  "data": {
    "feeding": {
      "food_weight_g": 85.0,
      "servo_state": "idle"
    },
    "watering": {
      "water_level_percent": 72,
      "pump_state": false
    },
    "maintenance": {
      "cleaning_due": false
    }
  }
}

Os valores de peso e nível são fictícios, usados somente para demonstrar o formato.

A telemetria poderá ser enviada periodicamente ou quando ocorrer uma mudança relevante. O intervalo definitivo será estabelecido durante os testes.

Os nomes dos campos e suas unidades deverão permanecer consistentes. Se a medição de peso não estiver disponível, o sistema deverá informar isso explicitamente, em vez de apresentar um valor fictício como se fosse uma leitura real.

7.5. Confirmar a limpeza

ESP32 → Arduino

{
  "v": "1.0",
  "type": "command",
  "id": "cmd-004",
  "source": "esp32",
  "action": "ack_cleaning",
  "data": {}
}

Arduino → ESP32

{
  "v": "1.0",
  "type": "response",
  "id": "cmd-004",
  "source": "arduino",
  "status": "success",
  "data": {
    "cleaning_due": false,
    "maintenance_timer_reset": true
  }
}

A confirmação deverá reiniciar o intervalo do lembrete conforme as regras de manutenção definidas pelo projeto. Ela não deverá iniciar automaticamente a alimentação ou a hidratação.

7.6. Informar uma falha

Arduino → ESP32

{
  "v": "1.0",
  "type": "error",
  "id": "cmd-003",
  "source": "arduino",
  "error": {
    "code": "WATER_SENSOR_FAILURE",
    "message": "Water level could not be read"
  }
}

Códigos iniciais sugeridos:

Código	Significado
INVALID_JSON	Mensagem JSON inválida
UNKNOWN_COMMAND	Comando não reconhecido
INVALID_PARAMETER	Parâmetro inválido
BUSY	Módulo ocupado
SAFETY_BLOCK	Operação impedida por segurança
WEIGHT_SENSOR_FAILURE	Falha no sensor de peso
WATER_SENSOR_FAILURE	Falha no sensor de água
ACTUATOR_TIMEOUT	Atuador excedeu o tempo máximo
RTC_FAILURE	Falha no relógio de tempo real
UNSUPPORTED_VERSION	Versão de protocolo incompatível

Os códigos deverão ser usados de forma consistente. A mensagem textual serve para diagnóstico; a lógica do sistema deverá depender principalmente do código.

⸻

8. Estados das operações

Para evitar ambiguidades, os módulos deverão usar estados padronizados.

Alimentação

* idle: nenhuma operação em andamento.
* running: mecanismo em operação.
* completed: operação concluída.
* blocked: operação impedida por uma condição de segurança.
* error: falha detectada.

Hidratação

* idle: nenhuma operação em andamento.
* running: bomba em operação.
* completed: reposição concluída.
* blocked: operação impedida por uma condição de segurança.
* error: falha detectada.

O ack informa que o comando foi recebido e aceito inicialmente. O estado do módulo informa a situação da operação física. Esses conceitos não deverão ser confundidos.

⸻

9. Regras de segurança e validação

1. O Arduino deverá validar todos os comandos recebidos antes de executar qualquer ação.
2. Comandos desconhecidos ou mensagens inválidas deverão ser rejeitados.
3. Operações físicas deverão ter limites de tempo e condições explícitas de interrupção.
4. A bomba não deverá ser acionada se houver falha de leitura que impeça verificar o nível de água com segurança.
5. O servo não deverá permanecer acionado indefinidamente em caso de falha do sensor de peso.
6. A perda de comunicação com o ESP32 não deverá interromper as rotinas locais essenciais do Arduino.
7. O sistema deverá distinguir falhas de comunicação de falhas dos sensores e atuadores.
8. Comandos repetidos com o mesmo identificador não deverão iniciar a mesma operação física novamente. O tratamento de duplicatas deverá ser definido na implementação.
9. O ESP32 deverá informar ao painel quando não conseguir se comunicar com o Arduino.
10. A interface web não deverá apresentar uma operação como concluída antes de receber a confirmação correspondente.
11. O funcionamento automático e os comandos manuais deverão respeitar as mesmas condições de segurança.
12. Nenhuma operação deverá depender exclusivamente da disponibilidade da internet.

⸻

10. Tratamento de erros de comunicação

Quando uma mensagem JSON não puder ser interpretada, o receptor deverá descartar a mensagem inválida e continuar procurando o próximo delimitador \n.

O firmware deverá impor um tamanho máximo de mensagem para evitar consumo excessivo de memória. O limite definitivo deverá ser escolhido de acordo com os recursos disponíveis e os tamanhos reais das mensagens.

Se uma mensagem exceder o limite, o receptor deverá descartar o conteúdo até o próximo delimitador e registrar o erro, sem tentar executar o comando parcialmente recebido.

Para comandos que aguardam resposta, o ESP32 deverá implementar um tempo limite. Se não houver resposta, poderá tentar recuperar a comunicação ou consultar novamente o estado do sistema. Não deverá repetir automaticamente um comando de acionamento sem verificar o resultado anterior.

⸻

11. Configuração de horários

A configuração de horários será definida pelo comando set_schedule, mas os parâmetros ainda precisam ser aprovados pela equipe.

Exemplo de estrutura proposta:

{
  "v": "1.0",
  "type": "command",
  "id": "cmd-005",
  "source": "esp32",
  "action": "set_schedule",
  "data": {
    "feeding": {
      "enabled": true,
      "times": ["08:00", "18:00"]
    },
    "watering": {
      "enabled": true,
      "times": ["08:00", "18:00"]
    }
  }
}

Os horários acima são apenas ilustrativos e não representam uma recomendação de alimentação ou hidratação para uma espécie específica.

Antes da implementação, a equipe deverá definir se a rotina será acionada por horário, por condição dos sensores ou pela combinação dos dois. Também será necessário estabelecer como os horários serão armazenados e recuperados após uma reinicialização.

⸻

12. Organização sugerida do software

Uma possível divisão do firmware:

Arduino Mega

* communication: leitura serial, separação das mensagens e envio de respostas.
* protocol: validação das mensagens e despacho dos comandos.
* feeding: controle da alimentação.
* watering: controle da hidratação.
* maintenance: gerenciamento dos lembretes.
* sensors: leitura e validação dos sensores.
* scheduler: gerenciamento das rotinas programadas.

ESP32

* serial_client: comunicação UART com o Mega.
* protocol: criação e interpretação das mensagens JSON.
* wifi_manager: gerenciamento da conexão Wi-Fi.
* web_server: interface e endpoints do painel.
* state_manager: manutenção do último estado conhecido.
* telemetry: encaminhamento das leituras para a interface.

A estrutura é uma sugestão e poderá ser adaptada à organização do repositório.

⸻

13. Critérios de validação

Antes de considerar o protocolo implementado, a equipe deverá verificar:

* [ ]	Os dispositivos conseguem trocar mensagens na velocidade definida.
* [ ]	As mensagens JSON válidas são interpretadas corretamente.
* [ ]	Mensagens inválidas não provocam acionamentos.
* [ ]	Os comandos são associados às respostas por seus identificadores.
* [ ]	Os estados dos módulos são transmitidos corretamente.
* [ ]	Falhas de sensores e atuadores são comunicadas.
* [ ]	A perda de conexão com o ESP32 não interrompe as rotinas locais.
* [ ]	Comandos duplicados não provocam acionamentos repetidos.
* [ ]	Os limites de tempo e as condições de segurança são respeitados.
* [ ]	O painel apresenta estados reais e distingue falhas de dados indisponíveis.

⸻

14. Decisões pendentes

Os seguintes pontos deverão ser definidos antes de considerar o protocolo estável:

* [ ]	Biblioteca JSON e limites de memória.
* [ ]	Tamanho máximo de mensagem.
* [ ]	Intervalo de envio da telemetria.
* [ ]	Estados e códigos de erro definitivos.
* [ ]	Parâmetros do comando set_schedule.
* [ ]	Estratégia de recuperação após perda de comunicação.
* [ ]	Política de repetição de comandos e tratamento de duplicatas.
* [ ]	Versão final dos nomes e tipos dos campos.
* [ ]	Método de autenticação e segurança do acesso remoto.
* [ ]	Necessidade de armazenamento persistente do histórico.

⸻

15. Controle de versões

Versão	Alterações
0.1.0	Estrutura inicial, tipos de mensagem, comandos propostos, exemplos JSON e regras de validação
1.0.0	A ser definida após validação e testes de integração

Regra da equipe: toda alteração incompatível com o protocolo deverá ser discutida e documentada antes de modificar os firmwares. O objetivo é garantir que o Arduino e o ESP32 continuem utilizando o mesmo contrato de comunicação.
