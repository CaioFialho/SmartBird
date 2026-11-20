# SmartBird — Firmware Arduino revisado

Este pacote contém a revisão dos módulos de firmware enviados para análise. A revisão prioriza consistência entre estruturas de dados, módulos físicos, agendamentos, controle central e protocolo JSON.

## Arquivos

- `Sistema_SmartBird_Arduino.ino`: ponto de entrada do Arduino.
- `Estado_SB.h`: estados, configurações e dados compartilhados.
- `Hardware_SB.h`: mapeamento dos pinos.
- `Alimentacao.cpp/.h`: HX711 e servo da comporta.
- `Hidratacao.cpp/.h`: boia e relé da bomba.
- `Reservatorios.cpp/.h`: sensores ultrassônicos dos reservatórios de origem.
- `Tempo.cpp/.h`: RTC e agendas.
- `Manutencao.cpp/.h`: lembrete de limpeza.
- `Comunicacao.cpp/.h`: transporte serial com mensagens terminadas em LF.
- `Protocolo.cpp/.h`: interpretação das mensagens JSON.
- `Interface.cpp/.h`: LCD, navegação, botões, LEDs e buzzer.
- `Controle_SB.cpp/.h`: integração e ciclo principal.

## Interface física implementada

- LCD 16x2 I2C no endereço padrão `0x27` (SDA=20 e SCL=21 no Mega). Se o módulo usar `0x3F`, alterar o construtor do LCD em `Interface.cpp`.
- Botões com `INPUT_PULLUP`: anterior=31, OK=32, próximo=33. Cada botão deve fechar contato com GND quando pressionado.
- LEDs: sistema=34, ação=35, aviso=36, crítico=37. A ligação deve incluir resistor limitador em série com cada LED.
- Buzzer no pino 38; terminal positivo no pino e negativo no GND. O código usa `tone()`; confirmar o tipo e a corrente do componente antes de ligar diretamente. Se necessário, usar transistor.
- Os pinos 20/21 são compartilhados no barramento I2C entre LCD e RTC.

A navegação principal percorre Home, Reservatórios, Potinhos, Próxima Ação, Manutenção e Conectividade. Na tela Próxima Ação, OK abre a edição das agendas; na tela Manutenção, OK abre a confirmação da limpeza. Na edição, `>` percorre os quatro espaços de horário, `<` volta entre etapas e ajusta o horário em passos de 30 minutos, e OK confirma. O LCD mostra um espaço de horário por vez para caber no formato 16x2.

Os estados Wi-Fi e MQTT aparecem como `--` por enquanto: a ponte ESP8266 e o protocolo atualmente integrados não enviam esses dois estados ao Mega. Não exibir `OK` sem receber essa confirmação real.

## Dependências

Instalar no Arduino IDE:
- ArduinoJson **7.x**
- HX711 (API compatível com a biblioteca bogde/HX711)
- Servo (biblioteca padrão do Arduino)
- RTClib (Adafruit)
- LiquidCrystal_I2C (biblioteca compatível com `LiquidCrystal_I2C lcd(0x27, 16, 2)`)
- Wire (biblioteca padrão)

Placa selecionada: **Arduino Mega 2560**.

## Comunicação

Em `Comunicacao.h`:
- `SMARTBIRD_COMUNICACAO_USB 1`: teste pelo Monitor Serial a 115200 baud.
- `SMARTBIRD_COMUNICACAO_USB 0`: UART `Serial1` para ESP8266 a 9600 baud.

Quando a comunicação USB estiver selecionada, não envie mensagens de depuração pela `Serial`, pois ela também transporta o JSON. O firmware revisado evita logs nessa porta.

Para `Serial1` no Mega:
- TX1: pino 18
- RX1: pino 19
- GND comum com o ESP8266
- O TX de 5 V do Mega deve passar por divisor/level shifter antes do RX de 3,3 V do ESP8266.
- Não alimente o ESP8266 diretamente pelo pino 3,3 V do Mega sem verificar a capacidade de corrente da fonte.

## Regras de hardware assumidas

- Boia do bebedouro: `INPUT_PULLUP`; LOW = nível baixo; HIGH = nível adequado.
- Relé da bomba: HIGH liga e LOW desliga. Se o módulo de relé utilizado for ativo em LOW, alterar `ligarBomba()` e `desligarBomba()` em `Hidratacao.cpp`.
- Servo: 0° fechado e 90° aberto. Ajustar após testar mecanicamente a comporta.
- Os limites de distância dos reservatórios configurados em `Controle_SB.cpp` são valores iniciais e precisam ser calibrados no equipamento real.
- O fator HX711 `-1032.96` é um valor provisório vindo do código recebido; deve ser validado com pesos conhecidos.

## Comportamento implementado

- A alimentação abre a comporta até atingir o peso-alvo ou o tempo máximo.
- O módulo de hidratação inicia automaticamente quando a boia indica nível baixo e o reservatório de origem tem leitura válida e nível suficiente.
- A bomba desliga quando a boia indica nível adequado ou quando o tempo máximo é excedido.
- O agendamento identifica ocorrências e o controle central encaminha as pendências aos módulos.
- Agendamentos que terminam em erro são marcados como recusados; não são repetidos indefinidamente. O agendador ignora saltos de relógio superiores a 120 segundos para evitar executar em sequência horários antigos após sincronização do RTC.
- O comando `set_feeding_config` aceita `target_weight_g` e `max_duration_ms`; os limites implementados são 1–500 g e 1.000–120.000 ms. O peso-alvo inicia em zero, então a alimentação deve ser configurada antes do primeiro acionamento.
- `set_watering_config` permanece explicitamente não suportado pelo protocolo atual, conforme a especificação v0.2.2, que deixa essa configuração pendente de definição.
- `sync_time` espera `YYYY-MM-DDTHH:MM:SS±HH:MM`. O RTC armazena os componentes de horário local enviados pela Web; o offset é validado, mas não convertido para UTC.

## Limitações conhecidas e validações necessárias

1. A boia de dois fios não permite detectar com segurança um cabo rompido. `valid:true` indica que o pino foi lido, não que a integridade elétrica da boia foi diagnosticada.
2. `watering.pump_on` informa o estado lógico do relé, não confirma fluxo real de água. Para isso seria necessário sensor de fluxo.
3. A leitura ultrassônica ainda utiliza `pulseIn()` e um intervalo entre sensores; pode bloquear o loop por dezenas de milissegundos.
4. O módulo HX711 não se recupera automaticamente se o conversor deixar de responder após a inicialização.
5. Os valores de calibração, limites, ângulos do servo e polaridade do relé precisam ser testados no hardware real.
6. O lembrete de limpeza considera a primeira limpeza pendente até que uma confirmação seja registrada.
7. A tela de conectividade ainda exibe `--`, pois a ponte ESP8266 atual não entrega estados Wi-Fi/MQTT ao Mega no contrato de comunicação revisado.
8. A biblioteca `LiquidCrystal_I2C` deve ser instalada e seu endereço I2C confirmado no módulo real.
8. A revisão foi feita por inspeção estática dos arquivos. Não foi possível executar compilação real porque o ambiente não possui Arduino CLI/toolchain nem as bibliotecas instaladas. Compile no Arduino IDE antes de conectar os atuadores.

## Teste inicial recomendado

1. Desconecte a bomba e o mecanismo da comporta durante o primeiro teste.
2. Compile para Arduino Mega 2560.
3. Teste o RTC e confirme o horário.
4. Teste leituras dos reservatórios e da balança.
5. Teste a boia manualmente, observando `level_state`.
6. Teste o relé sem a bomba conectada.
7. Teste a alimentação sem ave e com a comporta mecanicamente segura.
8. Só depois teste comandos remotos via ESP8266.

**Atenção:** não deixe bomba, servo ou alimentação mecânica operando sem supervisão durante os primeiros testes.
