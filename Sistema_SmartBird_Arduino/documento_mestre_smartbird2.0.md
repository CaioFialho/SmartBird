# **SMARTBIRD**

# **Documento Mestre do Projeto — Versão 2.0**

Arquitetura, funcionamento, requisitos, desenvolvimento e integração do sistema

| **Campo** | **Especificação** |
| :--- | :--- |
| **Projeto acadêmico** | SmartBird |
| **Área** | Automação, sistemas embarcados, programação e monitoramento |
| **Plataforma principal proposta** | Arduino Mega 2560 |
| **Plataforma complementar proposta** | ESP32 |
| **Contexto** | Projeto universitário desenvolvido em equipe |
| **Status** | Planejamento e estruturação técnica; componentes, montagem e implementação dependem de validações da equipe |

---

## **1. Visão geral do SmartBird**

O SmartBird é um projeto de automação voltado à alimentação, hidratação e ao monitoramento de aves domésticas. A proposta é desenvolver um sistema integrado capaz de acompanhar condições importantes para a rotina da ave, executar determinadas tarefas automaticamente e informar ao responsável quando houver necessidade de intervenção.

A ideia central é reduzir a dependência de verificações manuais constantes, tornando o manejo mais organizado e previsível. Para isso, o sistema combina sensores, atuadores, controle de tempo, interface física e, como expansão prevista na arquitetura, uma interface digital conectada a uma placa ESP32.

O projeto não deve ser entendido como um simples alimentador automático. Seu diferencial está na integração de diferentes funções: monitoramento dos reservatórios, controle da liberação de alimento, controle da água, acompanhamento de horários, alertas de manutenção e possibilidade de configuração pelo usuário.

O funcionamento deve ser organizado em módulos independentes, mas capazes de trocar informações entre si. Dessa forma, cada parte pode ser desenvolvida e testada separadamente antes da integração definitiva.

O sistema também deve considerar situações em que algum sensor falhe, um atuador não responda ou a comunicação entre as placas seja interrompida. A automação precisa agir de maneira previsível nessas condições, sem interpretar uma leitura inválida como se fosse uma medição normal.

### **1.1. Objetivos gerais**

- Automatizar etapas da alimentação e da hidratação da ave.
- Monitorar os níveis de conteúdo nos reservatórios.
- Permitir o acionamento de determinadas funções por horário e por comando manual.
- Informar ao usuário o estado geral do sistema.
- Emitir alertas relacionados à manutenção e a condições que exijam atenção.
- Permitir a configuração de parâmetros operacionais.
- Preparar uma arquitetura que possibilite uma interface digital por meio do ESP32.
- Organizar o código para facilitar o trabalho simultâneo da equipe.
- Desenvolver um protótipo universitário funcional, demonstrável e tecnicamente justificável.

### **1.2. Princípios de funcionamento**

O SmartBird deverá seguir alguns princípios fundamentais:

- **Automação com supervisão:** tarefas rotineiras poderão ser executadas automaticamente, mas o responsável continuará podendo acompanhar o estado do sistema e realizar ações manuais autorizadas.
- **Monitoramento contínuo:** os sensores deverão ser consultados periodicamente, sem que a execução de uma tarefa bloqueie todo o restante do programa.
- **Separação de responsabilidades:** cada módulo deverá cuidar de uma função específica, como medir os reservatórios, controlar a alimentação ou atualizar a interface.
- **Funcionamento local:** as funções essenciais deverão continuar operando mesmo que a interface digital, a rede Wi-Fi ou a conexão entre placas esteja indisponível, desde que o controlador principal e os componentes locais estejam funcionando.
- **Segurança:** leituras inválidas, reservatórios vazios, níveis excessivos, falhas de comunicação e tempos de acionamento ultrapassados deverão ser tratados explicitamente.
- **Configuração controlada:** parâmetros alteráveis pelo usuário deverão ser validados antes de serem aplicados.

---

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

---

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
- **Broker MQTT:** O servidor central será o **HiveMQ Cloud** (plano gratuito), responsável por receber as mensagens do site e repassá-las instantaneamente ao ESP32, suportando múltiplas conexões sem travamentos.
- **Autenticação:** O acesso ao Broker exige credenciais (usuário e senha configurados no HiveMQ), garantindo que apenas os tutores autorizados acessem os tópicos de comando da gaiola.

---

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
Essa estimativa representa a altura ocupada. Ela só representa aproximadamente a porcentagem de volume quando a geometria do reservatório permite essa relação. Reservatórios com formatos irregulares podem exigir uma tabela de calibração ou outro método de conversão.

### **4.3. Parâmetros de calibração**
Cada reservatório poderá precisar de parâmetros próprios, porque os sensores podem ser instalados em alturas diferentes e os recipientes podem possuir dimensões distintas.
Os parâmetros previstos incluem:
- Distância de referência do sensor.
- Altura útil do reservatório.
- Limite para considerar o nível baixo.
- Limite para considerar o nível adequado.
- Limite para considerar o reservatório cheio, quando aplicável.
- Margens de tolerância para evitar oscilações constantes de estado.
- Validade mínima da leitura.
- Identificação do reservatório associado ao sensor.
Esses valores deverão ser configuráveis quando necessário, mas sempre submetidos a validação. O programa deverá verificar se os parâmetros inseridos são compatíveis com as dimensões e os limites definidos.

### **4.4. Estados dos reservatórios**
A lógica poderá classificar os reservatórios em estados como:
- **Normal:** nível dentro da faixa esperada.
- **Baixo:** nível abaixo do limite configurado.
- **Crítico:** nível muito baixo, quando essa classificação for necessária.
- **Cheio:** nível próximo do limite superior configurado.
- **Leitura inválida:** sensor sem retorno confiável ou medição fora das condições esperadas.
- **Indisponível:** sensor ainda não inicializado ou sem atualização válida.
A classificação definitiva deverá ser definida pela equipe, considerando a capacidade dos recipientes e o comportamento real dos sensores. Um sensor que não recebeu retorno não deverá produzir automaticamente uma leitura de 0%, pois ausência de medição e reservatório vazio são condições distintas.

### **4.5. Frequência de leitura e estabilidade**
As leituras deverão ocorrer periodicamente, sem bloquear o restante do programa.
Como sensores ultrassônicos podem apresentar pequenas oscilações, poderá ser necessário filtrar as medições, rejeitar valores claramente inválidos ou considerar mais de uma leitura antes de alterar o estado do reservatório.
Também será importante evitar que o sistema alterne repetidamente entre "normal" e "baixo" quando o nível estiver próximo do limite. Uma margem de histerese poderá ajudar a estabilizar a classificação.

### **4.6. Relação com os outros módulos**
O módulo responsável pelos reservatórios deverá concentrar a leitura ultrassônica, a calibração e a interpretação dos dados. Os módulos de alimentação e hidratação deverão consumir o estado já calculado, sem precisar conhecer os detalhes elétricos ou o algoritmo interno dos sensores.
Essa separação permite trocar um sensor, modificar a calibração ou alterar o método de leitura sem reescrever toda a lógica de alimentação e hidratação.

---

## **5. Módulo de alimentação**

O módulo de alimentação deverá controlar a liberação de alimento e acompanhar as condições necessárias para realizar essa operação.
A proposta inicial considera um mecanismo acionado por servo motor e uma medição de peso utilizando célula de carga e módulo HX711. O monitoramento ultrassônico do reservatório também poderá fornecer informações sobre a disponibilidade geral de alimento.

### **5.1. Componentes previstos**
- Servo motor para controlar o mecanismo de liberação.
- Célula de carga, caso a medição de peso seja mantida.
- Módulo HX711 para leitura da célula de carga.
- Sensor ultrassônico do reservatório de alimento, conforme a arquitetura de monitoramento.
- Arduino Mega como controlador.
- Interface física e digital para exibir o estado e solicitar ações.
O posicionamento da célula de carga precisa ser definido com cuidado. Se ela estiver sob a bandeja, medirá o peso existente nessa bandeja; se estiver sob o reservatório, medirá outra grandeza. Essas medidas não são intercambiáveis, e o software deverá ser desenvolvido de acordo com a montagem real.

### **5.2. Alimentação automática por horário**
A rotina deverá consultar o horário fornecido pelo relógio de tempo real e verificar se existe uma alimentação programada para aquele momento.
Quando o horário for atingido, o sistema deverá avaliar as condições necessárias para liberar alimento. A operação deverá considerar, no mínimo:
1. Se existe uma programação válida.
2. Se o horário corresponde a uma alimentação prevista.
3. Se o mecanismo está disponível.
4. Se o nível do reservatório é válido e suficiente.
5. Se a quantidade de alimento disponível ou o peso da bandeja permitem a operação, conforme a lógica adotada.
6. Se não existe uma falha que impeça o acionamento.
O servo deverá se movimentar de acordo com uma sequência definida, evitando movimentos indefinidos ou acionamentos repetidos a cada ciclo do programa.

### **5.3. Alimentação com base no peso**
A medição de peso poderá auxiliar a identificar quando a bandeja estiver com pouco alimento.
Para isso, a célula de carga deverá passar por calibração, e o programa precisará converter as leituras do HX711 em uma unidade útil, como gramas. O controle deverá distinguir entre:
- Peso dentro da faixa esperada.
- Peso abaixo do limite configurado.
- Peso incompatível com a medição válida.
- Falha ou ausência de leitura.
Caso a lógica utilize um peso-alvo, o sistema poderá liberar alimento em pequenas etapas e verificar novamente a condição após cada operação, respeitando limites de tempo e de acionamento.

### **5.4. Alimentação manual**
A interface deverá permitir uma solicitação manual de alimentação, desde que o sistema esteja em condições seguras.
A solicitação poderá vir dos botões físicos ou da interface digital. Independentemente da origem, deverá passar pela mesma validação central utilizada pela automação. Um comando manual não deverá ignorar limites de segurança, como a indisponibilidade do mecanismo ou uma leitura crítica que impeça a operação.

### **5.5. Limites de operação**
O módulo deverá possuir limites para evitar que o servo permaneça acionado ou que o mecanismo seja movimentado indefinidamente.
Também será necessário definir o comportamento diante de reservatório vazio, leitura ultrassônica inválida, falha na medição de peso ou não resposta do servo. A posição de repouso, os ângulos de abertura e fechamento e o tempo máximo de acionamento deverão ser determinados durante os testes do protótipo.

---

## **6. Módulo de hidratação**

O módulo de hidratação deverá acompanhar a disponibilidade de água e controlar a bomba responsável pela movimentação ou reposição de água.
O projeto prevê a utilização de um sensor de nível, além do monitoramento ultrassônico dos reservatórios.

### **6.1. Componentes previstos**
- Bomba de água.
- Circuito de acionamento apropriado (driver ou módulo de potência compatível).
- Sensor ou sensores de nível definidos para a montagem.
- Sensor ultrassônico do reservatório, conforme a arquitetura final.
- Arduino Mega.
- Relógio de tempo real para as rotinas programadas.
- Interface física e digital.
A bomba não deverá ser alimentada diretamente por um pino do Arduino. O circuito de potência e a fonte deverão ser dimensionados conforme a tensão e a corrente exigidas pelo modelo escolhido.

### **6.2. Hidratação automática**
A lógica de hidratação poderá considerar três tipos de solicitação:
- Horário programado.
- Nível de água abaixo do limite configurado.
- Comando manual do usuário.
Antes de ligar a bomba, o módulo deverá verificar se a leitura do nível é válida e se a operação pode ser executada com segurança. A bomba deverá ser desligada quando a condição de parada for atingida. Se houver um nível-alvo, a leitura deverá ser atualizada durante a operação para impedir que o sistema continue bombeando além do necessário.

### **6.3. Limites e falhas**
A bomba deverá possuir um tempo máximo de funcionamento por operação. Esse limite é especialmente importante quando o sensor não responde ou quando a água não chega ao nível esperado.
O módulo deverá prever situações como: reservatório sem água, nível máximo atingido, sensor inválido, ausência de mudança esperada no nível ou tempo de bombeamento excedido. Se uma leitura essencial for inválida, o comportamento seguro será impedir ou interromper a operação automática.

### **6.4. Comandos manuais**
A interface poderá disponibilizar um comando para solicitar hidratação manual. O comando deverá ser interpretado como uma solicitação, e não como autorização para manter a bomba ligada indefinidamente. A execução continuará sujeita aos limites de segurança do módulo.

---

## **7. Relógio de tempo real e agendamento**

O projeto prevê a utilização do módulo RTC DS3231 para fornecer uma referência de horário estável ao controlador.

### **7.1. Responsabilidades do RTC**
O módulo de tempo deverá permitir:
- Consultar a data e a hora.
- Identificar os horários programados.
- Comparar o horário atual com os horários de operação.
- Calcular intervalos transcorridos.
- Apoiar o controle do lembrete de limpeza.
- Detectar condições em que a leitura do relógio não seja confiável.
O RTC fornece a referência temporal, mas a decisão de executar uma ação pertence ao módulo responsável pela função.

### **7.2. Agendamentos**
Os horários deverão ser armazenados em uma estrutura de configuração compreensível pelo programa. A lógica deverá evitar que um horário programado seja interpretado como um novo evento a cada ciclo do `loop()`, registrando que a ocorrência já foi tratada. Se houver mais de um horário, cada evento deverá ser avaliado de forma independente.

### **7.3. Falhas de horário**
O sistema deverá considerar o comportamento caso o RTC não seja inicializado, apresente dados inválidos ou perca a referência temporal. Nessas condições, as rotinas dependentes de horário não deverão operar com referências desconhecidas. O comportamento após uma reinicialização deve impedir acionamentos duplicados.

---

## **8. Módulo de manutenção e limpeza**

O SmartBird deverá ajudar o responsável a acompanhar a necessidade de limpeza periódica do sistema.

### **8.1. Funcionamento do lembrete**
O sistema registrará o momento da última limpeza confirmada e comparará esse dado com o horário atual. A proposta inicial prevê um intervalo base de aproximadamente 48 horas.
Quando o prazo for atingido, o módulo emitirá um alerta por meio dos recursos locais (LED e buzzer) e na interface digital conectada.

### **8.2. Confirmação da limpeza**
A contagem não será reiniciada apenas pela visualização do alerta; será exigida uma confirmação explícita de limpeza realizada via interface física ou digital. Após o registro, uma nova janela temporal de 48 horas começará a ser contada.

### **8.3. Persistência**
Para assegurar que reinicializações do Arduino ou quedas de energia não apaguem o histórico de limpeza, o registro de confirmação deverá ser armazenado em memória não volátil (persistente), acompanhado de rotinas de validação de consistência de dados.

### **8.4. Estados possíveis**
- Limpeza dentro do prazo.
- Limpeza próxima do vencimento (aviso antecipado opcional).
- Limpeza pendente.
- Limpeza confirmada.
- Registro de tempo inválido ou indisponível.

---

## **9. Interface física: LCD, botões, LEDs e buzzer**

A interface física garante acesso essencial e autônomo ao sistema, operando plenamente mesmo sem conectividade com a internet.

### **9.1. LCD 16x2**
O display LCD exibirá mensagens curtas, priorizando status críticos de funcionamento:

| **Exemplo de Mensagem** | **Significado Operacional** |
| :--- | :--- |
| `SmartBird  10:30` <br> `Agua:OK  Rac:BAIX` | Horário atual e monitoramento resumido dos dois reservatórios |
| `Alimentacao:ativa` <br> `Limpeza pendente` | Execução de rotina mecânica e alerta de manutenção ultrapassada |

A formatação textual evitará caracteres acentuados incompatíveis com a memória de fontes padrão do módulo HD44780.

### **9.2. Botões físicos**
A proposta avalia o uso de três botões de comando com tratamento de repique via software (debounce):
- **Anterior:** retorno ou navegação ascendente nos itens de menu.
- **Próximo:** avanço ou navegação descendente nos itens de menu.
- **Selecionar:** confirmação do parâmetro ou disparo da ação selecionada.

### **9.3. LEDs e buzzer**
- **LEDs:** sinalizam visualmente estados de prontidão, nível crítico e atividade de atuadores.
- **Buzzer:** emite alertas sonoros pontuais e controlados para advertir manutenção pendente ou falhas críticas, sem causar estresse ao animal alojado.

### **9.4. Limites da interface física**
O display LCD é restrito à consulta rápida e ações pontuais. Diagnósticos detalhados, ajustes finos de calibração e parametrizações complexas são delegados à interface digital.

---

## **10. Interface Digital: Broker MQTT e Painel Web**

A interface digital funcionará como um **Painel Web Responsivo**, hospedado na nuvem, desvinculando o usuário da necessidade de estar na mesma rede Wi-Fi que o protótipo. Isso viabiliza o controle remoto e prepara o sistema para expansões como aplicativos móveis.

### **10.1. Estrutura do Frontend Web**
A interface será construída em HTML5, CSS3 (com frameworks como Bootstrap ou Tailwind para responsividade mobile) e JavaScript. Ela exibirá o estado do sistema, alertas de manutenção pendente e níveis de reservatório em tempo real.

### **10.2. Conexão Web-Nuvem (Assincronia)**
O JavaScript utilizará a biblioteca **Paho MQTT** para se conectar via WebSockets ao HiveMQ Cloud. A interface adotará o princípio de execução assíncrona:
1. Ao solicitar uma ação (ex: "Alimentar"), o botão exibirá um indicador de carregamento (status *running*).
2. A tela só apresentará a mensagem de sucesso (*completed*) quando o painel receber, via MQTT, a resposta (*response*) gerada fisicamente pelo Arduino Mega.

### **10.3. Parâmetros configuráveis**
- Horários programados de alimentação e hidratação.
- Dimensões de referência (altura útil e distância zero) dos sensores ultrassônicos.
- Limiares de alerta para nível baixo e reservatório cheio.
- Intervalo configurável para o lembrete de higienização.
- Tempos máximos de acionamento contínuo dos atuadores.

### **10.4. Validação e armazenamento**
Qualquer alteração submetida pela interface web passará por checagem de limites operacionais aceitáveis antes da persistência. O ESP32 funciona como canal de entrada do formulário, transmitindo os dados ao Arduino Mega para consolidação e aplicação nas rotinas de segurança local.

### **10.5. Funcionamento sem conexão**
A interface web é estritamente uma camada de supervisão e conveniência. Se a rede Wi-Fi cair ou o ESP32 for reiniciado, o Arduino Mega continuará cumprindo autonomamente os ciclos de alimentação, controle de nível de água e acionamento dos alertas sonoros e visuais locais.

---

## **11. Organização do código e modularização**

Para permitir o desenvolvimento paralelo por múltiplos integrantes da equipe, o código-fonte deve ser distribuído em módulos funcionais isolados com contratos claros.

### **11.1. SmartBird_Arduino.ino**
Ponto de entrada do firmware do Arduino Mega. Contém as rotinas `setup()` para inicialização dos subsistemas e `loop()` para invocação contínua e não bloqueante das tarefas de cada módulo.

### **11.2. EstadoSmartBird.h**
Arquivo de cabeçalho que consolida as estruturas de dados compartilhadas, enumerações de estado dos reservatórios, flags de segurança, estruturas de telemetria e códigos globais de falha.

### **11.3. ControleSmartBird.h/.cpp**
Camada central de coordenação. Sincroniza a transição entre leituras de sensores, regras de intertravamento de segurança e atualização dos dados repassados às interfaces.

### **11.4. Reservatorios.h/.cpp**
Concentra o acionamento dos sensores ultrassônicos, rotinas de filtragem contra ruídos de leitura, aplicação da calibração dimensional e determinação dos níveis em centímetros e percentual.

### **11.5. Tempo.h/.cpp**
Gerencia a comunicação I²C com o RTC DS3231, validação da integridade do relógio e marcação temporal para eventos de agendamento.

### **11.6. Alimentacao.h/.cpp**
Controla o ciclo de acionamento angular do servo motor, monitoramento da célula de carga (HX711) e validação de segurança para liberação de ração.

### **11.7. Hidratacao.h/.cpp**
Controla o acionamento do circuito de potência da bomba d'água, monitoramento de tempos máximos de funcionamento contínuo (*timeout*) e condição de parada por sensor de nível.

### **11.8. Interface.h/.cpp**
Gerencia a escrita no LCD 16x2, leitura com *debounce* dos botões físicos e ativação dos sinais visuais e sonoros nos pinos de LED e buzzer.

### **11.9. Manutencao.h/.cpp**
Calcula a contagem de tempo regressivo para higienização, dispara alertas locais e digitais após o prazo limite e processa as confirmações de limpeza salvando-as de forma não volátil.

### **11.10. Comunicacao.h/.cpp**
Gerencia a camada física e buffers da UART Serial1 do Arduino Mega, implementando recepção por interrupção ou pooling rápido delimitado por quebra de linha (`\n`).

### **11.11. Protocolo.h/.cpp**
Interpreta strings JSON recebidas através do Comunicacao, valida a conformidade das chaves e formata os objetos JSON de telemetria e resposta para envio ao ESP32.

### **11.12. Arquivos auxiliares e bibliotecas**
Dependências externas recomendadas para o Arduino Mega:
- `ArduinoJson`: serialização e desserialização rápida de pacotes JSON.
- `RTClib`: abstração e controle do módulo DS3231 via I²C.
- `LiquidCrystal_I2C` ou `LiquidCrystal`: interface com display de caracteres.
- `Servo`: controle de pulso PWM para movimentação angular do servo motor.
- `HX711`: comunicação serial síncrona com amplificador de célula de carga.

### **11.13. ESP32: Comunicação Wi-Fi e Ponte MQTT**
O firmware do ESP32 terá responsabilidades estritas de rede e conectividade, abstendo-se de comandar diretamente os atuadores da gaiola. Seu projeto na IDE Arduino é dividido em três blocos operacionais:
- **WiFi_Manager:** gerencia a conexão e reconexão automática com a rede Wi-Fi local de 2.4 GHz.
- **MQTT_Client:** faz uso da biblioteca `PubSubClient` para estabelecer e manter a conexão TLS segura com o HiveMQ Cloud, gerenciando o ciclo de vida das inscrições de tópicos.
- **UART_Bridge:** funciona como ponte bidirecional. Mensagens JSON recebidas dos tópicos MQTT da nuvem são imediatamente impressas na porta Serial conectada ao Arduino Mega via `Serial.println()`. Em sentido inverso, mensagens JSON completas recebidas do Mega via UART são publicadas nos tópicos de telemetria e resposta da nuvem.

### **11.14. Painel Web: Organização do Código Backend e Frontend**
O repositório do painel web hospedado reúne os arquivos essenciais para execução no navegador:
- `index.html`: estrutura semântica contendo painéis de telemetria, indicadores de reservatório e botões de comando.
- `style.css`: estilização responsiva adaptada a telas mobile e desktop.
- `app.js`: lógica de aplicação contendo as rotinas da biblioteca **Paho MQTT** via WebSockets, tratamento de subscrição dos tópicos de status, tradução das ações do usuário em payloads JSON estruturados e uma **função geradora de IDs únicos (hashes)** para preencher o atributo `id` de cada comando antes do envio à nuvem.

---

## **12. Comunicação por JSON entre Arduino e ESP32**

As mensagens trocadas entre os controladores utilizam representações estruturadas em formato JSON, transmitidas sequencialmente linha a linha, empregando o caractere delimitador `\n`.

### **12.1. Exemplo de comando**
Formato de solicitação de alimentação manual originada pela nuvem ou interface digital:
```json
{"type":"command","id":"cmd-001","name":"feed_manual"}
```
- `type`: identifica a classe funcional da mensagem.
- `id`: identificador de rastreabilidade único, permitindo emparelhar uma resposta futura ao comando originário.
- `name`: mnemônico da operação solicitada.

### **12.2. Tipos de mensagem previstos**
| Tipo | Descrição e Finalidade |
| :--- | :--- |
| `command` | Dispara ações mecânicas ou solicita atualização de configurações operacionais. |
| `ack` | Confirma o recebimento imediato e a integridade sintática da mensagem. |
| `response` | Retorna o resultado final de processamento de um comando (ex: sucesso ou recusa por trava de segurança). |
| `telemetry` | Pacote periódico contendo leituras de nível, status de atuadores e horários do sistema. |
| `event` | Comunicações assíncronas pontuais (ex: término de abastecimento, alerta de limpeza vencida). |
| `error` | Informa inconsistência estrutural no JSON recebido ou parâmetro fora da faixa aceita. |

### **12.3. Recepção e validação**
O subsistema de recepção acumula bytes em buffer linear até encontrar o caractere `\n`. Em seguida, o buffer é processado pela biblioteca de JSON.
O sistema trata falhas como:
- Mensagens com estouro de tamanho máximo de buffer (*overflow*).
- Strings com JSON malformado ou chaves truncadas.
- Ausência de chaves obrigatórias (`type`, `id`).
- Comandos desconhecidos ou comandos fora dos limites aceitáveis.

### **12.4. Segurança lógica dos comandos**
Um pacote JSON válido não autoriza o acionamento cego de atuadores. Após a decodificação da mensagem, o módulo de protocolo despacha a requisição ao módulo operacional específico (`Alimentacao` ou `Hidratacao`). Se o reservatório de ração estiver vazio ou a bomba d'água atingir o limite de tempo contínuo, o comando será formalmente recusado e uma resposta de erro será despachada de volta à nuvem.

### **12.5. Atualizações de estado**
O Arduino transmite ciclicamente pacotes de telemetria refletindo o estado dos sensores e atuadores. A interface web utiliza essas atualizações para atualizar elementos visuais na tela. Caso a telemetria não seja recebida dentro de um período pré-definido, a interface marcará os dados em tela como desatualizados.

### **12.6. Tópicos MQTT e a ponte de dados**
O protocolo JSON trafegado na UART entre Arduino e ESP32 possui exatamente a mesma estrutura enviada aos tópicos MQTT da nuvem, eliminando a sobrecarga de tradução de dados no ESP32.
O Broker organiza o tráfego em dois canais principais:
- **Tópico de Comandos (`smartbird/gaiola01/comandos`):** o Painel Web publica comandos JSON neste canal. O ESP32, inscrito permanentemente no tópico, repassa o payload integralmente via UART ao Arduino Mega.
- **Tópico de Telemetria (`smartbird/gaiola01/status`):** o Arduino Mega formata as medições e status operacionais em JSON e os envia ao ESP32 via UART. O ESP32 os publica neste canal. O Painel Web consome os dados e atualiza a interface do usuário em tempo real.

---

## **13. Fluxo integrado de funcionamento**

O fluxo de processamento do sistema divide-se em cinco macroetapas encadeadas:
[ Inicialização ] ➔ [ Leitura dos Sensores ] ➔ [ Avaliação de Regras ] ➔ [ Atuação ] ➔ [ Atualização das Interfaces ]

### **13.1. Inicialização**
Na energização, o Arduino configura pinos de E/S, testa a resposta do RTC DS3231 no barramento I²C, garante que os atuadores estejam em modo seguro e desativados, e inicializa as variáveis de controle.

### **13.2. Atualização dos sensores**
Em intervalos controlados por temporizadores baseados na função `millis()`, o sistema dispara as leituras de distância ultrassônica e da célula de carga, calculando percentuais e preenchendo as variáveis estruturadas globais.

### **13.3. Avaliação das regras**
Os módulos operacionais comparam as variáveis do sistema com parâmetros definidos:
- O horário retornado pelo RTC coincide com a rotina de alimentação configurada?
- O nível de água caiu abaixo do patamar de acionamento mínimo?
- A contagem de tempo ultrapassou as 48 horas de higienização?
- Houve recepção de comandos manuais vindos dos botões físicos ou da nuvem?

### **13.4. Execução dos atuadores**
Caso as condições operacionais e de intertravamento de segurança sejam cumpridas, o circuito correspondente é habilitado (movimentação angular do servo motor ou disparo do relé/driver da minibomba). O tempo de operação ativa é monitorado em tempo real.

### **13.5. Atualização das interfaces**
As informações consolidadas são despachadas simultaneamente:
- A interface física exibe as leituras no display LCD 16x2 e atualiza o estado dos LEDs indicadores.
- A interface serial transmite o frame de dados em JSON ao ESP32, que o encaminha ao broker MQTT para exibição no painel web.

---

## **14. Execução não bloqueante**

O código-fonte do microcontrolador não utiliza instruções de congelamento de fluxo como `delay()`. Rotinas bloqueantes impedem o atendimento de requisições seriais e prejudicam a precisão temporal dos sensores ultrassônicos.
Toda temporização é implementada por meio de comparadores temporais baseados em `millis()`, organizados sob uma máquina de estados:

| Tarefa Operacional | Intervalo de Execução Recomendado | Justificativa |
| :--- | :--- | :--- |
| Amostragem ultrassônica | 500 ms – 1000 ms | Redução de ruídos e estabilização de eco |
| Consulta do RTC DS3231 | 1000 ms | Granularidade suficiente para controle de horários |
| Varredura dos botões físicos | 20 ms – 50 ms | Aplicação de lógica anti-repique (*debounce*) |
| Atualização da tela do LCD | 500 ms | Legibilidade humana e alívio do barramento |
| Telemetria Serial UART/MQTT | 2000 ms – 5000 ms | Redução do tráfego na rede e economia de banda |

---

## **15. Alimentação elétrica e montagem física**

A separação dos circuitos de controle e dos circuitos de potência é fundamental para evitar reinicializações espúrias no microcontrolador causadas por indução eletromagnética dos atuadores.

### **15.1. Alimentação dos atuadores**
O microcontrolador Arduino Mega fornece apenas sinais lógicos de controle. Motores e bombas consomem picos de corrente que excedem a capacidade de saída dos pinos digitais da placa (limitados a cerca de 20–40 mA):
- **Bomba de Hidratação:** alimentada por linha de potência dedicada, chaveada por meio de transistor MOSFET ou módulo relé com isolamento óptico (optoacoplador) e diodo de retorno (*flyback*).
- **Servo Motor de Alimentação:** alimentado por fonte de tensão de 5V dimensionada para suportar picos de corrente de até 1A em situações de torque elevado.

### **15.2. Referência elétrica**
Todos os barramentos de alimentação secundários devem compartilhar a mesma referência física de terra (GND comum). A equalização de potencial de GND entre Arduino Mega e ESP32 é requisito mandatório para a estabilidade do canal serial UART.

### **15.3. Proteção contra água**
Os recipientes de líquidos e a tubulação de recirculação devem ficar mecanicamente isolados da eletrônica de controle. Fios e cabos que partem da câmara úmida devem conter curvas de gotejamento para evitar que líquidos escorram em direção às placas em caso de vazamentos.

### **15.4. Estrutura do protótipo**
A estrutura mecânica externa será montada em placas de MDF cortadas a laser, combinadas com componentes modulares impressos em 3D (suportes para bicos de alimentação, calhas para direcionamento de ração e abraçadeiras de fixação dos sensores ultrassônicos).

### **15.5. Conectividade Wi-Fi e Estratégia de Apresentação**
O módulo ESP32 exige uma rede Wi-Fi estável na frequência de 2.4 GHz para dialogar com a nuvem e viabilizar a interface web.
- **Bloqueios de Rede:** Redes corporativas e universitárias frequentemente utilizam firewalls restritivos ou portais cativos de autenticação web. Como o ESP32 não conta com navegador interno para cumprir etapas de login manual, a tentativa de conexão com brokers MQTT na nuvem por essas redes pode ser bloqueada.
- **Ambiente de Demonstração (Banca Avaliadora):** Para garantir estabilidade técnica durante a demonstração prática perante a banca acadêmica, o ESP32 será configurado previamente para conectar-se exclusivamente a uma rede de ponto de acesso portátil (*Hotspot 4G/5G*) originada pelo smartphone de um dos membros da equipe. Essa estratégia assegura conexão livre de firewalls, garante baixa latência e comprova o controle remoto real, permitindo que os próprios avaliadores acionem a gaiola a partir de seus celulares por meio da rede externa.
- **Adequação de Tensão de Hardware:** É obrigatório o emprego de um **Módulo Conversor de Nível Lógico Bidirecional (3.3V – 5V)** entre a porta Serial1 do Arduino Mega (5V) e os pinos de recepção UART do ESP32 (3.3V). O envio de pulsos diretos de 5V do Mega ao ESP32 pode danificar o módulo de conectividade ou comprometer a integridade dos dados trafegados.

---

## **16. Organização do trabalho em equipe e Git**

O desenvolvimento do firmware, dos circuitos e do software web adotará controle de versão distribuído via Git, hospedado no GitHub, utilizando a abordagem de ramificações baseadas em funcionalidades (*feature branches*).

### **16.1. Branch principal**
A branch `main` mantém o código funcionalmente consolidado e validado nos testes integrados. Commits diretos na `main` são restritos; toda alteração deve ingressar por meio de mesclagens supervisionadas (*pull requests*).

### **16.2. Branches sugeridas**
```text
main  
├── feature/reservatorios  
├── feature/alimentacao  
├── feature/hidratacao  
├── feature/interface-lcd  
├── feature/tempo-manutencao  
├── feature/comunicacao-uart  
└── feature/painel-web-mqtt
```

### **16.3. Fluxo de trabalho**
1. Sincronizar localmente a branch `main` atualizada.
2. Criar a branch de escopo específico (`feature/nome-da-tarefa`).
3. Desenvolver código e testes com commits pequenos, atômicos e descritivos.
4. Submeter a branch para o repositório remoto e abrir o correspondente *Pull Request* (PR).
5. Revisão de código (*peer review*) realizada por outro integrante da equipe.
6. Mesclagem (*merge*) aprovada após confirmação de não quebra dos contratos e módulos existentes.

### **16.4. Contratos entre módulos**
Antes de iniciar a codificação isolada das rotinas, a equipe deve formalizar o cabeçalho compartilhado `EstadoSmartBird.h` e o dicionário de dados JSON do protocolo. Isso garante que a representação de dados (ex: percentual de 0 a 100 em vez de níveis em centímetros) seja uniforme em todo o sistema.

---

## **17. Estratégia de desenvolvimento e testes**

A implementação do sistema deve avançar por estágios graduais e incrementais de validação:

### **17.1. Etapa 1 — Definição técnica**
Dimensionamento dos recipientes físicos, seleção de fornecedores de atuadores e sensores, elaboração do mapa de pinos do microcontrolador e definição dos schemas de mensagens.

### **17.2. Etapa 2 — Testes individuais**
Validação em bancada de cada componente eletrônico de forma isolada:
- Avaliação da margem de erro dos módulos ultrassônicos.
- Calibração da célula de carga e obtenção do fator de escala no amplificador HX711.
- Varredura angular controlada do servo motor para mapear aberturas de dosagem.
- Medição da vazão e consumo de corrente da bomba de hidratação.
- Validação do barramento I²C com RTC e display LCD em operação simultânea.
- Validação do conversor de nível lógico entre a UART do Mega e a do ESP32.

### **17.3. Etapa 3 — Desenvolvimento modular**
Implementação individual de cada arquivo funcional do firmware, simulando entradas de hardware via constantes de teste antes de conectá-los fisicamente.

### **17.4. Etapa 4 — Integração do firmware local**
Junção dos módulos operacionais no Arduino Mega. Validação do escalonador não bloqueante garantindo que a varredura contínua de segurança não seja interrompida durante acionamentos mecânicos.

### **17.5. Etapa 5 — Integração com o ESP32 e Nuvem**
Conexão física da UART adaptada (Mega-ESP32), conexão Wi-Fi do ESP32 via Hotspot de teste e estabelecimento da sessão MQTT com o HiveMQ Cloud. Validação do envio de telemetria em formato JSON e recepção de comandos pela ponte UART.

### **17.6. Etapa 6 — Testes integrados**
Execução de rotinas completas simulando o ciclo de operação diário: alimentação por timer do RTC, esgotamento do nível de reservatório, timeout de proteção de bomba d'água e interrupção proposital da rede Wi-Fi.

### **17.7. Etapa 7 — Validação física e demonstração**
Montagem final de chicotes, organização de fiação nas canaletas da estrutura em MDF e testes de estresse em bancada. Não serão utilizados animais vivos durante as etapas de testes de desenvolvimento ou apresentações preliminares.

---

## **18. Tratamento de falhas e comportamento seguro**

A estabilidade de um sistema embarcado autônomo fundamenta-se na capacidade de lidar de forma segura e previsível com anomalias de sensores, falhas mecânicas e instabilidades elétricas.

### **18.1. Falha de sensor**
Leituras fora dos limites do conversor analógico-digital ou ausência de pulso de eco no sensor ultrassônico não devem ser tratadas como reservatório em nível zero. O sistema deve marcar a medição como `INVALID`, suspender acionamentos automáticos associados e emitir um aviso na interface.

### **18.2. Falha de comunicação**
Se o Arduino Mega deixar de receber comunicações do ESP32, ou se o broker MQTT ficar inacessível, o sistema local manterá suas rotinas de controle de forma totalmente autônoma. Ao restabelecer a conexão, os estados atuais serão reenviados à nuvem sem a necessidade de reinicializar o controlador local.

### **18.3. Falha de atuador**
Caso o sensor de nível não acuse variação durante o acionamento da bomba, o firmware desliga o motor automaticamente por *timeout* de segurança, evitando que o dispositivo funcione a seco ou cause transbordamento.

### **18.4. Reinicialização**
Após uma queda de energia ou reset intencional, o programa relê os estados dos sensores e restaura os parâmetros persistidos antes de liberar qualquer atuador. Tarefas agendadas que deveriam ter ocorrido durante o período desligado não são executadas retroativamente de forma cumulativa.

### **18.5. Segurança do animal**
Todos os mecanismos de dosagem e corte de ração devem ser projetados para evitar o aprisionamento do bico ou patas da ave. O sistema conta com limites estruturais de hardware para evitar sobrealimentação ou privação de água em caso de falha de software.

---

## **19. Decisões pendentes e pontos que exigem validação**

| Item | Proposta Inicial | Ponto Pendente de Confirmação |
| :--- | :--- | :--- |
| **Controlador Local** | Arduino Mega 2560 | Homologar a disponibilidade da placa na bancada física. |
| **Controlador IoT** | ESP32 NodeMCU | Confirmar o modelo exato e quantidade de pinos UART disponíveis. |
| **Sensores de Nível** | Módulos ultrassônicos (ex: HC-SR04) | Definir zonas cegas mínimas e necessidade de blindagem contra umidade. |
| **Reservatórios** | Geometria prismática retangular | Definir volume útil final e altura de coluna d'água em centímetros. |
| **Medição de Peso** | Célula de carga com módulo HX711 | Decidir entre instalação na base da bandeja ou descarte do componente. |
| **Atuador de Grãos** | Mini servo motor (ex: SG90 / MG90S) | Validar se o torque mecânico é suficiente para movimentar as porções de grãos. |
| **Bomba d'água** | Minibomba submersível DC 5V/12V | Definir a tensão de alimentação e o modelo de circuito de acionamento (transistor/relé). |
| **Persistência** | EEPROM interna do Mega | Mapear os blocos de endereçamento de memória para gravação das marcações de limpeza. |

---

## **20. Organização dos documentos técnicos**

A documentação de engenharia do projeto é organizada em três volumes complementares:

### **20.1. Documento de visão geral e arquitetura**
O presente material, cobrindo o escopo funcional, a fundamentação teórica de operação, o particionamento de módulos de software, o protocolo JSON, a estrutura MQTT na nuvem e as rotinas de tratamento de falhas.

### **20.2. Documento de ligações elétricas**
Representação esquemática do circuito elétrico contendo barramentos de alimentação de 5V e 3.3V, transistores de acionamento, diodos de proteção, conversores de nível lógico e ligações dos módulos sensores.

### **20.3. Documento de mapa de pinos e alimentação**
Tabela detalhada consolidando o mapeamento de pinagem física (*pinout*) do microcontrolador:
- Atribuição de pinos para barramentos compartilhados (I²C: SDA/SCL).
- Canais dedicados de comunicação serial UART1 (Pinos 18 e 19).
- Entradas digitais com resistores de *pull-up* ativados para botões físicos.
- Saídas de sinal PWM para o servo motor e pulsos lógicos de gatilho para os sensores ultrassônicos.
- Balanço energético indicando o consumo de corrente estimado de cada periférico e o ponto de extração de alimentação (barramento da fonte externa vs. barramento regulado da placa).

---

## **21. Apresentação acadêmica e demonstração do protótipo**

O projeto acadêmico deve ser apresentado com foco na justificativa de engenharia, viabilidade de implantação e integração de sistemas distribuídos (Hardware, Firmware, IoT e Web).

### **21.1. O que a equipe precisa dominar**
- Argumentação sobre o problema de manejo de aves e a relevância da automação proposta.
- Mecânica de cálculo da conversão de distância ultrassônica em estimativa percentual de volume.
- Justificativa da arquitetura em dois níveis (Arduino Mega para controle local de tempo real + ESP32 para conectividade).
- Papel do conversor de nível lógico (3.3V – 5V) na segurança dos pinos de comunicação serial.
- Funcionamento do modelo Publicador/Inscrito via MQTT através do broker HiveMQ Cloud.
- Procedimento de contingência para redes restritas por meio de Hotspot móvel 4G/5G.

### **21.2. Viabilidade técnica**
A separação de responsabilidades comprova que o sistema não depende de conexões remotas para manter as rotinas essenciais de alimentação e hidratação. O protótipo demonstra viabilidade técnica pelo uso de componentes consolidados e arquitetura modular de baixo custo.

### **21.3. Roteiro sugerido de demonstração**
1. Exibição da leitura dos reservatórios no display LCD 16x2 da bancada física.
2. Alteração de nível controlada no reservatório com atualização automática dos dados no LCD.
3. Demonstração da transmissão do pacote de telemetria via MQTT, atualizando o painel web em tempo real.
4. Acionamento do botão "Alimentar" no painel web do smartphone, demonstrando o ciclo: envio MQTT ➔ recepção no ESP32 ➔ repasse via UART ➔ validação e movimentação do servo pelo Arduino Mega.
5. Retorno visual de status *completed* na tela do smartphone após a conclusão do movimento.
6. Simulação de desconexão da rede sem interrupção das rotinas locais do Arduino Mega.

### **21.4. Perguntas que a equipe deve estar preparada para responder**
- *Como o sistema lida com o transbordamento do comedouro caso grãos fiquem acumulados sob o dosador?*
- *O que acontece na placa ESP32 caso a tensão de 5V da porta do Arduino Mega seja ligada diretamente sem conversor de nível?*
- *Como o sistema garante que uma oscilação na rede Wi-Fi não provoque o disparo repetido de uma mesma rotina de alimentação?*
- *Qual a estratégia para evitar gravações excessivas que degradem a vida útil da memória EEPROM durante o registro do lembrete de manutenção?*

---

## **22. Possibilidades de evolução**

A estrutura desacoplada em módulos viabiliza atualizações de software e expansões graduais de hardware sem retrabalho da base:
- **Aplicativo Mobile Dedicado:** empacotamento do frontend web atual em aplicações nativas Android/iOS usando frameworks híbridos.
- **Telemetria de Consumo:** gráficos históricos semanais no painel web apontando oscilações de consumo hídrico e alimentar para suporte veterinário.
- **Controle Térmico:** integração de sensor de temperatura e umidade (ex: DHT22) e acionamento de microventilador para conforto térmico da ave.
- **Câmera de Monitoramento:** inclusão de módulo ESP32-CAM dedicado para transmissão de imagens em tempo real dentro da gaiola.

---

## **23. Conclusão**

O SmartBird consolida uma arquitetura distribuída para o monitoramento e manejo automatizado de aves domésticas, unindo controle em tempo real e supervisão remota via nuvem.
A distribuição de responsabilidades entre o Arduino Mega 2560 (focado em controle de baixo nível, intertravamentos de segurança e acionamento local) e o ESP32 (responsável pela conectividade Wi-Fi, comunicação MQTT com o HiveMQ Cloud e ponte de dados bidirecional) garante que o sistema mantenha o funcionamento local independente do estado da rede externa.
A estrutura modular do código-fonte, a padronização das mensagens em formato JSON com campos identificadores (`id`), o isolamento elétrico das linhas de potência e o emprego de técnicas de programação não bloqueante formam a base para o desenvolvimento progressivo e coordenado da equipe ao longo das etapas de montagem, testes de bancada e apresentação acadêmica.