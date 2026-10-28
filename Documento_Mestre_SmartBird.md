SMARTBIRD

Documento Mestre do Projeto — Versão 2.0

Arquitetura, funcionamento, requisitos, desenvolvimento e integração do sistema

Projeto acadêmico: SmartBird
Área: Automação, sistemas embarcados, programação e monitoramento
Plataforma principal proposta: Arduino Mega 2560
Plataforma complementar proposta: ESP32
Contexto: Projeto universitário desenvolvido em equipe
Status: Planejamento e estruturação técnica; componentes, montagem e implementação ainda dependem de definições e validações da equipe.

⸻

1. Visão geral do SmartBird

O SmartBird é um projeto de automação voltado à alimentação, hidratação e ao monitoramento de aves domésticas. A proposta é desenvolver um sistema integrado capaz de acompanhar condições importantes para a rotina da ave, executar determinadas tarefas automaticamente e informar ao responsável quando houver necessidade de intervenção.

A ideia central é reduzir a dependência de verificações manuais constantes, tornando o manejo mais organizado e previsível. Para isso, o sistema combina sensores, atuadores, controle de tempo, interface física e, como expansão prevista na arquitetura, uma interface digital conectada a uma placa ESP32.

O projeto não deve ser entendido como um simples alimentador automático. Seu diferencial está na integração de diferentes funções: monitoramento dos reservatórios, controle da liberação de alimento, controle da água, acompanhamento de horários, alertas de manutenção e possibilidade de configuração pelo usuário.

O funcionamento deve ser organizado em módulos independentes, mas capazes de trocar informações entre si. Dessa forma, cada parte pode ser desenvolvida e testada separadamente antes da integração definitiva.

O sistema também deve considerar situações em que algum sensor falhe, um atuador não responda ou a comunicação entre as placas seja interrompida. A automação precisa agir de maneira previsível nessas condições, sem interpretar uma leitura inválida como se fosse uma medição normal.

1.1. Objetivos gerais

* Automatizar etapas da alimentação e da hidratação da ave.
* Monitorar os níveis de conteúdo nos reservatórios.
* Permitir o acionamento de determinadas funções por horário e por comando manual.
* Informar ao usuário o estado geral do sistema.
* Emitir alertas relacionados à manutenção e a condições que exijam atenção.
* Permitir a configuração de parâmetros operacionais.
* Preparar uma arquitetura que possibilite uma interface digital por meio do ESP32.
* Organizar o código para facilitar o trabalho simultâneo da equipe.
* Desenvolver um protótipo universitário funcional, demonstrável e tecnicamente justificável.

1.2. Princípios de funcionamento

O SmartBird deverá seguir alguns princípios fundamentais.

Automação com supervisão: tarefas rotineiras poderão ser executadas automaticamente, mas o responsável continuará podendo acompanhar o estado do sistema e realizar ações manuais autorizadas.

Monitoramento contínuo: os sensores deverão ser consultados periodicamente, sem que a execução de uma tarefa bloqueie todo o restante do programa.

Separação de responsabilidades: cada módulo deverá cuidar de uma função específica, como medir os reservatórios, controlar a alimentação ou atualizar a interface.

Funcionamento local: as funções essenciais deverão continuar operando mesmo que a interface digital, a rede Wi-Fi ou a conexão entre placas esteja indisponível, desde que o controlador principal e os componentes locais estejam funcionando.

Segurança: leituras inválidas, reservatórios vazios, níveis excessivos, falhas de comunicação e tempos de acionamento ultrapassados deverão ser tratados explicitamente.

Configuração controlada: parâmetros alteráveis pelo usuário deverão ser validados antes de serem aplicados.

⸻

2. Escopo funcional do projeto

O SmartBird está organizado em cinco grandes áreas funcionais.

2.1. Alimentação

O sistema deverá acompanhar a condição do alimento e controlar um mecanismo de liberação, utilizando um servo motor como atuador. A lógica poderá considerar horários programados, quantidade de alimento disponível e comandos manuais.

A proposta inicial também considera uma medição de peso na bandeja ou no recipiente de alimentação, utilizando uma célula de carga e um módulo HX711. Essa medição poderá auxiliar na identificação de uma quantidade baixa de alimento e no controle da quantidade liberada.

2.2. Hidratação

O sistema deverá monitorar a água disponível e controlar uma bomba responsável por movimentar ou repor água, conforme a configuração física escolhida.

O controle deverá considerar níveis de água, horários e comandos manuais. A bomba não poderá permanecer ligada indefinidamente: deverá existir um limite de operação e uma condição de parada segura.

2.3. Monitoramento dos reservatórios

Os reservatórios deverão ser monitorados por sensores ultrassônicos instalados acima de seu conteúdo, conforme a proposta mais recente discutida.

Esses sensores permitirão estimar o nível disponível sem depender de contato direto com o alimento ou a água. O sistema precisará conhecer as dimensões de referência e os limites configurados para transformar a distância medida em uma estimativa de nível.

2.4. Interface e alertas

Uma interface física com LCD, botões e indicadores permitirá consultar informações essenciais e realizar ações básicas.

A proposta de expansão inclui uma interface digital por meio do ESP32, oferecendo uma visualização mais completa, configurações e informações de diagnóstico.

2.5. Manutenção

O sistema deverá acompanhar o tempo transcorrido desde a última limpeza confirmada. A proposta inicial estabelece um lembrete aproximadamente a cada dois dias, ou 48 horas.

Quando o intervalo for atingido, o sistema deverá emitir um alerta. A contagem somente deverá ser reiniciada após uma confirmação explícita de que a limpeza foi realizada.

⸻

3. Arquitetura geral do sistema

A arquitetura proposta utiliza dois controladores com responsabilidades diferentes: um Arduino Mega 2560 para o controle local e um ESP32 para comunicação e interface digital.

Essa divisão evita concentrar toda a lógica em uma única placa e permite desenvolver a automação principal independentemente da interface conectada.

3.1. Arduino Mega 2560 — controlador principal

O Arduino Mega 2560 é a plataforma principal proposta para o protótipo. Sua função será executar a lógica de controle e coordenar os módulos responsáveis pelos sensores e atuadores.

Entre suas responsabilidades estão:

* Ler os sensores físicos.
* Atualizar o estado dos reservatórios.
* Controlar o servo motor da alimentação.
* Controlar a bomba de hidratação por meio de um circuito de acionamento apropriado.
* Consultar o relógio de tempo real.
* Executar as rotinas de alimentação e hidratação.
* Controlar o LCD, os botões e os indicadores locais.
* Verificar o intervalo de manutenção.
* Receber comandos válidos do ESP32.
* Enviar informações de estado para a interface digital.
* Tratar falhas e condições de segurança.

O Arduino deverá ser capaz de continuar executando as funções locais essenciais sem depender permanentemente do ESP32.

3.2. ESP32 — comunicação e interface digital

O ESP32 é previsto como a plataforma complementar para conectar o sistema a uma interface digital. A arquitetura permite que ele ofereça recursos de comunicação, visualização e configuração sem precisar assumir diretamente o controle de cada atuador.

A interface digital poderá apresentar informações como:

* Estado geral do SmartBird.
* Níveis estimados dos reservatórios.
* Estado da alimentação.
* Estado da hidratação.
* Horários e parâmetros configurados.
* Situação do lembrete de limpeza.
* Alertas e falhas identificadas.
* Informações de diagnóstico.
* Comandos manuais autorizados.
* Parâmetros de calibração e limites operacionais.

A forma final dessa interface — por exemplo, uma página web acessada localmente — ainda depende da decisão de implementação da equipe. A presença do ESP32, por si só, não significa que todas essas funcionalidades já estejam implementadas.

O ESP32 deverá enviar solicitações ao Arduino e receber as respostas correspondentes. Ele não deverá assumir que uma solicitação foi executada apenas porque conseguiu transmiti-la.

3.3. Comunicação entre os controladores

A proposta é utilizar comunicação serial UART entre o Arduino Mega e o ESP32.

No Arduino Mega, a interface Serial1 utiliza:

* TX1: pino digital 18.
* RX1: pino digital 19.

A ligação deverá cruzar os sinais: TX de uma placa conectado ao RX da outra, e vice-versa. As duas placas também precisam compartilhar o GND de referência.

Existe, entretanto, uma diferença elétrica importante: o Mega trabalha com lógica de 5 V, enquanto os pinos de comunicação do ESP32 utilizam lógica de 3,3 V. O sinal de transmissão do Mega para a entrada RX do ESP32 deverá passar por uma adaptação de nível apropriada. Não se deve conectar diretamente um sinal de 5 V a um pino do ESP32 sem verificar a compatibilidade elétrica.

Como ponto de partida, foi sugerida uma comunicação serial a 115200 baud, com formato 8N1. A velocidade definitiva deverá ser validada durante os testes.

A serial USB principal do Arduino poderá continuar disponível para depuração, enquanto a Serial1 ficará dedicada à comunicação entre as placas.

⸻

4. Funcionamento dos reservatórios e sensores ultrassônicos

O monitoramento dos reservatórios é uma parte central da arquitetura, porque as informações de nível influenciam decisões de alimentação, hidratação e apresentação de alertas.

A proposta mais recente considera sensores ultrassônicos instalados na parte superior dos reservatórios, apontados para a superfície do conteúdo.

4.1. Princípio de medição

O sensor ultrassônico emite um pulso e mede o tempo necessário para receber seu retorno. Com esse tempo, o sistema estima a distância entre o sensor e a superfície detectada.

Em termos práticos:

* Quando o conteúdo sobe, a distância medida tende a diminuir.
* Quando o conteúdo baixa, a distância medida tende a aumentar.

A medição direta é uma distância, não uma porcentagem. Para apresentar um nível compreensível, o programa deverá utilizar medidas de referência e parâmetros de calibração.

4.2. Conversão da distância em nível

O sistema deverá conhecer a distância de referência entre o sensor e o ponto de nível zero do reservatório, além da altura útil de conteúdo.

Considere:

* distanciaMedida: distância atual entre o sensor e a superfície.
* distanciaReferencia: distância entre o sensor e a referência de nível zero.
* alturaUtil: altura útil disponível para o conteúdo.
* alturaConteudo: altura estimada do conteúdo.
* percentualNivel: estimativa percentual do nível.

A lógica conceitual será:

alturaConteudo = distanciaReferencia - distanciaMedida

Depois, a estimativa percentual poderá ser calculada por:

percentualNivel = (alturaConteudo / alturaUtil) * 100

O resultado deverá ser limitado ao intervalo de 0% a 100% quando a medição for válida. Leituras fora do comportamento esperado deverão ser investigadas e não simplesmente interpretadas como níveis normais.

Exemplo ilustrativo: se a altura útil for de 20 cm e o conteúdo ocupar aproximadamente 10 cm dessa altura, o nível estimado será de 50%.

Essa estimativa representa a altura ocupada. Ela só representa aproximadamente a porcentagem de volume quando a geometria do reservatório permite essa relação. Reservatórios com formatos irregulares podem exigir uma tabela de calibração ou outro método de conversão.

4.3. Parâmetros de calibração

Cada reservatório poderá precisar de parâmetros próprios, porque os sensores podem ser instalados em alturas diferentes e os recipientes podem possuir dimensões distintas.

Os parâmetros previstos incluem:

* Distância de referência do sensor.
* Altura útil do reservatório.
* Limite para considerar o nível baixo.
* Limite para considerar o nível adequado.
* Limite para considerar o reservatório cheio, quando aplicável.
* Margens de tolerância para evitar oscilações constantes de estado.
* Validade mínima da leitura.
* Identificação do reservatório associado ao sensor.

Esses valores deverão ser configuráveis quando necessário, mas sempre submetidos a validação. Não basta permitir que o usuário digite um número: o programa deverá verificar se ele é compatível com as dimensões e os limites definidos.

4.4. Estados dos reservatórios

A lógica poderá classificar os reservatórios em estados como:

* Normal: nível dentro da faixa esperada.
* Baixo: nível abaixo do limite configurado.
* Crítico: nível muito baixo, quando essa classificação for necessária.
* Cheio: nível próximo do limite superior configurado.
* Leitura inválida: sensor sem retorno confiável ou medição fora das condições esperadas.
* Indisponível: sensor ainda não inicializado ou sem atualização válida.

A classificação definitiva deverá ser definida pela equipe, considerando a capacidade dos recipientes e o comportamento real dos sensores.

Um sensor que não recebeu retorno não deverá produzir automaticamente uma leitura de 0%. Ausência de medição e reservatório vazio são condições diferentes.

4.5. Frequência de leitura e estabilidade

As leituras deverão ocorrer periodicamente, sem bloquear o restante do programa.

Como sensores ultrassônicos podem apresentar pequenas oscilações, poderá ser necessário filtrar as medições, rejeitar valores claramente inválidos ou considerar mais de uma leitura antes de alterar o estado do reservatório.

Também será importante evitar que o sistema alterne repetidamente entre “normal” e “baixo” quando o nível estiver próximo do limite. Uma margem de histerese poderá ajudar a estabilizar a classificação.

4.6. Relação com os outros módulos

O módulo responsável pelos reservatórios deverá concentrar a leitura ultrassônica, a calibração e a interpretação dos dados.

Os módulos de alimentação e hidratação deverão consumir o estado já calculado, sem precisar conhecer os detalhes elétricos ou o algoritmo interno dos sensores.

Essa separação permite trocar um sensor, modificar a calibração ou alterar o método de leitura sem reescrever toda a lógica de alimentação e hidratação.

A quantidade definitiva de sensores e a associação física de cada um ainda precisam ser confirmadas. A proposta de arquitetura considera monitoramento dos reservatórios de alimento e água, mas a disposição final deverá seguir a montagem física escolhida.

⸻

5. Módulo de alimentação

O módulo de alimentação deverá controlar a liberação de alimento e acompanhar as condições necessárias para realizar essa operação.

A proposta inicial considera um mecanismo acionado por servo motor e uma medição de peso utilizando célula de carga e módulo HX711. O monitoramento ultrassônico do reservatório também poderá fornecer informações sobre a disponibilidade geral de alimento.

5.1. Componentes previstos

* Servo motor para controlar o mecanismo de liberação.
* Célula de carga, caso a medição de peso seja mantida.
* Módulo HX711 para leitura da célula de carga.
* Sensor ultrassônico do reservatório de alimento, conforme a arquitetura de monitoramento.
* Arduino Mega como controlador.
* Interface física e digital para exibir o estado e solicitar ações.

O posicionamento da célula de carga precisa ser definido com cuidado. Se ela estiver sob a bandeja, medirá o peso existente nessa bandeja; se estiver sob o reservatório, medirá outra grandeza. Essas medidas não são intercambiáveis, e o software deverá ser desenvolvido de acordo com a montagem real.

5.2. Alimentação automática por horário

A rotina deverá consultar o horário fornecido pelo relógio de tempo real e verificar se existe uma alimentação programada para aquele momento.

Quando o horário for atingido, o sistema deverá avaliar as condições necessárias para liberar alimento.

A operação deverá considerar, no mínimo:

1. Se existe uma programação válida.
2. Se o horário corresponde a uma alimentação prevista.
3. Se o mecanismo está disponível.
4. Se o nível do reservatório é válido e suficiente.
5. Se a quantidade de alimento disponível ou o peso da bandeja permitem a operação, conforme a lógica adotada.
6. Se não existe uma falha que impeça o acionamento.

O servo deverá se movimentar de acordo com uma sequência definida, evitando movimentos indefinidos ou acionamentos repetidos a cada ciclo do programa.

5.3. Alimentação com base no peso

A medição de peso poderá auxiliar a identificar quando a bandeja estiver com pouco alimento.

Para isso, a célula de carga deverá passar por calibração, e o programa precisará converter as leituras do HX711 em uma unidade útil, como gramas.

O controle deverá distinguir entre:

* Peso dentro da faixa esperada.
* Peso abaixo do limite configurado.
* Peso incompatível com a medição válida.
* Falha ou ausência de leitura.

Caso a lógica utilize um peso-alvo, o sistema poderá liberar alimento em pequenas etapas e verificar novamente a condição após cada operação, respeitando limites de tempo e de acionamento.

O comportamento exato — por exemplo, se o peso será apenas um indicador de bandeja vazia ou se controlará diretamente a quantidade liberada — ainda precisa ser definido pela equipe.

5.4. Alimentação manual

A interface deverá permitir uma solicitação manual de alimentação, desde que o sistema esteja em condições seguras.

A solicitação poderá vir dos botões físicos ou da interface digital. Independentemente da origem, deverá passar pela mesma validação central utilizada pela automação.

Um comando manual não deverá ignorar limites de segurança, como a indisponibilidade do mecanismo ou uma leitura crítica que impeça a operação.

5.5. Limites de operação

O módulo deverá possuir limites para evitar que o servo permaneça acionado ou que o mecanismo seja movimentado indefinidamente.

Também será necessário definir como o sistema se comportará se:

* O reservatório estiver vazio ou com nível crítico.
* A leitura ultrassônica for inválida.
* A medição de peso falhar.
* O servo não responder como esperado.
* Um comando repetido for recebido.
* O sistema reiniciar durante uma operação.

O comportamento seguro dependerá da construção mecânica. Por isso, a posição de repouso, os ângulos de abertura e fechamento e o tempo máximo de acionamento deverão ser determinados durante os testes do protótipo.

⸻

6. Módulo de hidratação

O módulo de hidratação deverá acompanhar a disponibilidade de água e controlar a bomba responsável pela movimentação ou reposição de água.

O projeto prevê a utilização de um sensor de nível, além do monitoramento ultrassônico dos reservatórios discutido anteriormente. A configuração física deverá esclarecer se esses recursos medirão o mesmo recipiente, partes diferentes do sistema ou condições complementares.

6.1. Componentes previstos

* Bomba de água.
* Circuito de acionamento apropriado, como um driver ou módulo de potência compatível.
* Sensor ou sensores de nível definidos para a montagem.
* Sensor ultrassônico do reservatório, conforme a arquitetura final.
* Arduino Mega.
* Relógio de tempo real para as rotinas programadas.
* Interface física e digital.

A bomba não deverá ser alimentada diretamente por um pino do Arduino. O circuito de potência e a fonte deverão ser dimensionados conforme a tensão e a corrente exigidas pelo modelo escolhido.

6.2. Hidratação automática

A lógica de hidratação poderá considerar três tipos de solicitação:

* Horário programado.
* Nível de água abaixo do limite configurado.
* Comando manual do usuário.

Antes de ligar a bomba, o módulo deverá verificar se a leitura do nível é válida e se a operação pode ser executada com segurança.

A bomba deverá ser desligada quando a condição de parada for atingida. Se houver um nível-alvo, a leitura deverá ser atualizada durante a operação para impedir que o sistema continue bombeando além do necessário.

6.3. Limites e falhas

A bomba deverá possuir um tempo máximo de funcionamento por operação. Esse limite é especialmente importante quando o sensor não responde ou quando a água não chega ao nível esperado.

O módulo deverá prever situações como:

* Reservatório sem água disponível.
* Nível máximo atingido.
* Sensor de nível inválido.
* Ausência de mudança esperada no nível.
* Tempo máximo de bombeamento excedido.
* Falha de comunicação com a interface digital.
* Reinicialização do controlador durante a operação.

Se uma leitura essencial for inválida, o comportamento mais seguro normalmente será impedir ou interromper a operação automática até que a condição seja avaliada.

6.4. Comandos manuais

A interface poderá disponibilizar um comando para solicitar hidratação manual. O comando deverá ser interpretado como uma solicitação, não como autorização para manter a bomba ligada indefinidamente.

A execução continuará sujeita aos limites definidos pelo módulo, incluindo tempo máximo, condições de nível e bloqueios de segurança.

⸻

7. Relógio de tempo real e agendamento

O projeto prevê a utilização do módulo RTC DS3231 para fornecer uma referência de horário estável ao controlador.

O relógio será importante para as rotinas periódicas de alimentação, hidratação e manutenção.

7.1. Responsabilidades do RTC

O módulo de tempo deverá permitir:

* Consultar a data e a hora.
* Identificar os horários programados.
* Comparar o horário atual com os horários de operação.
* Calcular intervalos transcorridos.
* Apoiar o controle do lembrete de limpeza.
* Detectar condições em que a leitura do relógio não seja confiável.

O RTC fornece a referência temporal, mas não deverá controlar diretamente o servo ou a bomba. A decisão de executar uma ação pertence ao módulo responsável pela função.

7.2. Agendamentos

Os horários deverão ser armazenados em uma estrutura de configuração compreensível pelo programa.

A lógica deverá evitar que um horário programado seja interpretado como um novo evento a cada ciclo do loop(). Será necessário registrar que determinada ocorrência já foi tratada, ou utilizar uma estratégia equivalente que impeça acionamentos duplicados.

Se houver mais de um horário programado, o sistema deverá tratar cada ocorrência de maneira independente.

7.3. Falhas de horário

O sistema deverá considerar o que acontece se o RTC não for inicializado, apresentar uma data inválida ou perder a confiabilidade da informação temporal.

Nessas condições, as rotinas dependentes de horário não deverão ser executadas com base em uma referência desconhecida.

O comportamento após uma reinicialização também precisa ser definido, principalmente para evitar a repetição indevida de uma alimentação ou de uma operação que já ocorreu.

⸻

8. Módulo de manutenção e limpeza

O SmartBird deverá ajudar o responsável a acompanhar a necessidade de limpeza do sistema.

A proposta inicial estabelece um intervalo de aproximadamente 48 horas entre limpezas, com emissão de alerta quando esse prazo for atingido.

Esse intervalo é um parâmetro de projeto e deverá ser ajustado de acordo com a necessidade real de manutenção e as condições de uso.

8.1. Funcionamento do lembrete

O sistema deverá registrar o momento da última limpeza confirmada e comparar esse registro com o horário atual.

Quando o intervalo configurado for alcançado, o módulo deverá solicitar a atenção do usuário e sinalizar a necessidade de manutenção.

O alerta poderá utilizar os recursos locais, como LED e buzzer, e também ser apresentado na interface digital, se estiver disponível.

8.2. Confirmação da limpeza

A contagem não deverá ser reiniciada apenas porque o usuário visualizou o alerta.

O sistema deverá receber uma confirmação explícita de limpeza concluída. Essa confirmação poderá ser realizada por meio da interface física ou digital, conforme a implementação.

Após a confirmação, o programa deverá registrar o novo momento de referência e calcular o próximo intervalo.

8.3. Persistência

O registro da última limpeza não deverá existir apenas na memória temporária do programa.

Se o Arduino for reiniciado ou perder a alimentação, o sistema deverá preservar a informação necessária para continuar acompanhando o intervalo corretamente.

Isso exige uma estratégia de armazenamento persistente, a ser definida na implementação. O armazenamento também deverá considerar a validade dos dados gravados e a possibilidade de uma configuração inicial ainda não ter sido realizada.

8.4. Estados possíveis

A lógica poderá diferenciar:

* Limpeza dentro do prazo.
* Limpeza próxima do vencimento, caso exista um aviso antecipado.
* Limpeza pendente.
* Limpeza confirmada.
* Registro de tempo inválido ou indisponível.

O aviso antecipado é uma possibilidade de implementação, não um requisito já confirmado.

⸻

9. Interface física: LCD, botões, LEDs e buzzer

A interface física deverá permitir que o usuário consulte as informações essenciais e execute operações básicas sem precisar acessar a interface digital.

A proposta discutida considera um LCD 16x2, botões físicos, LEDs indicadores e um buzzer.

9.1. LCD 16x2

O LCD 16x2 possui duas linhas com 16 caracteres cada. Portanto, sua interface deverá ser planejada para apresentar informações curtas, e não reproduzir todas as informações disponíveis no painel digital.

A proposta é priorizar:

* Horário atual, quando disponível.
* Estado resumido dos reservatórios.
* Situação da alimentação.
* Situação da hidratação.
* Alertas relevantes.
* Estado da manutenção.
* Menus de configuração e ações essenciais.

Exemplos meramente ilustrativos de mensagens:

SmartBird 10:30

Agua: OK  Racao:BAIXA

Alimentacao: ativa

Limpeza pendente

Os textos definitivos precisarão caber no espaço disponível. Abreviações poderão ser necessárias, e o uso de acentos deverá considerar a fonte de caracteres suportada pelo display.

9.2. Botões físicos

Foi discutida a possibilidade de utilizar três botões para navegação e seleção.

Uma organização possível seria:

* Anterior: voltar ou percorrer as opções para cima.
* Próximo: avançar ou percorrer as opções para baixo.
* Selecionar: confirmar uma opção ou acessar uma ação.

Essa distribuição é uma proposta de interface, não uma decisão definitiva de hardware. A quantidade de botões deverá ser confirmada considerando os recursos disponíveis, a facilidade de uso e o número de ações que precisam ser acessíveis fisicamente.

O software deverá reconhecer os botões de maneira confiável, evitando que um único pressionamento seja interpretado como várias ações involuntárias.

9.3. LEDs e buzzer

Os LEDs e o buzzer poderão fornecer alertas simples, especialmente quando o usuário não estiver olhando para o LCD.

Os indicadores poderão representar condições como nível baixo, alerta de manutenção, operação em andamento ou falha.

A equipe deverá definir um padrão consistente para os sinais luminosos e sonoros. O mesmo padrão não deverá representar condições diferentes sem uma forma de distingui-las.

O alerta sonoro deverá ser utilizado de maneira controlada, considerando o ambiente em que a ave ficará.

9.4. Limites da interface física

O LCD não deverá tentar reproduzir o painel digital completo.

Sua função é disponibilizar as informações mais importantes para o uso cotidiano, além de permitir ações básicas. Configurações mais detalhadas, calibração dos reservatórios e diagnóstico poderão ser direcionados à interface digital, desde que isso não comprometa as funções essenciais quando o ESP32 estiver indisponível.

⸻

10. Interface digital e configuração pelo ESP32

A interface digital é uma expansão importante da arquitetura prevista para o SmartBird.

Seu objetivo é tornar o sistema mais fácil de acompanhar e configurar, especialmente quando a quantidade de informações exceder o espaço disponível no LCD.

10.1. Informações apresentadas

O painel digital poderá reunir informações como:

* Estado geral do sistema.
* Nível estimado de alimento.
* Nível estimado de água.
* Estado dos sensores.
* Horários programados.
* Última alimentação registrada, se esse dado for implementado.
* Estado da bomba e do servo.
* Situação da manutenção.
* Alertas ativos.
* Falhas de leitura.
* Última atualização dos dados recebidos do Arduino.

As informações deverão deixar claro quando um dado estiver desatualizado ou inválido. Uma tela que exibe o último nível recebido sem indicar que a comunicação foi perdida pode levar o usuário a interpretar uma informação antiga como atual.

10.2. Parâmetros configuráveis

A interface poderá permitir a alteração de parâmetros como:

* Horários de alimentação.
* Horários de hidratação, se adotados.
* Altura útil dos reservatórios.
* Distância de referência dos sensores.
* Limites de nível baixo.
* Limites de nível alto, quando aplicáveis.
* Intervalo de manutenção.
* Limites de operação dos atuadores, quando esses parâmetros forem configuráveis pelo usuário.

A lista definitiva dependerá das decisões da equipe. Nem todos os parâmetros técnicos precisam ser expostos ao usuário comum.

10.3. Validação e armazenamento

Toda configuração recebida deverá ser validada antes de ser aplicada.

Por exemplo, a altura útil de um reservatório não poderá ser zero ou negativa. Um limite percentual deverá estar dentro da faixa aceita pelo programa. Um horário deverá ter valores válidos.

Também será necessário decidir onde cada configuração ficará armazenada e qual controlador será responsável pela sua persistência.

Uma possibilidade é o ESP32 oferecer o formulário de configuração e encaminhar os valores ao Arduino, que continuará responsável por aplicar as regras operacionais locais. Outra possibilidade é distribuir parte do armazenamento entre as placas.

A escolha precisa ser documentada para evitar que as duas placas mantenham configurações divergentes.

10.4. Funcionamento sem conexão

A interface digital não deverá ser um requisito para o funcionamento básico da alimentação, da hidratação ou dos alertas locais.

Se o ESP32 reiniciar ou a conexão Wi-Fi ficar indisponível, o Arduino deverá continuar executando as funções locais que estiverem em condições de operar.

Da mesma forma, comandos enviados pela interface digital não deverão ser executados automaticamente apenas por terem sido recebidos. O Arduino deverá validar o comando, verificar as condições de segurança e informar se ele foi aceito ou recusado.

⸻

11. Organização do código e modularização

O código deverá ser organizado em módulos com responsabilidades claras. Essa organização é especialmente importante porque o projeto será desenvolvido em equipe.

A intenção é evitar que toda a lógica seja escrita em um único arquivo, dificultando os testes, a leitura e a integração das funcionalidades.

A estrutura abaixo é uma proposta de organização para o firmware do Arduino. Os nomes podem ser ajustados, desde que as responsabilidades permaneçam bem definidas.

11.1. SmartBird_Arduino.ino

Será o ponto principal de entrada do programa.

Deverá conter a inicialização geral, a configuração dos módulos e o ciclo principal de execução.

O setup() ficará responsável por inicializar os recursos necessários. O loop() coordenará a atualização dos módulos sem concentrar nele todos os detalhes internos de cada funcionalidade.

O arquivo principal não deverá se tornar um conjunto extenso de regras específicas de alimentação, hidratação ou interface. Ele deverá coordenar os módulos e preservar uma visão clara do fluxo geral.

11.2. EstadoSmartBird.h

Deverá reunir estruturas e tipos compartilhados pelos módulos.

Poderá conter definições como:

* Estado dos reservatórios.
* Valores de sensores.
* Indicadores de validade das leituras.
* Estado dos atuadores.
* Estado da alimentação.
* Estado da hidratação.
* Estado da manutenção.
* Configurações do sistema.
* Alertas e códigos de falha.

A vantagem de manter essas estruturas em um local comum é reduzir a duplicação de definições e facilitar a comunicação entre os módulos.

Esse arquivo deverá ser acordado antes de a equipe desenvolver todas as funcionalidades em paralelo.

11.3. ControleSmartBird.h/.cpp

É uma camada opcional de coordenação.

Caso seja útil, poderá centralizar a atualização do estado geral, a coordenação entre módulos e as regras que envolvem mais de uma funcionalidade.

Por exemplo, poderá organizar a sequência entre leitura dos reservatórios, verificação de agendamentos e atualização dos estados exibidos na interface.

A criação desse módulo não é obrigatória se a equipe conseguir manter o programa principal simples e organizado sem ele.

11.4. Reservatorios.h/.cpp

Será responsável por:

* Inicializar os sensores ultrassônicos.
* Realizar as leituras de distância.
* Identificar leituras inválidas.
* Aplicar a calibração de cada reservatório.
* Calcular a altura estimada do conteúdo.
* Calcular o nível percentual, quando aplicável.
* Classificar o estado de cada reservatório.
* Disponibilizar os resultados aos outros módulos.

Esse módulo não deverá decidir sozinho quando alimentar a ave ou ligar a bomba. Sua responsabilidade será fornecer informações confiáveis sobre os reservatórios.

11.5. Tempo.h/.cpp

Será responsável por:

* Inicializar e consultar o DS3231.
* Fornecer data e hora aos demais módulos.
* Identificar problemas na leitura do RTC.
* Apoiar o controle de horários programados.
* Disponibilizar informações temporais necessárias à manutenção.

O módulo de tempo não deverá controlar diretamente os atuadores.

11.6. Alimentacao.h/.cpp

Será responsável por:

* Interpretar as solicitações de alimentação.
* Verificar as condições para executar a tarefa.
* Consultar o estado do reservatório.
* Utilizar a medição de peso, caso implementada.
* Controlar a sequência do servo.
* Impedir operações incompatíveis com as regras de segurança.
* Informar o estado da operação.
* Registrar falhas ou recusas relevantes.

11.7. Hidratacao.h/.cpp

Será responsável por:

* Interpretar as solicitações de hidratação.
* Verificar o nível de água.
* Consultar a validade das leituras.
* Controlar a bomba.
* Interromper a operação quando a condição de parada for alcançada.
* Aplicar o limite máximo de tempo de funcionamento.
* Informar estados e falhas.

11.8. Interface.h/.cpp

Será responsável pela interface física.

Deverá controlar o LCD, ler os botões, gerenciar os LEDs e emitir os sinais do buzzer.

Também poderá organizar a navegação dos menus e converter uma ação física em uma solicitação para o módulo correspondente.

A interface não deverá controlar diretamente o servo ou a bomba. Por exemplo, ao selecionar “Alimentar”, deverá solicitar a operação ao módulo de alimentação, que então verificará se a execução é permitida.

11.9. Manutencao.h/.cpp

Será responsável por:

* Consultar o horário atual.
* Calcular o intervalo desde a última limpeza confirmada.
* Identificar quando a manutenção está pendente.
* Solicitar alertas à interface.
* Receber a confirmação de limpeza.
* Atualizar o registro persistente.
* Tratar registros ausentes ou inválidos.

11.10. Comunicacao.h/.cpp

Será responsável pela camada de transporte entre o Arduino e o ESP32.

Deverá gerenciar:

* Inicialização da serial dedicada.
* Recepção de mensagens.
* Acúmulo de dados recebidos.
* Identificação do fim de cada mensagem.
* Transmissão de respostas e atualizações.
* Tratamento de mensagens incompletas.
* Limites do buffer de recepção.
* Estado básico da comunicação.

A comunicação deverá ser executada sem impedir que os outros módulos continuem funcionando.

11.11. Protocolo.h/.cpp

Será responsável pela interpretação das mensagens estruturadas, utilizando JSON como formato proposto.

Deverá:

* Interpretar mensagens recebidas.
* Verificar se os campos obrigatórios existem.
* Validar os tipos dos dados.
* Reconhecer os comandos permitidos.
* Converter os dados recebidos em estruturas internas tipadas.
* Gerar mensagens de resposta.
* Tratar mensagens malformadas ou desconhecidas.

É importante separar o transporte da interpretação. O módulo Comunicacao recebe e envia os bytes; o módulo Protocolo interpreta o significado da mensagem.

O protocolo não deverá executar diretamente os atuadores. Ele deverá produzir uma solicitação que será validada e encaminhada ao módulo responsável.

11.12. Arquivos auxiliares e bibliotecas

Dependendo da implementação, a equipe poderá precisar de bibliotecas para o DS3231, o LCD, o servo, o HX711 e a interpretação de JSON, como a ArduinoJson.

A escolha definitiva deverá considerar a compatibilidade com as placas e versões utilizadas.

O uso de bibliotecas não elimina a necessidade de entender suas responsabilidades, inicialização e limites de operação.

⸻

12. Comunicação por JSON entre Arduino e ESP32

A proposta é utilizar mensagens JSON transmitidas pela UART, com uma mensagem por linha e o caractere de nova linha como delimitador.

Essa estratégia facilita a identificação das mensagens e a depuração durante o desenvolvimento.

12.1. Exemplo de comando

Um pedido de alimentação manual poderia utilizar uma mensagem semelhante a esta:

{"type":"command","id":12,"name":"feed_manual"}

Nesse exemplo:

* type identifica a categoria da mensagem.
* id permite associar uma resposta à solicitação original.
* name identifica o comando solicitado.

O exemplo representa o formato conceitual. A estrutura definitiva deverá ser documentada e adotada igualmente pelas duas placas.

12.2. Tipos de mensagem previstos

O protocolo poderá utilizar as seguintes categorias:

command: solicita uma ação ou alteração de configuração.

ack: informa que uma mensagem foi recebida ou que a solicitação foi aceita, conforme o significado definido para esse tipo.

response: apresenta o resultado de uma solicitação, como uma consulta ou uma configuração.

telemetry: transmite informações de estado, como níveis dos reservatórios e condições operacionais.

event: comunica uma ocorrência relevante, como início de operação, manutenção pendente ou mudança de estado.

error: informa uma falha de interpretação, validação, comunicação ou execução.

A equipe deverá definir claramente o significado de cada resposta. Receber uma mensagem, aceitar um comando e concluir uma operação são eventos diferentes.

12.3. Recepção e validação

A recepção deverá acumular os caracteres até identificar o delimitador da mensagem. O conteúdo completo será então enviado ao interpretador JSON.

O sistema deverá lidar com:

* Mensagens incompletas.
* Mensagens excessivamente longas.
* JSON malformado.
* Campos ausentes.
* Tipos incorretos.
* Comandos desconhecidos.
* Valores fora dos limites permitidos.
* Mensagens repetidas.
* Respostas que não correspondam a uma solicitação conhecida.

O buffer deverá ter um limite definido, evitando que uma mensagem malformada consuma memória indefinidamente.

12.4. Segurança lógica dos comandos

O JSON não é uma autorização para executar uma ação. Ele é apenas uma representação dos dados transmitidos.

Depois de interpretar a mensagem, o programa deverá verificar se o comando é permitido e se as condições operacionais estão adequadas.

Um pedido de alimentação, por exemplo, deverá passar pelas regras do módulo de alimentação. Um pedido de acionamento da bomba deverá passar pelas regras do módulo de hidratação.

12.5. Atualizações de estado

O Arduino deverá enviar informações suficientes para que a interface digital apresente o estado atual do sistema.

A equipe precisará definir quais dados serão transmitidos, com qual frequência e em quais situações uma atualização será enviada imediatamente.

Também será necessário definir como o ESP32 identificará dados desatualizados. A interface deverá distinguir uma informação válida e recente de uma última leitura que não pôde ser atualizada.

⸻

13. Fluxo integrado de funcionamento

A arquitetura pode ser compreendida como uma sequência de atualização de dados, tomada de decisão e execução de ações.

13.1. Inicialização

Quando o sistema for ligado, o Arduino deverá inicializar os módulos necessários, verificar a disponibilidade do RTC e preparar os sensores e atuadores.

As leituras iniciais deverão ser classificadas como válidas, inválidas ou ainda indisponíveis. O sistema não deverá assumir que todos os sensores já forneceram informações confiáveis imediatamente após a inicialização.

Os atuadores deverão começar em estados seguros, evitando que a inicialização acione involuntariamente o servo ou a bomba.

13.2. Atualização dos sensores

Durante a execução, o sistema deverá consultar periodicamente os sensores de reservatório e, se implementada, a medição de peso.

As informações deverão ser atualizadas nas estruturas compartilhadas, acompanhadas de indicadores de validade.

13.3. Avaliação das regras

Os módulos de alimentação e hidratação deverão consultar as condições necessárias à execução de suas rotinas.

O módulo de tempo fornecerá a referência temporal, enquanto o módulo de manutenção verificará o intervalo de limpeza.

As solicitações manuais recebidas pela interface física ou digital deverão passar pelas mesmas validações operacionais.

13.4. Execução dos atuadores

Quando uma operação for autorizada, o módulo responsável deverá executar sua sequência de controle.

O sistema deverá acompanhar a duração da operação, verificar as condições de parada e registrar o resultado.

Uma falha não deverá ser ocultada apenas porque o comando inicial foi aceito.

13.5. Atualização das interfaces

A interface física deverá apresentar um resumo do estado do sistema.

A interface digital poderá receber atualizações mais detalhadas, apresentar configurações e disponibilizar informações de diagnóstico.

As duas interfaces deverão representar o mesmo estado lógico do sistema, mesmo que mostrem níveis diferentes de detalhe.

⸻

14. Execução não bloqueante

O programa principal deverá evitar o uso excessivo de delay() ou de outras rotinas que interrompam a execução de todo o sistema durante períodos prolongados.

Se o Arduino permanecer bloqueado enquanto movimenta o servo, por exemplo, poderá deixar de atualizar os sensores, verificar o estado da bomba, processar os botões ou receber mensagens do ESP32.

A alternativa é organizar as operações em etapas e utilizar referências de tempo para determinar quando cada etapa deverá ser executada.

O princípio geral é que cada módulo realize sua atualização e devolva o controle ao programa principal rapidamente.

Isso não significa que todas as tarefas precisam ocorrer na mesma frequência. A leitura de um sensor, a atualização do LCD e a transmissão de dados podem ter intervalos diferentes, desde que a arquitetura preserve a capacidade de responder às condições relevantes.

A execução não bloqueante também facilita o tratamento de falhas, porque permite verificar limites de tempo e interromper operações quando necessário.

⸻

15. Alimentação elétrica e montagem física

A arquitetura eletrônica deverá considerar a diferença entre os circuitos de controle e os circuitos de potência.

O Arduino não deve fornecer diretamente a corrente necessária para alimentar componentes como uma bomba ou um servo que exija corrente elevada.

15.1. Alimentação dos atuadores

A fonte e o circuito de acionamento deverão ser escolhidos com base nas especificações dos componentes efetivamente adquiridos.

A bomba poderá precisar de um driver, transistor, módulo apropriado ou outro circuito de acionamento compatível. O servo também deverá receber alimentação adequada à sua corrente de operação.

O dimensionamento definitivo depende dos modelos selecionados. Não é possível determinar com segurança a fonte final sem conhecer as tensões e correntes dos componentes.

15.2. Referência elétrica

Quando forem utilizados circuitos alimentados separadamente, as referências de GND deverão ser organizadas de acordo com o circuito de controle e com os requisitos dos módulos.

O compartilhamento de GND entre o Arduino e o ESP32 será necessário para a UART proposta. A montagem deverá seguir o esquema elétrico definitivo, evitando ligações improvisadas.

15.3. Proteção contra água

Como o projeto trabalha com água, a disposição física dos componentes deverá impedir que respingos, vazamentos ou condensação atinjam a eletrônica.

A placa de controle, as conexões, a fonte e os circuitos de potência deverão ficar protegidos e afastados das áreas de armazenamento e circulação de água.

A montagem física deverá permitir inspeção, manutenção e acesso aos reservatórios sem exigir que a eletrônica seja exposta desnecessariamente.

15.4. Estrutura do protótipo

Uma proposta discutida anteriormente considera uma estrutura em painel de MDF, com possibilidade de utilizar peças produzidas em impressão 3D.

Essa construção deverá ser validada de acordo com as dimensões dos recipientes, o posicionamento dos sensores e o mecanismo de alimentação.

A montagem precisa garantir que os sensores ultrassônicos tenham um caminho de medição adequado, sem interferências físicas que prejudiquem a leitura.

A estrutura mecânica é parte essencial do sistema: uma lógica correta não compensa um sensor mal posicionado ou um mecanismo que trava fisicamente.

⸻

16. Organização do trabalho em equipe e Git

Como o projeto será desenvolvido por uma equipe, o trabalho deverá ser dividido por funcionalidades e responsabilidades, evitando que todos alterem simultaneamente os mesmos arquivos.

A proposta é utilizar o GitHub e um fluxo de branches por funcionalidade.

16.1. Branch principal

A branch main deverá representar a versão integrada e considerada estável do projeto.

Ela não deverá ser utilizada como espaço de experimentação contínua. As alterações deverão ser desenvolvidas em branches próprias e integradas após revisão e testes.

16.2. Branches sugeridas

Uma divisão inicial possível é:

* feature/reservatorios
* feature/alimentacao
* feature/hidratacao
* feature/interface-lcd
* feature/tempo-manutencao
* feature/comunicacao
* feature/painel-web

A branch feature/comunicacao poderá reunir UART e JSON inicialmente. Caso a equipe perceba que são trabalhos independentes, poderá separar o transporte serial e a implementação do protocolo.

A branch feature/painel-web faz sentido se a interface digital tiver um desenvolvimento separado do firmware principal.

Essas branches são uma sugestão de organização, não uma obrigação de manter todas abertas simultaneamente. A quantidade ativa deverá acompanhar o número de tarefas realmente em andamento.

16.3. Fluxo de trabalho

O fluxo recomendado é:

1. Atualizar a branch principal.
2. Criar uma branch para uma funcionalidade.
3. Desenvolver e testar a alteração isoladamente.
4. Registrar commits com descrições claras.
5. Abrir um pull request.
6. Revisar a alteração e verificar a compatibilidade com os módulos existentes.
7. Executar os testes de integração necessários.
8. Integrar a alteração à main.
9. Excluir a branch concluída, quando apropriado.

16.4. Contratos entre módulos

Antes de a equipe desenvolver todas as funcionalidades em paralelo, é importante definir as estruturas compartilhadas e as interfaces entre módulos.

O arquivo EstadoSmartBird.h e o formato do protocolo JSON são exemplos de contratos que precisam ser acordados.

Se uma pessoa desenvolver o módulo de alimentação esperando receber um valor percentual e outra entregar um estado textual sem definir a conversão, a integração exigirá retrabalho.

O mesmo problema ocorre quando um módulo espera controlar diretamente o sensor e outro considera que a leitura será feita por uma camada central.

Por isso, a equipe deverá definir previamente:

* Quais dados cada módulo recebe.
* Quais dados cada módulo produz.
* Quais funções cada módulo disponibiliza.
* Como os estados de falha são representados.
* Como as configurações são armazenadas e compartilhadas.
* Como os comandos são solicitados e respondidos.

⸻

17. Estratégia de desenvolvimento e testes

O desenvolvimento deverá ocorrer progressivamente. É preferível validar pequenos conjuntos de componentes antes de integrar todas as funções de uma vez.

17.1. Etapa 1 — definição técnica

Antes da montagem definitiva, a equipe deverá definir os componentes, as dimensões dos reservatórios, a quantidade de sensores, os mecanismos físicos e as responsabilidades de cada placa.

Também deverá elaborar o mapa de pinos, o esquema elétrico e os contratos entre os módulos de software.

17.2. Etapa 2 — testes individuais

Cada componente deverá ser testado separadamente.

Exemplos:

* Confirmar a leitura de distância dos sensores ultrassônicos.
* Calibrar a célula de carga e o HX711, caso sejam utilizados.
* Testar os ângulos e movimentos do servo.
* Testar a bomba e seu circuito de acionamento.
* Verificar a comunicação com o DS3231.
* Exibir informações básicas no LCD.
* Ler os botões físicos.
* Testar LEDs e buzzer.
* Verificar a UART entre Arduino e ESP32.

17.3. Etapa 3 — desenvolvimento modular

Cada módulo deverá implementar sua responsabilidade e ser testado com dados controlados.

Quando um componente físico ainda não estiver disponível, a equipe poderá utilizar valores simulados para desenvolver a lógica. Esses dados deverão estar claramente identificados como simulados para não serem confundidos com medições reais.

17.4. Etapa 4 — integração do firmware

Após os testes individuais, os módulos deverão ser integrados ao programa principal.

A equipe deverá verificar se a atualização de um módulo não bloqueia os demais e se as estruturas compartilhadas estão sendo utilizadas de maneira consistente.

17.5. Etapa 5 — integração com o ESP32

A comunicação deverá ser testada primeiro com mensagens simples.

Em seguida, poderão ser implementados o envio de telemetria, a consulta de estados, o tratamento de comandos, as respostas e as mensagens de erro.

A integração deverá ser testada tanto em condições normais quanto com mensagens malformadas e interrupções de comunicação.

17.6. Etapa 6 — testes integrados

O sistema deverá ser submetido a cenários como:

* Alimentação automática no horário previsto.
* Solicitação manual de alimentação.
* Nível baixo de alimento.
* Nível baixo de água.
* Solicitação manual de hidratação.
* Atingimento do nível de parada.
* Falha de leitura de um sensor.
* Perda de comunicação com o ESP32.
* Reinicialização do Arduino.
* Reinicialização do ESP32.
* Alerta de limpeza vencido.
* Confirmação de limpeza.
* Recuperação após uma falha.

A equipe deverá registrar os resultados, os problemas encontrados e as correções realizadas.

17.7. Etapa 7 — validação física e demonstração

Antes da apresentação, o protótipo deverá ser testado repetidamente nas condições em que será demonstrado.

É importante verificar a estabilidade mecânica, a fixação dos sensores, a alimentação elétrica e o comportamento dos atuadores.

Os testes deverão começar sem uma ave exposta a riscos, utilizando condições controladas. O protótipo somente deverá ser considerado apto para uso real após avaliação das condições de segurança, higiene, confiabilidade e bem-estar animal.

⸻

18. Tratamento de falhas e comportamento seguro

Um sistema automatizado não pode considerar que todos os componentes funcionarão corretamente o tempo todo.

Por isso, o SmartBird deverá distinguir entre uma condição normal, uma condição que exige atenção e uma falha que impede a execução de determinada operação.

18.1. Falha de sensor

Se uma leitura for inválida, o sistema deverá registrar essa condição e impedir que ela seja interpretada como um nível real.

A reação dependerá da função afetada. Uma leitura inválida do reservatório de água, por exemplo, poderá impedir o acionamento automático da bomba até que a condição seja verificada.

18.2. Falha de comunicação

Se o ESP32 parar de se comunicar com o Arduino, a interface digital deverá indicar que os dados não estão sendo atualizados.

O Arduino deverá continuar executando as funções locais que não dependam dessa comunicação. Comandos incompletos ou recebidos em condições inválidas não deverão ser executados.

18.3. Falha de atuador

A ausência de uma resposta esperada do atuador deverá levar a uma condição de erro, quando for possível detectar essa situação.

O sistema deverá evitar tentativas indefinidas de operação. Os limites de tempo e o comportamento após a falha deverão ser definidos para cada módulo.

18.4. Reinicialização

Após uma reinicialização, o programa deverá reavaliar os sensores e as configurações antes de permitir operações que dependam dessas informações.

Também deverá evitar a repetição involuntária de tarefas programadas que já tenham sido executadas.

18.5. Segurança do animal

O sistema deverá ser projetado para evitar riscos físicos, elétricos e operacionais à ave.

A automação não deverá substituir a observação do responsável, a higienização regular ou os cuidados necessários à espécie.

A disponibilidade de água e alimento, os limites de operação e a confiabilidade do protótipo precisam ser avaliados considerando o uso real, e não apenas o sucesso de uma demonstração.

⸻

19. Decisões pendentes e pontos que exigem validação

O projeto possui uma arquitetura conceitual definida, mas algumas escolhas ainda precisam ser confirmadas antes da implementação final.

Tema	Situação atual	O que precisa ser definido
Controlador principal	Arduino Mega 2560 é a proposta de referência	Confirmar a placa efetivamente utilizada
Controlador digital	ESP32 previsto para comunicação e interface	Confirmar modelo e escopo da interface
Sensores ultrassônicos	Propostos para monitorar os reservatórios	Quantidade, posicionamento e modelo
Reservatórios	Monitoramento de alimento e água previsto	Dimensões, formato e altura útil
Calibração	Distância de referência e altura útil necessárias	Como calibrar e armazenar cada parâmetro
Medição de peso	Célula de carga e HX711 considerados	Local da medição e função exata no controle
Alimentação	Servo motor previsto	Mecanismo físico, ângulos e limites de operação
Hidratação	Bomba e monitoramento de nível previstos	Circuito hidráulico, sensor e condição de parada
RTC	DS3231 proposto	Validação da leitura e tratamento de falhas
LCD	LCD 16x2 proposto	Organização final das telas
Botões	Três botões discutidos como possibilidade	Quantidade e funções definitivas
Interface digital	Prevista na arquitetura com ESP32	Tecnologia, telas e funções prioritárias
Persistência	Necessária para configurações e limpeza	Local de armazenamento e formato dos dados
Comunicação	UART e JSON propostos	Esquema definitivo das mensagens e respostas
Alimentação elétrica	Fontes e circuitos adequados necessários	Dimensionamento após escolher os componentes
Estrutura física	Painel de MDF e peças 3D considerados	Dimensões, fixações e proteção contra água
Intervalo de limpeza	Aproximadamente 48 horas na proposta inicial	Valor final e critérios de confirmação
Programação	Desenvolvimento modular proposto	Interfaces e contratos finais entre módulos

Esses itens não representam necessariamente problemas do projeto. São decisões que precisam ser tomadas de maneira organizada para que o desenvolvimento do software acompanhe a montagem real.

⸻

20. Organização dos documentos técnicos

Para facilitar o trabalho da equipe, a documentação poderá ser dividida em três documentos complementares.

20.1. Documento de visão geral e arquitetura

Este documento deverá explicar:

* Objetivo e escopo.
* Funcionamento geral.
* Responsabilidades do Arduino e do ESP32.
* Módulos de software.
* Fluxos de funcionamento.
* Comunicação entre placas.
* Interface física e digital.
* Tratamento de falhas.
* Decisões pendentes.

O presente documento pertence a essa categoria, funcionando como referência geral do projeto.

20.2. Documento de ligações elétricas

Deverá apresentar o esquema elétrico do protótipo, incluindo:

* Alimentação das placas.
* Sensores.
* Servo motor.
* Bomba e circuito de acionamento.
* LCD.
* Botões.
* LEDs.
* Buzzer.
* RTC.
* HX711 e célula de carga, se utilizados.
* UART entre Arduino e ESP32.
* Adaptação de nível lógico.
* Referências de GND e fontes.

O esquema definitivo deverá ser elaborado somente depois da confirmação dos modelos e das especificações elétricas.

20.3. Documento de mapa de pinos e alimentação

Deverá apresentar uma tabela com cada componente, os pinos utilizados, a tensão de operação, a corrente exigida quando relevante e a origem da alimentação.

Também deverá indicar eventuais conflitos de pinos e as interfaces utilizadas, como I²C, UART e pinos digitais.

O RTC DS3231 e um LCD com interface I²C poderão compartilhar o barramento I²C, desde que os endereços e a montagem sejam compatíveis. A pinagem final deverá ser validada na placa e nos módulos adquiridos.

⸻

21. Apresentação acadêmica e demonstração do protótipo

O SmartBird também precisa ser apresentado de forma que a banca compreenda o problema, a solução, a viabilidade técnica e o potencial de evolução.

O projeto foi preparado para uma apresentação curta, com referência anterior a um pitch de aproximadamente cinco minutos. O material de preparação da equipe deve ser mais completo do que o discurso efetivamente apresentado.

21.1. O que a equipe precisa dominar

Cada integrante deverá conseguir explicar:

* Qual problema o SmartBird pretende resolver.
* Como funciona a alimentação automática.
* Como funciona a hidratação.
* Como os sensores estimam os níveis dos reservatórios.
* Por que o Arduino Mega foi considerado para o controle local.
* Qual é a função do ESP32.
* Como as placas se comunicam.
* Como o sistema identifica falhas.
* Como as configurações serão feitas.
* Como o sistema se comporta sem a interface digital.
* Quais limitações ainda existem.
* Quais são as próximas etapas do desenvolvimento.

21.2. Viabilidade técnica

A viabilidade deverá ser demonstrada pela divisão do sistema em módulos, pelo uso de componentes compatíveis com o escopo acadêmico e por testes progressivos.

A equipe deverá evitar afirmar que uma funcionalidade está concluída antes de ela ter sido implementada e testada.

21.3. Demonstração

Uma demonstração eficiente poderá apresentar, conforme o estágio real do protótipo:

1. A leitura dos níveis dos reservatórios.
2. Uma mudança de estado provocada por uma alteração controlada do nível.
3. Uma solicitação de alimentação.
4. Uma solicitação de hidratação.
5. A atualização do LCD.
6. O envio de dados para a interface digital.
7. Um alerta de manutenção ou de falha simulada.

As funcionalidades demonstradas deverão corresponder ao que estiver realmente implementado. Se uma parte ainda estiver simulada, isso deverá ser explicado.

21.4. Perguntas que a equipe deve estar preparada para responder

A equipe deverá refletir sobre questões como:

* O que acontece se o sensor ultrassônico fornecer uma leitura inválida?
* Como o sistema evita que a bomba continue funcionando indefinidamente?
* O que acontece se o ESP32 perder a conexão com o Arduino?
* Como o sistema preserva o registro da última limpeza após uma reinicialização?
* Como a equipe verificará se a quantidade de alimento liberada está adequada?
* Como será possível alterar a altura útil e os limites dos reservatórios?
* Quais partes já estão implementadas e quais ainda são propostas?
* Como o projeto será validado para evitar riscos à ave?

Também é importante preparar dúvidas legítimas da própria equipe sobre os requisitos, os critérios de avaliação, o nível de automação esperado e as exigências para a versão final. Essas dúvidas deverão ser formuladas a partir das orientações reais da disciplina e da banca.

⸻

22. Possibilidades de evolução

A arquitetura proposta permite ampliar o projeto futuramente sem precisar substituir toda a lógica local.

A presença do ESP32 abre espaço para uma interface digital mais completa, enquanto a modularização permite acrescentar funcionalidades sem concentrar todo o código em um único arquivo.

Entre as possibilidades de evolução estão:

* Interface digital com histórico de estados.
* Configuração mais completa dos horários.
* Diagnóstico mais detalhado dos sensores.
* Registro de operações e falhas.
* Melhorias na calibração dos reservatórios.
* Expansão das notificações.
* Novos sensores, caso exista uma necessidade funcional justificada.

Essas possibilidades representam caminhos de evolução, não funcionalidades já garantidas para a primeira versão.

A prioridade do protótipo deverá ser implementar corretamente o conjunto essencial: leitura dos sensores, controle dos atuadores, agendamento, interface física, manutenção e comunicação prevista.

⸻

23. Conclusão

O SmartBird é uma proposta de sistema embarcado que combina automação, monitoramento e interface para auxiliar no manejo de aves domésticas.

A arquitetura proposta utiliza o Arduino Mega 2560 como controlador principal das funções locais e o ESP32 como plataforma complementar para comunicação e interface digital. A organização em módulos permite separar a leitura dos reservatórios, o controle da alimentação, a hidratação, o relógio, a manutenção, a interface e a comunicação.

O monitoramento ultrassônico exige calibração e tratamento adequado das leituras. A alimentação e a hidratação precisam de regras claras para autorizar, executar e interromper operações. O lembrete de limpeza depende de um registro temporal persistente e de confirmação explícita do usuário. A interface física deverá permanecer útil mesmo sem a interface digital.

A qualidade do projeto dependerá não apenas de fazer os componentes funcionarem isoladamente, mas também de garantir que trabalhem de forma coordenada e previsível. Por isso, a definição dos contratos entre módulos, a organização do código, os testes progressivos e o tratamento de falhas são partes tão importantes quanto a montagem eletrônica.

O próximo passo é transformar esta arquitetura em especificações concretas: confirmar os componentes, definir as dimensões dos reservatórios, estabelecer a pinagem, documentar o protocolo de comunicação e acordar as interfaces de software. Com essas decisões tomadas, a equipe poderá desenvolver cada módulo separadamente e integrá-los de forma gradual, mantendo uma referência comum para o funcionamento completo do SmartBird.
