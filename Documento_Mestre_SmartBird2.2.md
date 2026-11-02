# **SMARTBIRD**

# **Documento Mestre do Projeto — Versão 2.2**

Arquitetura, funcionamento, requisitos, desenvolvimento e integração do sistema

| **Campo** | **Especificação** | 
| ----- | ----- | 
| **Projeto acadêmico** | SmartBird | 
| **Área** | Automação, sistemas embarcados, programação e monitoramento | 
| **Plataforma principal proposta** | Arduino Mega 2560 | 
| **Plataforma complementar proposta** | ESP32 | 
| **Contexto** | Projeto universitário desenvolvido em equipe | 
| **Status** | Planejamento e estruturação técnica; componentes, montagem e implementação dependem de validações da equipe | 

## **1. Visão geral do SmartBird**

O SmartBird é um projeto de automação voltado à alimentação, hidratação e ao monitoramento de aves domésticas. A proposta é desenvolver um sistema integrado capaz de acompanhar condições importantes para a rotina da ave, executar determinadas tarefas automaticamente e informar ao responsável quando houver necessidade de intervenção.

A ideia central é reduzir a dependência de verificações manuais constantes, tornando o manejo mais organizado e previsível. Para isso, o sistema combina sensores, atuadores, controle de tempo, interface física e, como expansão prevista na arquitetura, uma interface digital conectada a uma placa ESP32.

O projeto não deve ser entendido como um simples alimentador automático. Seu diferencial está na integração de diferentes funções: monitoramento dos reservatórios, controle da liberação de alimento, controle da água, acompanhamento de horários, alertas de manutenção e possibilidade de configuração pelo usuário.

O funcionamento deve ser organizado em módulos independentes, mas capazes de trocar informações entre si. Dessa forma, cada parte pode ser desenvolvida e testada separadamente antes da integração definitiva.

O sistema também deve considerar situações em que algum sensor falhe, um atuador não responda ou a comunicação entre as placas seja interrompida. A automação precisa agir de maneira previsível nessas condições, sem interpretar uma leitura inválida como se fosse uma medição normal.

### **1.1. Objetivos gerais**

* Automatizar etapas da alimentação e da hidratação da ave.
* Monitorar os níveis de conteúdo nos reservatórios.
* Permitir o acionamento de determinadas funções por horário e por comando manual.
* Informar ao usuário o estado geral do sistema.
* Emitir alertas relacionados à manutenção e a condições que exijam atenção.
* Permitir a configuração de parâmetros operacionais.
* Preparar uma arquitetura que possibilite uma interface digital por meio do ESP32.
* Organizar o código para facilitar o trabalho simultâneo da equipe.
* Desenvolver um protótipo universitário funcional, demonstrável e tecnicamente justificável.

### **1.2. Princípios de funcionamento**

O SmartBird deverá seguir alguns princípios fundamentais:

* **Automação com supervisão:** tarefas rotineiras poderão ser executadas automaticamente, mas o responsável continuará podendo acompanhar o estado do sistema e realizar ações manuais autorizadas.
* **Monitoramento contínuo:** os sensores deverão ser consultados periodicamente, sem que a execução de uma tarefa bloqueie todo o restante do programa.
* **Separação de responsabilidades:** cada módulo deverá cuidar de uma função específica, como medir os reservatórios, controlar a alimentação ou atualizar a interface.
* **Funcionamento local:** as funções essenciais deverão continuar operando mesmo que a interface digital, a rede Wi-Fi ou a conexão entre placas esteja indisponível, desde que o controlador principal e os componentes locais estejam funcionando.
* **Segurança:** leituras inválidas, reservatórios vazios, níveis excessivos, falhas de comunicação e tempos de acionamento ultrapassados deverão ser tratados explicitamente.
* **Configuração controlada:** parâmetros alteráveis pelo usuário deverão ser validados antes de serem aplicados.

## **2. Escopo funcional do projeto**

O SmartBird está organizado em cinco grandes áreas funcionais.

### **2.1. Alimentação**
O sistema deverá acompanhar a condição do alimento e controlar um mecanismo de liberação, utilizando um servo motor como atuador. A lógica poderá considerar horários programados, quantidade de alimento disponível e comandos manuais.
A proposta inicial também considera uma medição de peso na bandeja ou no recipiente de alimentação, utilizando uma célula de carga e um módulo HX711. Essa medição poderá auxiliar na identificação de uma quantidade baixa de alimento e no controle da quantidade liberada.

### **2.2. Hidratação**
O sistema deverá monitorar a água disponível e controlar uma bomba responsável por movimentar ou repor água, conforme a configuração física escolhida.
O controle deverá considerar níveis de água, horários e comandos manuais. A bomba não poderá permanecer ligada indefinidamente: deverá existir um limite de operação e uma condição de parada segura.

### **2.3. Monitoramento dos reservatórios**
Os reservatórios deverão ser monitorados por sensores ultrassônicos instalados acima de seu conteúdo, conforme a proposta mais recente discutida.
Esses sensores permitirão estimar o nível disponível sem depender de contato direto com o alimento ou a água. O sistema precisará conhecer as dimensões de referência e os limites configurados para transformar a distância medida em uma estimativa de nível.

### **2.4. Interface e alertas**
Uma interface física com LCD, botões e indicadores permitirá consultar informações essenciais e realizar ações básicas.
A proposta de expansão inclui uma interface digital por meio do ESP32, oferecendo uma visualização mais completa, configurações e informações de diagnóstico.

### **2.5. Manutenção**
O sistema deverá acompanhar o tempo transcorrido desde a última limpeza confirmada. A proposta inicial estabelece um lembrete aproximadamente a cada dois dias, ou 48 horas.
Quando o intervalo for atingido, o sistema deverá emitir um alerta. A contagem somente deverá ser reiniciada após uma confirmação explícita de que a limpeza foi realizada.

## **3. Arquitetura geral do sistema**

A arquitetura proposta utiliza dois controladores com responsabilidades diferentes: um Arduino Mega 2560 para o controle local e um ESP32 para comunicação e interface digital.
Essa divisão evita concentrar toda a lógica em uma única placa e permite desenvolver a automação principal independentemente da interface conectada.

### **3.1. Arduino Mega 2560 — controlador principal**
O Arduino Mega 2560 é a plataforma principal proposta para o protótipo. Sua função será executar a lógica de controle e coordenar os módulos responsáveis pelos sensores e atuadores.
Entre suas responsabilidades estão:
- Ler os sensores físicos.
- Atualizar o estado dos reservatórios.
- Controlar o servo motor da alimentação.
- Controlar a bomba de hidratação por meio de um circuito de acionamento apropriado.
- Consultar o relógio de tempo real.
- Executar as rotinas de alimentação e hidratação.
- Controlar o LCD, os botões e os indicadores locais.
- Verificar o intervalo de manutenção.
- Receber comandos válidos do ESP32.
- Enviar informações de estado para a interface digital.
- Tratar falhas e condições de segurança.
O Arduino deverá ser capaz de continuar executando as funções locais essenciais sem depender permanentemente do ESP32.

### **3.2. ESP32 — comunicação e interface digital**
O ESP32 é previsto como a plataforma complementar para conectar o sistema a uma interface digital. A arquitetura permite que ele ofereça recursos de comunicação, visualização e configuração sem precisar assumir diretamente o controle de cada atuador.
A interface digital poderá apresentar informações como:
- Estado geral do SmartBird.
- Níveis estimados dos reservatórios.
- Estado da alimentação.
- Estado da hidratação.
- Horários e parâmetros configurados.
- Situação do lembrete de limpeza.
- Alertas e falhas identificadas.
- Informações de diagnóstico.
- Comandos manuais autorizados.
- Parâmetros de calibração e limites operacionais.
A forma final dessa interface consiste em um painel web hospedado e conectado via internet, conforme detalhado nas seções a seguir. A presença do ESP32, por si só, não significa que todas as funcionalidades estejam ativas sem o devido código de ponte implementado.
O ESP32 deverá enviar solicitações ao Arduino e receber as respostas correspondentes. Ele não deverá assumir que uma solicitação foi executada apenas porque conseguiu transmiti-la.

### **3.3. Comunicação entre os controladores**
A proposta é utilizar comunicação serial UART entre o Arduino Mega e o ESP32.
No Arduino Mega, a interface Serial1 utiliza:
- TX1: pino digital 18.
- RX1: pino digital 19.
A ligação deverá cruzar os sinais: TX de uma placa conectado ao RX da outra, e vice-versa. As duas placas também precisam compartilhar o GND de referência.
Existe, entretanto, uma diferença elétrica importante: o Mega trabalha com lógica de 5 V, enquanto os pinos de comunicação do ESP32 utilizam lógica de 3,3 V. O sinal de transmissão do Mega para a entrada RX do ESP32 deverá passar por uma adaptação de nível apropriada. Não se deve conectar diretamente um sinal de 5 V a um pino do ESP32 sem verificar a compatibilidade elétrica.
Como ponto de partida, foi sugerida uma comunicação serial a 115200 baud, com formato 8N1. A velocidade definitiva deverá ser validada durante os testes.
A serial USB principal do Arduino poderá continuar disponível para depuração, enquanto a Serial1 ficará dedicada à comunicação entre as placas.

### **3.4. Arquitetura de Nuvem e Internet das Coisas (IoT)**
A arquitetura de comunicação remota do SmartBird é baseada no protocolo MQTT (Message Queuing Telemetry Transport), utilizando o modelo Publicador-Inscrito para garantir comunicação leve e em tempo real. O fluxo de informações funcionará em quatro etapas independentes: **Painel Web (Celular/PC) ➔ Broker MQTT (Nuvem) ➔ ESP32 (Wi-Fi) ➔ Arduino Mega 2560 (UART)**.
- **Broker MQTT:** O servidor central será o **EMQX Broker** (plano Cloud/Serverless), responsável por receber as mensagens do site e repassá-las instantaneamente ao ESP32, suportando múltiplas conexões sem travamentos.
- **Autenticação:** O acesso ao Broker exige credenciais (usuário e senha configurados no EMQX), garantindo que apenas os tutores autorizados acessem os tópicos de comando da gaiola.

## **4. Funcionamento dos reservatórios e sensores ultrassônicos**

O monitoramento dos reservatórios é uma parte central da arquitetura, porque as informações de nível influenciam decisões de alimentação, hidratação e apresentação de alertas.
A proposta mais recente considera sensores ultrassônicos instalados na parte superior dos reservatórios, apontados para a superfície do conteúdo.

### **4.1. Princípio de medição**
O sensor ultrassônico emite um pulso e mede o tempo necessário para receber seu retorno. Com esse tempo, o sistema estima a distância entre o sensor e a superfície detectada.
Em termos práticos:
- Quando o conteúdo sobe, a distância medida tende a diminuir.
- Quando o conteúdo baixa, a distância medida tende a aumentar.
A medição direta é uma distância, não uma porcentagem. Para apresentar um nível compreensível, o programa deverá utilizar medidas de referência e parâmetros de calibração.

### **4.2. Conversão da distância em nível**
O sistema deverá conhecer a distância de referência entre o sensor e o ponto de nível zero do reservatório, além da altura útil de conteúdo.
Considere:
- distanciaMedida: distância atual entre o sensor e a superfície.
- distanciaReferencia: distância entre o sensor e a referência de nível zero.
- alturaUtil: altura útil disponível para o conteúdo.
- alturaConteudo: altura estimada do conteúdo.
- percentualNivel: estimativa percentual do nível.

A lógica conceitual será:
`alturaConteudo = distanciaReferencia - distanciaMedida`
Depois, a estimativa percentual poderá ser calculada por:
`percentualNivel = (alturaConteudo / alturaUtil) * 100`

O resultado deverá ser limitado ao intervalo de 0% a 100% quando a medição for válida. Leituras fora do comportamento esperado deverão ser investigadas e não simplesmente interpretadas como níveis normais.
Exemplo ilustrativo: se a altura útil for de 20 cm e o conteúdo ocupar aproximadamente 10 cm dessa altura, o nível estimado será de 50%.

### **4.3. Parâmetros de calibração**
Cada reservatório poderá precisar de parâmetros próprios, porque os sensores podem ser instalados em alturas diferentes e os recipientes podem possuir dimensões distintas.
Os parâmetros previstos incluem distância de referência, altura útil, limites para considerar nível baixo/adequado, margens de tolerância (histerese) e identificação do reservatório.

### **4.4. Estados dos reservatórios**
A lógica poderá classificar os reservatórios em estados como: Normal, Baixo, Crítico, Cheio, Leitura inválida, Indisponível.

### **4.5. Frequência de leitura e estabilidade**
As leituras deverão ocorrer periodicamente, sem bloquear o restante do programa. Será necessário filtrar as medições e rejeitar valores claramente inválidos.

## **5. Módulo de alimentação**

O módulo de alimentação deverá controlar a liberação de alimento e acompanhar as condições necessárias para realizar essa operação.
A proposta inicial considera um mecanismo acionado por servo motor e uma medição de peso utilizando célula de carga e módulo HX711. 

### **5.1. Componentes previstos**
- Servo motor para controlar o mecanismo de liberação.
- Célula de carga, caso a medição de peso seja mantida.
- Módulo HX711 para leitura da célula de carga.
- Sensor ultrassônico do reservatório de alimento.
- Arduino Mega como controlador.

### **5.2. Alimentação automática por horário**
A rotina deverá consultar o horário fornecido pelo relógio de tempo real e verificar se existe uma alimentação programada para aquele momento, validando as condições necessárias antes de liberar o alimento.

### **5.3. Alimentação com base no peso**
A medição de peso poderá auxiliar a identificar quando a bandeja estiver com pouco alimento.

### **5.4. Alimentação manual**
A interface deverá permitir uma solicitação manual de alimentação, desde que o sistema esteja em condições seguras, obedecendo o intertravamento local.

### **5.5. Limites de operação**
O módulo deverá possuir limites de segurança contra travamentos mecânicos, níveis excessivos e tempo máximo de acionamento.

## **6. Módulo de hidratação**

O módulo de hidratação deverá acompanhar a disponibilidade de água e controlar a bomba responsável pela reposição.

### **6.1. Componentes previstos**
- Bomba de água.
- Circuito de acionamento apropriado (driver/módulo relé).
- Sensor de nível.
- Arduino Mega.

### **6.2. Hidratação automática e manual**
A lógica considerará acionamentos por horário programado, nível de água baixo ou comando manual. A bomba deverá ser desligada quando a condição de parada for atingida.

### **6.3. Limites e falhas**
A bomba deverá possuir um tempo máximo de funcionamento por operação (*timeout*), crucial em caso de falha dos sensores.

## **7. Relógio de tempo real e agendamento**

O RTC DS3231 fornece a referência temporal, operando independentemente da rede Wi-Fi.

## **8. Módulo de manutenção e limpeza**

O SmartBird acompanhará a necessidade de limpeza periódica do sistema, emitindo alertas locais e na interface web. A contagem de tempo exigirá confirmação explícita de limpeza concluída pelo usuário.

## **9. Interface física: LCD, botões, LEDs e buzzer**

A interface física garante acesso essencial e autônomo ao sistema, priorizando mensagens curtas operacionais, interações com botões de *debounce* e avisos controlados sonoros.

## **10. Interface Digital: Broker MQTT e Painel Web**

A interface digital funcionará como um **Painel Web Responsivo**, hospedado na nuvem, desvinculando o usuário da necessidade de estar na mesma rede Wi-Fi que o protótipo. 

### **10.1. Estrutura do Frontend Web**
Interface construída em HTML5, CSS3 e JavaScript. Exibirá o estado do sistema e alertas em tempo real.

### **10.2. Conexão Web-Nuvem (Assincronia)**
O JavaScript utilizará a biblioteca **Paho MQTT** para se conectar via WebSockets ao EMQX Broker. A interface adotará o princípio de execução assíncrona, onde os feedbacks visuais dependem da confirmação de recepção do JSON processado localmente.

### **10.4. Validação e armazenamento**
O ESP32 funciona como canal transparente; as alterações submetidas pela interface web serão transmitidas ao Arduino Mega para consolidação e aplicação nas rotinas de segurança local.

## **11. Organização do código e modularização**

Para permitir o desenvolvimento paralelo por múltiplos integrantes, o código-fonte deve ser distribuído em módulos funcionais isolados com contratos claros.

### **11.1. SmartBird_Arduino.ino**
Ponto de entrada do firmware do Arduino Mega. Contém as rotinas `setup()` e `loop()` não bloqueantes.

### **11.10. Comunicacao.h/.cpp (Arduino Mega)**
Gerencia a camada física e buffers da UART Serial1 do Arduino Mega. Implementa a recepção baseada no acúmulo de bytes (pooling) até a identificação do delimitador de quebra de linha (`\n`), garantindo que apenas pacotes inteiros sejam repassados à camada de protocolo.

### **11.11. Protocolo.h/.cpp (Arduino Mega)**
Responsável por utilizar a biblioteca `ArduinoJson` no Arduino Mega para processar integralmente o JSON completo recebido do ESP32 via UART.
Deverá:
- Interpretar as mensagens JSON recebidas (extraindo `id`, `type`, `action`).
- Validar a conformidade das chaves e parâmetros de acordo com o Documento de Protocolo.
- Despachar as intenções para os módulos mecânicos (Alimentação, Hidratação).
- Estruturar os objetos JSON completos de telemetria e resposta (`ack`, `response`, `error`) e escrevê-los na UART de volta para a nuvem.

### **11.12. Arquivos auxiliares e bibliotecas (Arduino Mega)**
Dependências externas recomendadas para o Arduino Mega:
- `ArduinoJson`: serialização e desserialização dos pacotes do protocolo.
- `RTClib`: abstração e controle do módulo DS3231.
- `LiquidCrystal_I2C` ou `LiquidCrystal`: interface com display.
- `Servo`: controle de pulso PWM.
- `HX711`: comunicação com célula de carga.

### **11.13. ESP32: Comunicação Wi-Fi e Ponte MQTT (UART Bridge)**
O firmware do ESP32 terá responsabilidades estritas de rede e conectividade. Seu projeto na IDE Arduino é dividido nos seguintes blocos:
- **WiFi_Manager:** gerencia a conexão e reconexão automática com a rede Wi-Fi local de 2.4 GHz.
- **MQTT_Client:** utiliza a biblioteca `PubSubClient` para manter a conexão TLS segura com o EMQX Broker.
- **UART_Bridge (Ponte Transparente):** O ESP32 atua exclusivamente como ponte bidirecional, **sem efetuar o parsing local do JSON**. 
    1. Mensagens JSON brutos recebidas dos tópicos MQTT da nuvem são retransmitidas integralmente para a porta Serial conectada ao Arduino Mega (via `Serial.println()`).
    2. Mensagens JSON de respostas e telemetria recebidas do Mega via UART são diretamente publicadas nos tópicos MQTT correspondentes para chegar ao Painel Web.

### **11.14. Painel Web: Organização do Código**
O repositório web hospedará o `app.js` contendo a biblioteca **Paho MQTT**, a estruturação dos comandos em JSON e uma função geradora de hashes para popular o campo `id` de cada comando.

---

## **12. Comunicação por JSON entre Arduino e ESP32**

A comunicação entre controladores obedece ao padrão do documento complementar **SmartBird — Protocolo de Comunicação**. 
O modelo arquitetural definido é o de passagem transparente pelo gateway IoT: as mensagens trafegam completas (JSON end-to-end) entre o Painel Web e o Arduino Mega, e o ESP32 não quebra o formato do protocolo.

### **12.1. Exemplo de comando e Fluxo de Dados**
Formato de solicitação originada pelo Painel Web:
```json
{"v":"1.0","type":"command","id":"cmd-001","source":"web","action":"feed_manual"}
```
- **Painel Web:** Cria o JSON completo e publica no MQTT.
- **ESP32:** Intercepta o pacote pelo MQTT e imprime a `String` intacta na UART1 (Pinos TX/RX).
- **Arduino Mega:** Recebe o texto, utiliza o `ArduinoJson` para extrair as variáveis, valida a segurança mecânica, aciona o motor e devolve um novo objeto JSON completo na UART confirmando o sucesso da ação.
- **ESP32:** Lê a resposta do Mega pela UART e publica de volta no tópico MQTT para o Painel Web.

### **12.2. Tipos de mensagem e tópicos MQTT**
O protocolo engloba os formatos `command`, `ack`, `response`, `telemetry`, `event` e `error`. O tráfego na nuvem será segmentado em pelo menos dois canais principais:
- **Tópico de Comandos (`smartbird/gaiola01/comandos`):** o Painel Web publica, o ESP32 subscreve e repassa ao Mega.
- **Tópico de Telemetria (`smartbird/gaiola01/status`):** o Arduino Mega formata, o ESP32 recebe via serial e publica para consumo do Painel Web.

---

## **13. Fluxo integrado de funcionamento**

O fluxo de processamento divide-se em:
[ Inicialização ] ➔ [ Leitura dos Sensores ] ➔ [ Avaliação de Regras ] ➔ [ Atuação ] ➔ [ Atualização das Interfaces ]

O fluxo obedece princípios estritos de checagem, ondem a comunicação recebida pela UART passa pela conversão do módulo de Protocolo antes de influenciar as Atuações.

## **14. Execução não bloqueante**

O código-fonte do microcontrolador não utiliza instruções de congelamento de fluxo como `delay()`. Toda temporização é implementada por meio de comparadores baseados na função `millis()`.

## **15. Alimentação elétrica e montagem física**

A separação dos circuitos de controle (5V lógicos) e circuitos de potência (relés/drivers para motores e bombas) é mandatória.

### **15.5. Conectividade Wi-Fi e Estratégia de Apresentação**
- **Ambiente de Demonstração (Banca Avaliadora):** Para garantir estabilidade técnica, o ESP32 será configurado previamente para conectar-se exclusivamente a uma rede de ponto de acesso portátil (*Hotspot 4G/5G*).
- **Adequação de Tensão de Hardware:** É obrigatório o emprego de um **Módulo Conversor de Nível Lógico Bidirecional (3.3V – 5V)** entre a porta Serial1 do Arduino Mega (5V) e os pinos de recepção UART do ESP32 (3.3V).

---

## **16. Organização do trabalho em equipe e Git**

O desenvolvimento adotará controle de versão distribuído via GitHub.

### **16.2. Branches sugeridas**
A divisão proposta permite paralelismo no desenvolvimento, separando inclusive a implementação da comunicação UART/JSON (`feature/comunicacao-json`) do backend IoT (`feature/painel-web-mqtt`).

### **16.4. Contratos entre módulos**
A adoção formal do `protocolo_comunicacao_v1.md` garante que a representação de dados seja uniforme, não causando interrupções durante o merge das *feature branches*.

---

## **17. Estratégia de desenvolvimento e testes**

O desenvolvimento avança gradualmente: Definição Técnica ➔ Testes de Bancada de Componentes ➔ Desenvolvimento Modular (simulando entradas) ➔ Integração do Firmware Local ➔ **Integração com ESP32 (Ponte UART e Nuvem)** ➔ Validação Integrada Física.

## **18. Tratamento de falhas e comportamento seguro**

A estabilidade fundamenta-se no funcionamento local não-dependente da rede. Se o ESP32 perder o contato com o EMQX Broker, o Arduino Mega continuará cumprindo autonomamente os ciclos temporais de alimentação e alerta. Leituras anormais (out of bounds) são ignoradas e geram logs de falha na interface, impedindo o funcionamento do atuador respectivo.

---

## **19. Decisões pendentes e pontos que exigem validação**

Os parâmetros físicos (tipo da bomba, dimensões, pino exato dos sensores) devem ser confirmados com a bancada prática.

## **20. Organização dos Documentos Técnicos**

A documentação do SmartBird consiste em partes complementares:
20.1. Documento Mestre do Projeto (Este arquivo)
20.2. **SmartBird – Protocolo de Comunicação** (Define os schemas JSON em detalhes)
20.3. Documento de Ligações Elétricas
20.4. Documento de Mapa de Pinos e Alimentação

## **21. Apresentação acadêmica e demonstração do protótipo**

A equipe deve focar na justificativa de engenharia de ter a separação de responsabilidades (Arduino para Real-Time local e ESP32 como Gateway MQTT transparente), destacando a ausência de travamentos (não-bloqueante) e uso do protocolo JSON para interligação independente de linguagens ou nuvens.

## **22. Possibilidades de evolução**

O sistema suporta Aplicativo Mobile nativo, Dashboards avançados, Controle Térmico (Sensores I2C extras no Mega) e Câmera de Monitoramento sem precisar reescrever o código base das arquiteturas atuais.

## **23. Conclusão**

A estrutura modular do código-fonte, a padronização das mensagens em formato JSON com envio ponta a ponta (Web -> ESP32 -> Mega), o isolamento elétrico das linhas de potência e o emprego de técnicas de programação não bloqueante formam a base para o desenvolvimento progressivo da equipe. A arquitetura em ponte transparente (UART Bridge) desonera o módulo Wi-Fi e eleva a confiabilidade central do Arduino Mega 2560 como processador principal.
