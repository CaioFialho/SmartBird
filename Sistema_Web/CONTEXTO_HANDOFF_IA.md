# Contexto Completo de Handoff — Painel Web SmartBird

> **Instruções para a IA que receber este arquivo:**
> Este documento consolida **100% do contexto, especificação oficial, contrato JSON v1.0 e todas as 14 decisões de design/arquitetura já aprovadas pelo usuário**.
> Seu objetivo é criar os arquivos **`index.html`**, **`style.css`** e **`app.js`** dentro da pasta **`Sistema_Web/`** seguindo rigorosamente cada requisito abaixo, sem precisar refazer perguntas já respondidas.

---

## 1. Estado Atual do Repositório

- **Workspace:** `c:\Users\fjoao\Documents\GitHub\SmartBird`
- **Pasta alvo da aplicação Web:** `Sistema_Web/`
- **Arquivo já pronto em `Sistema_Web/`:** `Logo_SmartBird.jpg` (logo oficial já copiada — usar na tela de login e no topo da sidebar).
- **Arquivos a criar em `Sistema_Web/`:**
  1. `index.html`
  2. `style.css`
  3. `app.js`
- **Documentos de referência no repositório (caso queira consultar detalhes extras):**
  - `protocolo_comunicacao_v1.md` (Protocolo oficial v0.2.1 / contrato JSON `"v": "1.0"`)
  - `Documento_Mestre_SmartBird2.0.md`

---

## 2. Decisões Confirmadas pelo Usuário (Resultado do Alinhamento)

Siga **estritamente** estas 14 decisões (incluindo onde o usuário escolheu divergir propositalmente de algum item da spec original):

| # | Tema | Decisão Aprovada |
|---|---|---|
| 1 | **Layout de Navegação** | **Sidebar lateral fixa à esquerda** (`#1B4332`) no Desktop, colapsando para **Bottom Navigation Bar fixa** no Mobile, com 4 abas: Dashboard, Agendamentos, Configurações e Diagnóstico. |
| 2 | **Visualização dos Reservatórios** | **Anéis SVG circulares (donut chart)** com o percentual no centro (e `distance_cm` como info secundária quando presente). |
| 3 | **Ícones** | **FontAwesome Free 6 via CDN** (`https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.5.1/css/all.min.css`). |
| 4 | **Notificações / Toasts** | Toasts flutuantes no canto superior direito com **auto-dismiss de 5s** para info/sucesso e **persistente** para erros + **badge numérico** na sidebar indicando eventos não lidos. |
| 5 | **Persistência do Login** | **`localStorage`** quando *"Lembrar acesso neste dispositivo"* estiver marcado; limpar ao clicar em *"Desconectar / Sair"*. |
| 6 | **Idioma (Bilíngue)** | Interface, labels, botões e traduções amigáveis em **Português (PT-BR)**, mantendo as mensagens técnicas originais vindas do Arduino (`error.message`) em **Inglês** junto ao código/tradução. |
| 7 | **Confirmação de Comandos** | Modal curto de confirmação **apenas para ações físicas** (`feed_manual` e `water_manual`), prevenindo acionamentos acidentais. Demais comandos enviam direto. |
| 8 | **Política de Auto-Retry** | **Zero auto-retry para qualquer comando** (nem operações físicas, nem consultas). Em caso de timeout, exibir erro/aviso e deixar o usuário decidir manualmente. |
| 9 | **Modo Demo / Simulador Offline** | **Desvio intencional da spec:** **NÃO colocar botão de "Modo Demonstração" na tela de login.** O acesso exige conexão real ao EMQX primeiro; o Simulador fica acessível apenas via toggle na aba **Diagnóstico**. |
| 10 | **Reconexão Automática MQTT** | **Backoff exponencial** (`2s`, `4s`, `8s`, `16s`, máx `30s`) + banner laranja *"Reconectando..."* no topo + **re-disparo automático da sequência de inicialização** ao reconectar. |
| 11 | **UX de Agendamentos** | **Botão "Salvar Alterações" explícito.** O usuário adiciona/remove horários e alterna switches localmente, e envia o `set_schedule` completo ao clicar em salvar (confirmando na UI apenas após `response: success`). |
| 12 | **UX de Configurações** | **Botão "Salvar Configuração" por módulo** (um formulário para Alimentação/Ração enviando `set_feeding_config`, outro para Hidratação/Água enviando `set_watering_config`), com validação em tempo real no frontend. |
| 13 | **Organização do JS** | **Arquivo único `app.js`** (ES6+ Vanilla, padrão IIFE) bem organizado com seções/comentários claros para cada serviço interno. |
| 14 | **Logo** | Usar a imagem real **`Logo_SmartBird.jpg`** (já existente em `Sistema_Web/Logo_SmartBird.jpg`) na tela de login e no topo da sidebar. |

---

## 3. Stack Tecnológica e Identidade Visual

### Stack (sem bundlers — hospedagem estática direta)
- **HTML5 Semântico** (`index.html`)
- **Tailwind CSS via CDN** (`https://cdn.tailwindcss.com`) com configuração customizada da paleta + `style.css` complementar
- **FontAwesome 6 Free via CDN** (`https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.5.1/css/all.min.css`)
- **Eclipse Paho MQTT JS Client via CDN** (`https://cdnjs.cloudflare.com/ajax/libs/paho-mqtt/1.0.1/mqttws31.min.js`)
- **JavaScript Vanilla ES6+** (`app.js`)

### Paleta de Cores Oficial SmartBird
- **Bege Claro (fundo base):** `#F7F4EB` (`sb-beige`) e `#EFECE1` (`sb-beige-dark`) + `#FFFFFF` para cartões
- **Verde Escuro (primária / ações seguras / sidebar):** `#1B4332` (`sb-green-dark`), `#2D6A4F` (`sb-green`), `#40916C` (`sb-green-light`)
- **Laranja Vibrante (destaque / alimentação manual / alertas):** `#F77F00` (`sb-orange`), `#E36414` (`sb-orange-dark`)
- **Vermelho (erro crítico / sensor inválido / rejeitado / desconexão):** `#D62828` (`sb-red`)

---

## 4. Arquitetura, Regras de Ouro e MQTT

### Arquitetura Física/Lógica
`Aplicação Web (WSS)` ➔ `EMQX Cloud Broker` ➔ `ESP8266 NodeMCU V3 (Ponte Transparente UART 9600 baud)` ➔ `Arduino Mega 2560 (Fonte Única da Verdade)`

### Regras de Ouro
1. **Arduino é a autoridade:** A Web nunca presume sucesso ao publicar no MQTT; toda ação depende de `ack` + `response` do Arduino.
2. **Ciclo Assíncrono em 3 Etapas:** `command` ➔ `ack` (máx **5s**) ➔ `response` (máx **15s** após `ack: accepted`).
3. **Sem Auto-Retry:** Nenhum comando é reenviado automaticamente após timeout.
4. **Sensores Ultrassônicos (`valid: false` ≠ `0%`):** Se `valid === false`, **jamais exibir `0%`**; mostrar badge vermelho *"Leitura Inválida / Sensor Sem Retorno"* e desabilitar o anel normal.

### Login Gate & Configuração Paho MQTT
- Tela inicial bloqueia o painel até `onSuccess` real do broker EMQX.
- Campos: Usuário EMQX (`username`), Senha EMQX (`password` com botão mostrar/ocultar), checkbox *"Lembrar acesso neste dispositivo"* (`localStorage`), e acordeão recolhível **Configurações Avançadas** pré-preenchido:
  - **Host:** `x1901c96.ala.eu-central-1.emqxsl.com` (domínio limpo, sem `wss://`)
  - **Porta WSS:** `8084`
  - **Path:** `/mqtt`
  - **Device ID:** `gaiola01`
- Instanciação Paho:
  ```js
  const clientId = 'SmartBird-Web-' + Math.random().toString(16).substring(2, 10);
  const client = new Paho.MQTT.Client(host, Number(port), path, clientId);
  // connect options: useSSL: true, timeout: 10, keepAliveInterval: 60, cleanSession: true, userName, password
  ```
- **Estritamente 2 tópicos:**
  - Publicação (Web ➔ Arduino): `smartbird/{device_id}/comandos` — **QoS 1**, **`retained = false`** (obrigatório em 100% das mensagens).
  - Subscrição (Arduino ➔ Web): `smartbird/{device_id}/status` — **QoS 0**.
- **Regra UART/MQTT:** Sempre usar `JSON.stringify(payload)` em **uma única linha compacta**, limite máximo **1024 bytes**.

---

## 5. Máquina de Estados de Comandos e Inicialização

```js
const CONFIG_TIMEOUTS = {
  ACK_TIMEOUT_MS: 5000,      // 5s para receber ACK
  RESPONSE_TIMEOUT_MS: 15000 // 15s após ACK accepted para receber Response final
};
```

- **IDs únicos:** `"cmd-" + contador` para comandos gerais; `"sync-" + contador` para `sync_time`. Armazenar em `pendingCommands = new Map()`.
- **Etapa 1 (Envio):** Estado `waiting_ack`, inicia timer 5s. Se estourar: remove do mapa, exibe erro *"Sem resposta do equipamento (Timeout de ACK). Verifique se o ESP8266/Arduino está online."* (sem retry).
- **Etapa 2 (`ack`):** Cancela timer de 5s.
  - Se `status === "accepted"`: muda para `waiting_response`, UI mostra *"Executando no Arduino (máx 15s)..."*, inicia timer 15s.
  - Se `status === "rejected"`: remove do mapa e exibe alerta de recusa com motivo.
  - Se timer 15s estourar: **não declarar falha física automática nem reenviar**; exibir aviso *"Tempo de confirmação excedido (Resultado desconhecido). Consulte o status antes de tentar novamente"* com ação rápida para `get_status` / `get_telemetry`.
- **Etapa 3 (`response` ou `error` com `id`):** Cancela timer de 15s, remove do mapa, aplica resultado (`success` ou `failure`/erro traduzido + mensagem técnica em inglês).
- **Trava de UI:** Enquanto `feed_manual` ou `water_manual` estiver em `waiting_ack`, `waiting_response` ou `running`, o botão correspondente fica `disabled` com spinner e texto da etapa.

### Sequência de Inicialização Automática (ao conectar ou reconectar)
Após assinar `smartbird/{device_id}/status`, disparar com espaçamento de **`300ms`** entre cada envio:
1. `sync_time` (ISO 8601 com offset local, ex: `2026-11-17T15:45:00-03:00`)
2. `get_status`
3. `get_telemetry`
4. `get_schedule`
5. `get_system_info`

---

## 6. Contrato JSON Completo (`v: "1.0"`)

Todo comando Web envia: `{"v":"1.0","type":"command","id":"<id>","source":"web","action":"<action>","data":{...}}`

### 6.1 Comandos
- **`get_status`** (`data: {}`) ➔ Response `data`: `{"system":"operational","feeding":"idle","watering":"idle","maintenance":"normal","rtc":"available"}`
- **`get_telemetry`** (`data: {}`) ➔ Response `data`: `{"reservoirs":{"food":{"level_percent":76.0,"distance_cm":12.4,"valid":true,"low":false},"water":{"level_percent":62.0,"distance_cm":15.1,"valid":true,"low":false}},"feeding":{"state":"idle"},"watering":{"state":"idle"}}`
- **`get_schedule`** (`data: {}`) ➔ Response `data`: `{"feeding":{"enabled":true,"times":["08:00","18:00"]},"watering":{"enabled":true,"times":["08:00","18:00"]}}`
- **`get_system_info`** (`data: {}`) ➔ Response `data`: `{"device":"smartbird","controller":"arduino_mega_2560","firmware_version":"0.1.0","protocol_version":"1.0"}`
- **`feed_manual`** (`data: {}`) ➔ Response `data`: `{"module":"feeding","result":"completed"}` (ou `status: "failure"`, `error: {code, message}`)
- **`water_manual`** (`data: {}`) ➔ Response `data`: `{"module":"watering","result":"completed"}`
- **`ack_cleaning`** (`data: {}`) ➔ Response `data`: `{"cleaning_due":false,"maintenance_timer_reset":true}`
- **`set_schedule`** (`data: {"feeding":{"enabled":true,"times":["08:00","18:00"]},"watering":{"enabled":true,"times":["08:00","18:00"]}}`) ➔ Response `data`: `{"schedule_updated":true}`
  - Regras: Máximo **4 horários** por módulo (desabilitar botão adicionar ao atingir 4/4), `<input type="time">` livre `HH:MM`, impedir duplicados, manter ordenado cronologicamente. Manter função utilitária para `remove_schedule` (`data: {"module":"feeding","time":"18:00"}`) disponível para diagnósticos.
- **`set_feeding_config`** (`data`: `target_weight_g` [20.0], `max_duration_ms` [5000], `reference_distance_cm` [25.0], `useful_height_cm` [20.0], `low_threshold_percent` [20.0], `full_threshold_percent` [95.0], `cleaning_interval_hours` [48]) ➔ Response `data`: `{"configuration_updated":true}`
- **`set_watering_config`** (`data`: `target_level_percent` [90.0], `max_duration_ms` [10000], `reference_distance_cm` [25.0], `useful_height_cm` [20.0], `low_threshold_percent` [25.0], `full_threshold_percent` [95.0]) ➔ Response `data`: `{"configuration_updated":true}`
  - Validações frontend: impedir valores negativos, impedir `low_threshold_percent >= full_threshold_percent`, impedir `useful_height_cm > reference_distance_cm`. Exibir fórmula física na aba Configurações: `alturaConteudo = distanciaReferencia - distanciaMedida` e `percentualNivel = (alturaConteudo / alturaUtil) * 100`.
- **`sync_time`** (`id` com prefixo `"sync-"`, `data: {"datetime":"YYYY-MM-DDTHH:MM:SS±HH:MM"}`) ➔ Response `data`: `{"rtc_updated":true,"datetime":"..."}`

### 6.2 Mensagens Espontâneas (Arduino ➔ Web)
- **`telemetry`**: Atualiza reservatórios, estados dos módulos, `maintenance.cleaning_due` e *"Última atualização recebida às HH:MM:SS"*.
- **`event`**: Gera toast, incrementa badge de notificações não lidas e registra no histórico:
  - `feeding_completed` (`origin`: `"scheduled"` / `"manual"`)
  - `watering_completed` (`origin`)
  - `reservoir_low` (`reservoir`: `"food"` | `"water"`, `level_percent`)
  - `cleaning_due` (ativa alerta no card de manutenção)
  - **`scheduled_execution_missed`** (`module`: `"feeding"` | `"watering"`, `scheduled_time`, `action_required`: `"user_decision"`):
    - Abre **Modal de Decisão em Destaque** + exibe **banner fixo no topo** enquanto pendente.
    - Botão 1: *"Executar Agora Manualmente"* ➔ fecha pendência e dispara `feed_manual` ou `water_manual`.
    - Botão 2: *"Ignorar / Manter Apenas Próximo Horário"* ➔ fecha pendência sem acionar motores e registra no log local.

### 6.3 Mapeamento de Estados e Códigos de Erro
- **Sistema (`system`):** `initializing` (Azul), `operational` (Verde `#2D6A4F`), `alert` (Laranja `#F77F00`), `error` (Vermelho `#D62828`)
- **Alimentação / Hidratação:** `idle` (Neutro/Verde suave), `checking` (Laranja pulsando), `running` (Laranja spinner), `completed` (Verde), `error` (Vermelho)
- **RTC (`rtc`):** `available` (Verde), `invalid` (Laranja — sugerir `sync_time`), `failure` (Vermelho)
- **Códigos de erro (`error.code`)** — mostrar tradução PT-BR + `error.message` técnico original em inglês:
  - `INVALID_JSON`, `INVALID_MESSAGE`, `UNKNOWN_COMMAND`, `INVALID_PARAMETER`, `UNSUPPORTED_VERSION`, `BUSY`, `SAFETY_BLOCK`, `WEIGHT_SENSOR_FAILURE`, `WATER_SENSOR_FAILURE`, `RESERVOIR_SENSOR_FAILURE`, `ACTUATOR_TIMEOUT`, `RTC_FAILURE`, `INTERNAL_ERROR`.

---

## 7. Aba 4: Diagnóstico & Simulador (Módulo de Testes Removível)

Isolar claramente no HTML e no `app.js` com comentários delimitadores:
`/* === INÍCIO DO MÓDULO DE TESTES E SIMULAÇÃO (PODE SER REMOVIDO EM PRODUÇÃO) === */`
`/* === FIM DO MÓDULO DE TESTES E SIMULAÇÃO === */`

1. **Terminal de Tráfego JSON em Tempo Real:** Console escuro exibindo `[TX Web ➔ Arduino]` e `[RX Arduino ➔ Web]` com timestamp, tópico e JSON colorido. Botões: *Limpar Console*, *Pausar Scroll*, e campo + botão para *Enviar Comando JSON Manual*.
2. **Simulador do Arduino Mega (Modo Mock / Loopback):**
   - Toggle: *"Ativar Simulador de Respostas do Arduino (Para Testes sem Hardware)"*.
   - Quando ativo: intercepta comandos enviados e injeta `ack: accepted` após `400ms` e `response: success` realista após `2000ms` (preservando o `id`).
   - 6 Botões rápidos de cenário crítico:
     1. Telemetria Normal (Ração 76%, Água 62%)
     2. Sensor Ultrassônico Inválido (`valid: false`)
     3. Evento de Nível Baixo (`reservoir_low`)
     4. Alerta de Limpeza 48h (`cleaning_due`)
     5. Refeição Programada Perdida (`scheduled_execution_missed`)
     6. Falha na Bomba d'Água (`WATER_SENSOR_FAILURE`)

---

## 8. Organização Interna Recomendada para `app.js`

Estrutura única em IIFE com camadas conforme seção 19.3 do protocolo:
1. `CONFIG_TIMEOUTS` e `CONFIG_DEFAULTS`
2. `AppState` (estado global reativo)
3. `mqtt_client` (Paho WSS, login, reconexão exponencial, subscrição)
4. `command_service` (IDs únicos, `pendingCommands`, QoS 1, `retained: false`, timeouts 5s/15s)
5. `response_handler` (correlação de `ack`, `response` e `error` por `id`)
6. `telemetry_handler` (atualização dos anéis SVG, cards e última atualização)
7. `event_handler` (toasts, badge não lidos, modal + banner de `scheduled_execution_missed`)
8. `time_service` (`getLocalISOStringWithOffset()` + `sync_time`)
9. `UIController`, `ToastService`, `ModalService`, `ScheduleManager`, `ConfigManager`
10. `mock_simulator` e `TerminalLogger` *(dentro do bloco comentado removível)*
