# SmartBird

> **"Mais que automatizar tarefas, queremos tornar o cuidado mais inteligente."**

SmartBird é um projeto acadêmico de automação e monitoramento para aves domésticas, desenvolvido com foco em sistemas embarcados, Internet das Coisas (IoT) e desenvolvimento web.

O sistema combina um **Arduino Mega 2560** responsável pelo controle local e um **ESP32** responsável pela conectividade, permitindo alimentação automática, monitoramento dos reservatórios, controle de hidratação e acompanhamento remoto por meio de um painel web.

## Principais Funcionalidades

- Alimentação automática e manual
- Controle de hidratação
- Monitoramento dos níveis de água e ração
- Alertas de limpeza e manutenção
- Comunicação MQTT via internet
- Painel web para acompanhamento remoto
- Funcionamento local mesmo sem conexão com a rede

## Arquitetura

```text
Painel Web
    │
    ▼
HiveMQ Cloud
    │
    ▼
ESP32
    │ UART
    ▼
Arduino Mega 2560
    │
Sensores e Atuadores
```

## Tecnologias

- Arduino Mega 2560
- ESP32
- MQTT
- HiveMQ Cloud
- JSON
- HTML, CSS e JavaScript
- RTC DS3231
- Sensores Ultrassônicos
- Servo Motor

## Objetivo

Desenvolver uma solução inteligente capaz de automatizar tarefas de alimentação e hidratação, melhorar o monitoramento da rotina da ave e demonstrar a integração entre hardware, software e computação em nuvem.

---

Projeto desenvolvido para fins acadêmicos.
