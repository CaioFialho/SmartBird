(() => {
  'use strict';

  const CONFIG_TIMEOUTS = { ACK_TIMEOUT_MS: 5000, RESPONSE_TIMEOUT_MS: 15000 };
  const CONFIG_DEFAULTS = {
    host: 'x1901c96.ala.eu-central-1.emqxsl.com', port: 8084, path: '/mqtt', deviceId: 'gaiola01'
  };
  const STORAGE_KEY = 'smartbird.connection.v1';
  const MAX_PAYLOAD_BYTES = 1024;
  const MAX_SCHEDULE_TIMES = 4;
  const SECONDS_PER_HOUR = 3600;
  const MAX_CLEANING_INTERVAL_SECONDS = 0xFFFFFFFF;
  const MODULES = {
    feeding: { label: 'Alimentação', action: 'feed_manual', button: 'feed', caption: 'Alimentar agora' },
    watering: { label: 'Hidratação', action: 'water_manual', button: 'water', caption: 'Repor água agora' }
  };
  const CONFIG_GROUPS = {
    feeding: [
      { title: 'Dosagem', fields: [
        ['target_weight_g', 'Peso-alvo (g)', 20],
        ['max_duration_ms', 'Tempo máximo do servo (ms)', 5000, 'integer']
      ] },
      { title: 'Calibração do reservatório', fields: [
        ['reference_distance_cm', 'Distância de referência (cm)', 25],
        ['useful_height_cm', 'Altura útil (cm)', 20],
        ['low_threshold_percent', 'Limiar de nível baixo (%)', 20, 'percent'],
        ['full_threshold_percent', 'Limiar de nível cheio (%)', 95, 'percent']
      ] },
      { title: 'Higienização', fields: [
        ['cleaning_interval_seconds', 'Intervalo de limpeza (h)', 48 * SECONDS_PER_HOUR, 'integer', SECONDS_PER_HOUR]
      ] }
    ]
  };
  const CONFIG_FIELDS = Object.fromEntries(Object.entries(CONFIG_GROUPS).map(
    ([module, groups]) => [module, groups.flatMap((group) => group.fields)]
  ));
  const ERROR_MESSAGES = {
    INVALID_JSON: 'O pacote recebido contém JSON inválido.',
    INVALID_MESSAGE: 'A estrutura da mensagem é inválida.',
    UNKNOWN_COMMAND: 'O firmware não reconheceu o comando.',
    INVALID_PARAMETER: 'Um ou mais parâmetros estão fora dos limites aceitos.',
    UNSUPPORTED_VERSION: 'A versão do protocolo é incompatível com o firmware.',
    BUSY: 'O equipamento já está executando uma operação.',
    SAFETY_BLOCK: 'Acionamento bloqueado pelas condições de segurança do equipamento.',
    WEIGHT_SENSOR_FAILURE: 'Falha na verificação do sensor de alimentação.',
    WATER_SENSOR_FAILURE: 'Falha na verificação do bebedouro. Bomba bloqueada por segurança.',
    RESERVOIR_SENSOR_FAILURE: 'Falha na leitura do sensor do reservatório de origem.',
    ACTUATOR_TIMEOUT: 'O atuador atingiu o tempo limite de segurança.',
    RTC_FAILURE: 'Falha de comunicação com o relógio RTC.',
    INTERNAL_ERROR: 'O equipamento informou uma falha interna.'
  };
  const $ = (id) => document.getElementById(id);
  const isRecord = (value) => value !== null && typeof value === 'object' && !Array.isArray(value);
  const isTime = (value) => typeof value === 'string' && /^([01]\d|2[0-3]):[0-5]\d$/.test(value);
  const isScheduleSlot = (value) => isTime(value) && /:(00|30)$/.test(value);
  const isFirmwareTargetWeight = (value) => Number.isFinite(value) && value >= 1 && value <= 500;
  const scheduleHourOptions = Array.from({ length: 24 }, (_, hour) => {
    const label = String(hour).padStart(2, '0');
    return `<option value="${label}">${label}</option>`;
  }).join('');
  const isPercent = (value) => Number.isFinite(value) && value >= 0 && value <= 100;
  const owns = (object, key) => Object.prototype.hasOwnProperty.call(object, key);
  const copy = (value) => JSON.parse(JSON.stringify(value));
  const numberFormat = new Intl.NumberFormat('pt-BR', { maximumFractionDigits: 2 });
  const equipmentState = () => ({
    status: {}, reservoirs: {}, containers: {}, pumpOn: null, cleaningDue: null, updatedAt: null, systemInfo: {},
    schedule: null, draft: null, draftBase: null, scheduleStale: false,
    confirmedConfig: {}, configDrafts: {}, missed: [], decidedMissed: new Set(), eventScope: { closed: false }
  });
  const AppState = {
    signedIn: false, connection: 'offline', settings: null,
    equipment: equipmentState(), history: [], view: 'dashboard'
  };
  const pendingCommands = new Map();
  const diagnostics = {};
  const scheduleActions = ['get_schedule', 'set_schedule', 'remove_schedule'];
  const STATUS_ICONS = {
    neutral: 'circle-minus', success: 'circle-check', warning: 'triangle-exclamation',
    danger: 'circle-exclamation', info: 'circle-info'
  };
  const makeIcon = (name, extraClass = '') => {
    const icon = document.createElement('i');
    icon.className = 'fa-solid fa-' + name + extraClass;
    icon.setAttribute('aria-hidden', 'true');
    return icon;
  };

  // ToastService e ModalService
  const ToastService = {
    show(title, message, tone = 'success', shortcut) {
      const toast = document.createElement('article');
      toast.className = 'toast ' + tone;
      toast.setAttribute('role', tone === 'danger' ? 'alert' : 'status');
      toast.innerHTML = '<div><strong class="toast-title"></strong><p></p></div><button class="icon-button" type="button" aria-label="Dispensar aviso"></button>';
      toast.querySelector('strong').append(makeIcon(STATUS_ICONS[tone], ' status-icon'), title);
      toast.querySelector('p').textContent = message;
      const close = toast.querySelector('button');
      close.append(makeIcon('xmark'));
      close.addEventListener('click', () => toast.remove());
      if (shortcut) {
        const button = document.createElement('button');
        button.type = 'button';
        button.className = 'button small';
        button.textContent = 'Consultar equipamento';
        button.addEventListener('click', () => { if (shortcut()) toast.remove(); });
        toast.firstElementChild.append(button);
      }
      $('toasts').prepend(toast);
      if (tone === 'success' || tone === 'info') setTimeout(() => toast.remove(), 5000);
    }
  };

  const ModalService = {
    callback: null,
    open({ title, description, confirm, cancel, kind, onClose }) {
      const dialog = $('action-dialog');
      if (dialog.open || $('events-dialog').open) return false;
      this.callback = onClose;
      dialog.returnValue = '';
      dialog.dataset.kind = kind;
      const icons = { feed_manual: 'seedling', water_manual: 'droplet', missed: 'clock-rotate-left' };
      $('action-icon').className = 'fa-solid fa-' + icons[kind];
      $('action-title').textContent = title;
      $('action-description').textContent = description;
      $('action-confirm').textContent = confirm;
      $('action-confirm').classList.toggle('accent', kind === 'missed' || kind === 'feed_manual');
      $('action-cancel').textContent = cancel;
      dialog.showModal();
      return true;
    },
    clear() {
      this.callback = null;
      if ($('action-dialog').open) $('action-dialog').close();
    },
    decide(decision) {
      const dialog = $('action-dialog');
      const kind = dialog.dataset.kind;
      const callback = this.callback;
      this.clear();
      callback?.(decision);
      if (kind !== 'missed') event_handler.review();
    }
  };

  // time_service
  const time_service = {
    getLocalISOStringWithOffset(now = new Date()) {
      const offset = -now.getTimezoneOffset();
      const pad = (value) => String(Math.trunc(Math.abs(value))).padStart(2, '0');
      const day = [now.getFullYear(), pad(now.getMonth() + 1), pad(now.getDate())].join('-');
      const clock = [now.getHours(), now.getMinutes(), now.getSeconds()].map(pad).join(':');
      return day + 'T' + clock + (offset >= 0 ? '+' : '-') + pad(offset / 60) + ':' + pad(offset % 60);
    },
    sync(quiet = false) {
      return command_service.send('sync_time', { datetime: this.getLocalISOStringWithOffset() }, { quiet });
    }
  };

  // mqtt_client
  const mqtt_client = {
    client: null,
    retryTimer: null,
    retryDelay: 2000,
    initialization: 0,
    topic(direction) {
      return 'smartbird/' + AppState.settings.deviceId + '/' + direction;
    },
    connect() {
      const options = AppState.settings;
      let client;
      try {
        if (!window.Paho?.MQTT) throw new Error('O cliente MQTT não carregou. Verifique o acesso ao CDN e tente novamente.');
        const clientId = 'SmartBird-Web-' + crypto.getRandomValues(new Uint32Array(1))[0].toString(16).padStart(8, '0');
        client = new Paho.MQTT.Client(options.host, options.port, options.path, clientId);
        this.client = client;
        client.onConnectionLost = (result) => this.failure(client, result.errorMessage);
        client.onMessageArrived = (message) => {
          if (client !== this.client || message.destinationName !== this.topic('status')) return;
          response_handler.receive(message.payloadString, message.destinationName, 'MQTT', message.retained);
        };
        client.connect({
          useSSL: true, timeout: 10, keepAliveInterval: 60, cleanSession: true,
          userName: options.username, password: options.password,
          onSuccess: () => {
            if (client !== this.client) return;
            try {
              client.subscribe(this.topic('status'), {
                qos: 0, timeout: 10,
                onSuccess: (result) => {
                  if (result?.grantedQos?.includes(128)) {
                    this.failure(client, 'O broker recusou a assinatura do tópico de status.');
                    return;
                  }
                  this.connected(client);
                },
                onFailure: (result) => this.failure(client, result.errorMessage)
              });
            } catch (error) {
              this.failure(client, error.message);
            }
          },
          onFailure: (result) => this.failure(client, result.errorMessage)
        });
      } catch (error) {
        this.failure(client, error.message);
      }
    },
    connected(client) {
      if (client !== this.client) return;
      const firstConnection = !AppState.signedIn;
      AppState.signedIn = true;
      AppState.connection = 'online';
      this.retryDelay = 2000;
      if (firstConnection) {
        UIController.rememberAccess();
        UIController.openView('dashboard');
      }
      UIController.render();
      if (firstConnection) $('dashboard-title').focus();
      //if (!diagnostics.active?.()) this.initialize();
    },
    failure(client, details) {
      if (client && client !== this.client) return;
      this.closeClient();
      if (AppState.signedIn) {
        AppState.connection = 'reconnecting';
        if (!diagnostics.active?.()) {
          const equipment = AppState.equipment;
          telemetry_handler.invalidateFreshness(equipment);
          equipment.scheduleStale = Boolean(equipment.schedule);
          equipment.status = {};
          command_service.cancelAll(true);
        }
        const delay = this.retryDelay;
        this.retryDelay = Math.min(delay * 2, 30000);
        this.retryTimer = setTimeout(() => this.connect(), delay);
      } else {
        AppState.connection = 'offline';
        const message = 'Falha na conexão com o EMQX. Verifique as credenciais e as configurações do broker.';
        $('login-error').textContent = details ? message + ' ' + details : message;
        $('login-error').hidden = false;
        AppState.settings = null;
      }
      UIController.render();
    },
    closeClient() {
      clearTimeout(this.retryTimer);
      this.initialization += 1;
      const client = this.client;
      this.client = null;
      if (client?.isConnected()) client.disconnect();
    },
    async initialize() {
      const token = ++this.initialization;
      const steps = [() => time_service.sync(true), ...['get_status', 'get_telemetry', 'get_schedule', 'get_system_info']
        .map((action) => () => command_service.send(action, {}, { quiet: true }))];
      for (const step of steps) {
        if (token !== this.initialization || !command_service.available()) break;
        step();
        await new Promise((resolve) => setTimeout(resolve, 300));
      }
    },
    publish(payload) {
      const message = new Paho.MQTT.Message(payload);
      message.destinationName = this.topic('comandos');
      message.qos = 1;
      message.retained = false;
      this.client.send(message);
    }
  };

  // command_service
  function feedingSafetyError({ reservoirs, containers }) {
    const reservoir = reservoirs.food;
    const bowl = containers.food_bowl;
    let message;
    if (reservoir?.valid !== true || reservoir.stale) {
      message = 'A leitura do reservatório de ração está indisponível ou desatualizada.';
    } else if (reservoir.low === true || reservoir.level_percent === 0) {
      message = 'O reservatório de ração está em nível baixo. Reabasteça antes de alimentar.';
    } else if (reservoir.low !== false) {
      message = 'O nível do reservatório de ração ainda não foi confirmado.';
    } else if (bowl?.valid !== true || bowl.stale) {
      message = 'A leitura do peso do pote de ração está indisponível ou desatualizada.';
    }
    return message ? { code: 'SAFETY_BLOCK', message } : null;
  }

  let commandCounter = BigInt(Date.now()) * 2n ** 32n + BigInt(crypto.getRandomValues(new Uint32Array(1))[0]);
  const command_service = {
    available() {
      return AppState.signedIn && (Boolean(diagnostics.active?.()) ||
        (AppState.connection === 'online' && Boolean(mqtt_client.client?.isConnected())));
    },
    pending(action) {
      return [...pendingCommands.values()].find((command) => command.action === action);
    },
    canActuate(action) {
      const module = action === 'feed_manual' ? 'feeding' : 'watering';
      return this.available() && !this.pending(action) && !['running', 'checking'].includes(AppState.equipment.status[module]);
    },
    send(action, data = {}, { quiet = false } = {}) {
      if (this.pending(action)) return false;
      if (!this.available()) {
        ToastService.show('Equipamento indisponível', 'Aguarde a conexão com o broker antes de enviar um comando.', 'danger');
        return false;
      }
      if (['feed_manual', 'water_manual'].includes(action) && !this.canActuate(action)) return false;
      if (action === 'feed_manual') {
        const error = feedingSafetyError(AppState.equipment);
        if (error) {
          response_handler.showError('Alimentação bloqueada', error);
          return false;
        }
      }
      if (scheduleActions.includes(action) && scheduleActions.some((name) => this.pending(name))) {
        if (!quiet) ToastService.show('Agenda em atualização', 'Aguarde a confirmação do comando anterior.', 'warning');
        return false;
      }
      if (action === 'set_schedule') {
        const schedule = ScheduleManager.normalize(data);
        if (!schedule) {
          ToastService.show('Agenda inválida', 'Confira os horários de alimentação e hidratação.', 'warning');
          return false;
        }
        const warning = ScheduleManager.slotWarning(schedule);
        if (warning) {
          ToastService.show('Horário fora do intervalo', warning, 'warning');
          return false;
        }
        data = schedule;
      }
      const id = (action === 'sync_time' ? 'sync-' : 'cmd-') + (++commandCounter).toString();
      const payload = JSON.stringify({ v: '1.0', type: 'command', id, source: 'web', action, data });
      if (new TextEncoder().encode(payload).length > MAX_PAYLOAD_BYTES) {
        ToastService.show('Comando muito grande', 'O pacote excede o limite de 1024 bytes.', 'danger');
        return false;
      }
      const command = {
        id, action, data: JSON.parse(payload).data, quiet, state: 'waiting_ack',
        timer: setTimeout(() => this.timeout(id), CONFIG_TIMEOUTS.ACK_TIMEOUT_MS)
      };
      pendingCommands.set(id, command);
      try {
        const simulated = Boolean(diagnostics.active?.());
        diagnostics.log?.('TX', mqtt_client.topic('comandos'), payload, simulated ? 'Simulador' : 'MQTT');
        if (simulated) diagnostics.send(command);
        else mqtt_client.publish(payload);
      } catch {
        this.finish(command);
        ToastService.show('Falha no envio', 'O comando não pôde ser enviado. Consulte o status do equipamento.', 'danger');
        return false;
      }
      UIController.render();
      return true;
    },
    finish(command) {
      clearTimeout(command.timer);
      pendingCommands.delete(command.id);
      UIController.render();
    },
    timeout(id) {
      const command = pendingCommands.get(id);
      if (!command) return;
      const awaitingAck = command.state === 'waiting_ack';
      this.finish(command);
      if (awaitingAck) {
        ToastService.show('Sem resposta do equipamento', command.action + ' · Timeout de ACK. Verifique se o ESP8266/Arduino está online.', 'danger');
      } else {
        ToastService.show('Resultado desconhecido', command.action + ' · Tempo de confirmação excedido. Consulte o status antes de tentar novamente.', 'warning', UIController.refresh);
      }
    },
    cancelAll(showNotice = false) {
      if (showNotice && pendingCommands.size) {
        ToastService.show('Conexão interrompida', 'Há comandos sem confirmação. Consulte o status antes de repetir uma operação.', 'warning', UIController.refresh);
      }
      for (const command of pendingCommands.values()) clearTimeout(command.timer);
      pendingCommands.clear();
    }
  };

  // response_handler
  const response_handler = {
    receive(payload, topic, transport, retained = false) {
      diagnostics.log?.('RX', topic, payload, transport);
      if (!AppState.signedIn || retained || (transport === 'MQTT' && diagnostics.active?.())) return;
      let message;
      try { message = JSON.parse(payload); } catch { return; }
      if (message?.v !== '1.0' || message.source !== 'arduino') return;
      if (message.type === 'telemetry' && isRecord(message.data)) {
        telemetry_handler.apply(message.data);
        UIController.render();
      }
      if (message.type === 'event') event_handler.receive(message);
      if (['ack', 'response', 'error'].includes(message.type)) this.resolve(message);
    },
    resolve(message) {
      const command = pendingCommands.get(message.id);
      if (!command) {
        if (message.type === 'error' && !message.id) {
          const text = this.errorText(message.error);
          const simulated = diagnostics.active?.() ? 'Simulador · ' : '';
          event_handler.record(simulated + text, 'danger', message.timestamp, { manual: true });
          ToastService.show('Erro do equipamento', text, 'danger');
        }
        return;
      }
      if (message.type === 'ack' && command.state === 'waiting_ack') {
        if (message.status === 'rejected') {
          command_service.finish(command);
          this.showError('Comando recusado', message.error);
        } else if (message.status === 'accepted') {
          clearTimeout(command.timer);
          command.state = 'waiting_response';
          command.timer = setTimeout(() => command_service.timeout(command.id), CONFIG_TIMEOUTS.RESPONSE_TIMEOUT_MS);
          UIController.render();
        }
      }
      if (message.type === 'error' || (message.type === 'response' && command.state === 'waiting_response')) {
        command_service.finish(command);
        if (message.type === 'error' || message.status !== 'success') {
          const module = Object.keys(MODULES).find((key) => MODULES[key].action === command.action);
          if (module && message.type === 'response' && message.status === 'failure') {
            AppState.equipment.status[module] = 'error';
            UIController.render();
          }
          this.showError('Falha em ' + command.action, message.error);
          return;
        }
        if (!this.apply(command, message.data)) {
          ToastService.show('Confirmação incompleta', 'A resposta de ' + command.action + ' não contém os dados previstos pelo protocolo.', 'danger');
          return;
        }
        UIController.render();
        if (!command.quiet) {
          const title = diagnostics.active?.() ? 'Confirmado pelo simulador' : 'Confirmado pelo equipamento';
          ToastService.show(title, this.successLabel(command.action));
        }
      }
    },
    showError(title, error = {}) {
      ToastService.show(title, this.errorText(error), 'danger');
    },
    errorText(error) {
      if (!isRecord(error)) error = {};
      const code = typeof error.code === 'string' ? error.code : '';
      const description = owns(ERROR_MESSAGES, code) ? ERROR_MESSAGES[code] : 'O equipamento informou um erro.';
      const message = typeof error.message === 'string' ? error.message : '';
      return [code, description, message].filter(Boolean).join(' · ');
    },
    apply(command, data) {
      if (!isRecord(data)) return false;
      const equipment = AppState.equipment;
      switch (command.action) {
        case 'get_status':
          telemetry_handler.apply(data);
          for (const key of ['system', 'feeding', 'watering', 'maintenance', 'rtc']) {
            if (typeof data[key] === 'string') equipment.status[key] = data[key];
          }
          if (data.maintenance === 'normal') telemetry_handler.updateCleaning(false);
          if (data.maintenance === 'cleaning_due') telemetry_handler.updateCleaning(true);
          return true;
        case 'get_telemetry':
          return telemetry_handler.apply(data);
        case 'get_schedule':
          return ScheduleManager.load(data);
        case 'set_schedule':
          return data.schedule_updated === true && ScheduleManager.load(command.data, true);
        case 'remove_schedule':
          if (data.removed !== true) return false;
          equipment.scheduleStale = Boolean(equipment.schedule);
          return true;
        case 'get_system_info':
          equipment.systemInfo = data;
          return true;
        case 'feed_manual':
        case 'water_manual': {
          const module = command.action === 'feed_manual' ? 'feeding' : 'watering';
          if (data.module !== module || data.result !== 'completed') return false;
          equipment.status[module] = 'completed';
          if (module === 'watering') {
            equipment.pumpOn = false;
            telemetry_handler.updateKnownWaterBowlLevel('ok');
          }
          return true;
        }
        case 'ack_cleaning':
          if (data.maintenance_timer_reset !== true || data.cleaning_due !== false) return false;
          telemetry_handler.updateCleaning(false);
          return true;
        case 'set_feeding_config':
          if (data.configuration_updated !== true || (owns(data, 'target_weight_g') &&
            (!Number.isFinite(data.target_weight_g) || data.target_weight_g <= 0))) return false;
          equipment.confirmedConfig.feeding = {
            ...equipment.confirmedConfig.feeding, ...command.data,
            target_weight_g: data.target_weight_g ?? command.data.target_weight_g
          };
          return true;
        case 'sync_time':
          if (data.rtc_updated !== true) return false;
          equipment.status.rtc = 'available';
          return true;
        default:
          return false;
      }
    },
    successLabel(action) {
      const labels = {
        feed_manual: 'Ciclo de alimentação concluído.', water_manual: 'Ciclo de hidratação concluído.',
        set_schedule: 'Agendamentos salvos.', set_feeding_config: 'Configuração de alimentação gravada.',
        ack_cleaning: 'Lembrete de limpeza reiniciado.',
        sync_time: 'Relógio sincronizado.', get_status: 'Status atualizado.', get_telemetry: 'Leituras atualizadas.',
        get_schedule: 'Agendamentos consultados.', get_system_info: 'Identificação atualizada.',
        remove_schedule: 'Horário removido. Consulte a agenda para atualizar a lista.'
      };
      return labels[action];
    }
  };

  // telemetry_handler
  const telemetry_handler = {
    invalidateFreshness(equipment) {
      for (const reading of [...Object.values(equipment.reservoirs), ...Object.values(equipment.containers)]) {
        reading.stale = true;
      }
      equipment.pumpOn = null;
    },
    updateKnownWaterBowlLevel(level) {
      const reading = AppState.equipment.containers.water_bowl;
      if (reading?.valid && !reading.stale) {
        reading.level_state = level;
        reading.low = level === 'low';
      }
      if (level === 'ok') event_handler.resolveAlert('water_bowl');
    },
    updateCleaning(due) {
      AppState.equipment.cleaningDue = due;
      if (!due) event_handler.resolveAlert('cleaning');
    },
    apply(data) {
      const equipment = AppState.equipment;
      let receivedReading = false;
      for (const name of ['food', 'water']) {
        if (!isRecord(data.reservoirs) || !owns(data.reservoirs, name)) continue;
        const reading = data.reservoirs[name];
        equipment.reservoirs[name] = reading?.valid === true && isPercent(reading.level_percent) ? {
          valid: true,
          level_percent: reading.level_percent,
          low: typeof reading.low === 'boolean' ? reading.low : undefined,
          distance_cm: Number.isFinite(reading.distance_cm) && reading.distance_cm > 0 ? reading.distance_cm : undefined
        } : { valid: false };
        if (equipment.reservoirs[name].low === false) event_handler.resolveAlert('reservoir:' + name);
        receivedReading = true;
      }
      for (const name of ['food_bowl', 'water_bowl']) {
        if (!isRecord(data.containers) || !owns(data.containers, name)) continue;
        const reading = data.containers[name];
        let measurement = { valid: false };
        if (reading?.valid === true) {
          if (name === 'food_bowl' && Number.isFinite(reading.weight_g)) {
            measurement = { valid: true, weight_g: reading.weight_g };
          }
          if (name === 'water_bowl' && ['low', 'ok'].includes(reading.level_state)) {
            measurement = { valid: true, level_state: reading.level_state, low: reading.level_state === 'low' };
          }
        }
        equipment.containers[name] = measurement;
        if (name === 'water_bowl' && measurement.level_state === 'ok') event_handler.resolveAlert('water_bowl');
        receivedReading = true;
      }
      if (isRecord(data.watering)) {
        equipment.pumpOn = typeof data.watering.pump_on === 'boolean' ? data.watering.pump_on : null;
      }
      for (const module of Object.keys(MODULES)) {
        if (typeof data[module]?.state === 'string') equipment.status[module] = data[module].state;
      }
      if (typeof data.maintenance?.cleaning_due === 'boolean') this.updateCleaning(data.maintenance.cleaning_due);
      if (receivedReading) equipment.updatedAt = new Date();
      return receivedReading;
    }
  };

  // event_handler
  const event_handler = {
    historyActions: new WeakMap(),
    activeAlert(key) {
      return AppState.history.find(({ alert }) => alert?.scope === AppState.equipment.eventScope &&
        alert.key === key && !alert.resolved);
    },
    resolveAlert(key) {
      const entry = this.activeAlert(key);
      if (entry) this.resolveEntry(entry);
    },
    resolveEntry(entry) {
      entry.alert.resolved = true;
      if ($('events-dialog').open) this.renderHistory();
    },
    record(text, tone, timestamp = new Date(), alert = null) {
      if (alert?.key !== undefined && this.activeAlert(alert.key)) return false;
      const date = new Date(timestamp);
      const open = $('events-dialog').open;
      AppState.history.unshift({
        text, tone, date: Number.isNaN(date.getTime()) ? new Date() : date, unread: !open,
        alert: alert && { ...alert, scope: AppState.equipment.eventScope, resolved: false }
      });
      for (let index = AppState.history.length - 1; index >= 0 && AppState.history.length > 100; index -= 1) {
        const entry = AppState.history[index];
        if (!entry.alert || entry.alert.resolved) AppState.history.splice(index, 1);
      }
      if (open) this.renderHistory();
      UIController.render();
      return true;
    },
    receive(message) {
      const data = message.data;
      if (!isRecord(data)) return;
      let text;
      let tone = 'success';
      let alert = null;
      const equipment = AppState.equipment;
      switch (data.name) {
        case 'feeding_completed':
        case 'watering_completed': {
          const module = data.name === 'feeding_completed' ? 'feeding' : 'watering';
          if (module === 'watering') {
            if ((owns(data, 'container') && data.container !== 'water_bowl') ||
                (owns(data, 'level_state') && data.level_state !== 'ok')) return;
            equipment.pumpOn = false;
            if (data.level_state === 'ok') telemetry_handler.updateKnownWaterBowlLevel('ok');
          }
          const origin = { scheduled: 'programada', manual: 'manual', automatic: 'automática' };
          equipment.status[module] = 'completed';
          text = MODULES[module].label + (owns(origin, data.origin) ? ' ' + origin[data.origin] : '') + ' concluída.';
          break;
        }
        case 'reservoir_low': {
          if (!['food', 'water'].includes(data.reservoir)) return;
          const name = data.reservoir === 'food' ? 'Reservatório de ração' : 'Reservatório de água';
          const level = isPercent(data.level_percent) ? ' (' + numberFormat.format(data.level_percent) + '%)' : '';
          const reading = equipment.reservoirs[data.reservoir];
          if (reading?.valid === true && !reading.stale) {
            reading.low = true;
            if (isPercent(data.level_percent)) {
              reading.level_percent = data.level_percent;
              delete reading.distance_cm;
            }
          }
          text = name + ' em nível baixo' + level + '. Reabasteça o reservatório.';
          tone = 'warning';
          alert = { key: 'reservoir:' + data.reservoir };
          break;
        }
        case 'container_level_low':
          if (data.container !== 'water_bowl' || data.level_state !== 'low') return;
          telemetry_handler.updateKnownWaterBowlLevel('low');
          text = 'A boia do bebedouro indicou nível baixo.';
          tone = 'warning';
          alert = { key: 'water_bowl' };
          break;
        case 'cleaning_due':
          telemetry_handler.updateCleaning(true);
          text = 'Limpeza necessária. Higienize a gaiola e confirme no painel.';
          tone = 'warning';
          alert = { key: 'cleaning' };
          break;
        case 'scheduled_execution_missed': {
          if (!owns(MODULES, data.module) || !isTime(data.scheduled_time) || data.action_required !== 'user_decision') return;
          const timestamp = message.timestamp;
          const occurrence = typeof timestamp === 'string' && Number.isFinite(Date.parse(timestamp)) ?
            JSON.stringify([data.module, data.scheduled_time, timestamp]) : null;
          const duplicate = equipment.missed.some((item) => item.module === data.module &&
            item.scheduled_time === data.scheduled_time && item.timestamp === timestamp);
          if (duplicate || equipment.decidedMissed.has(occurrence)) return;
          const missed = { ...data, timestamp, occurrence };
          equipment.missed.push(missed);
          text = 'A rotina de ' + MODULES[data.module].label.toLowerCase() + ' das ' + data.scheduled_time + ' precisa da sua decisão.';
          tone = 'warning';
          alert = { key: missed };
          break;
        }
        default:
          return;
      }
      const simulated = diagnostics.active?.() ? 'Simulador · ' : '';
      if (this.record(simulated + text, tone, message.timestamp, alert)) {
        ToastService.show(simulated + 'Evento do equipamento', text, tone);
      } else {
        UIController.render();
      }
      if (data.name === 'scheduled_execution_missed') this.review();
    },
    review() {
      const { missed: queue, decidedMissed } = AppState.equipment;
      const missed = queue[0];
      if (!missed) return;
      const { action, label } = MODULES[missed.module];
      ModalService.open({
        title: 'Rotina programada perdida',
        description: 'A rotina programada de ' + label.toLowerCase() + ' das ' + missed.scheduled_time +
          ' não pôde ser executada (possível reinicialização ou queda de energia). Por segurança, o SmartBird aguarda sua decisão.',
        kind: 'missed', confirm: 'Executar agora manualmente', cancel: 'Ignorar / manter apenas próximo horário',
        onClose: (decision) => {
          if (decision === 'confirm' && !command_service.send(action)) return;
          if (decision !== 'cancel' && decision !== 'confirm') return;
          queue.shift();
          if (missed.occurrence) {
            decidedMissed.add(missed.occurrence);
            if (decidedMissed.size > 100) decidedMissed.delete(decidedMissed.values().next().value);
          }
          this.resolveAlert(missed);
          const prefix = diagnostics.active?.() ? 'Simulador · ' : '';
          const text = decision === 'cancel' ? 'Decisão registrada. Aguardar o próximo horário de ' :
            'Execução manual solicitada para a rotina perdida de ';
          this.record(prefix + text + label.toLowerCase() + ' (' + missed.scheduled_time + ').', 'info');
          this.review();
        }
      });
      UIController.renderControls();
    },
    renderHistory() {
      const list = $('event-history');
      const focusedEntry = this.historyActions.get(document.activeElement);
      let focusTarget;
      list.replaceChildren();
      if (!AppState.history.length) {
        const empty = document.createElement('li');
        empty.textContent = 'Nenhum evento registrado nesta sessão.';
        list.append(empty);
      }
      for (const entry of AppState.history) {
        const item = document.createElement('li');
        item.className = 'event-entry ' + entry.tone;
        const content = document.createElement('div');
        const time = document.createElement('time');
        time.dateTime = entry.date.toISOString();
        time.textContent = entry.date.toLocaleString('pt-BR');
        content.append(time, entry.text);
        if (entry.alert) {
          const state = document.createElement('span');
          state.className = 'badge ' + (entry.alert.resolved ? 'success' : entry.tone);
          state.textContent = entry.alert.resolved ? 'Resolvido' : 'Ativo';
          const actions = document.createElement('div');
          actions.className = 'event-actions';
          actions.append(state);
          if (entry.alert.resolved || entry.alert.manual || entry.alert.scope.closed) {
            const button = document.createElement('button');
            button.type = 'button';
            button.className = 'button small';
            button.textContent = entry.alert.resolved ? 'Remover' : 'Marcar como resolvido';
            button.addEventListener('click', () => {
              if (!entry.alert.resolved) {
                this.resolveEntry(entry);
                return;
              }
              const index = AppState.history.indexOf(entry);
              if (index < 0) return;
              AppState.history.splice(index, 1);
              this.renderHistory();
              UIController.render();
            });
            this.historyActions.set(button, entry);
            if (entry === focusedEntry) focusTarget = button;
            actions.append(button);
          }
          content.append(actions);
        }
        item.append(makeIcon(STATUS_ICONS[entry.tone], ' status-icon'), content);
        list.append(item);
      }
      if (focusedEntry) (focusTarget || $('events-dialog').querySelector('[value="close"]')).focus();
    }
  };

  // ScheduleManager
  const ScheduleManager = {
    lastRender: '',
    renderedDraft: null,
    normalize(data) {
      if (!isRecord(data) || Object.keys(data).length !== 2) return null;
      const schedule = {};
      for (const module of Object.keys(MODULES)) {
        const value = data[module];
        if (!isRecord(value) || Object.keys(value).length !== 2 || typeof value.enabled !== 'boolean' ||
          !Array.isArray(value.times) || value.times.length > MAX_SCHEDULE_TIMES ||
          !value.times.every(isTime) || new Set(value.times).size !== value.times.length) return null;
        schedule[module] = { enabled: value.enabled, times: [...value.times].sort() };
      }
      return schedule;
    },
    slotWarning(schedule) {
      if (!schedule) return '';
      const invalid = Object.entries(MODULES).flatMap(([module, { label }]) => {
        const times = schedule[module].times.filter((time) => !isScheduleSlot(time));
        return times.length ? [label + ' (' + times.join(', ') + ')'] : [];
      });
      return invalid.length ? 'Use minutos 00 ou 30. Horários a ajustar · ' + invalid.join(' · ') + '.' : '';
    },
    load(data, saved = false) {
      const schedule = this.normalize(data);
      if (!schedule) return false;
      const equipment = AppState.equipment;
      equipment.schedule = schedule;
      equipment.scheduleStale = false;
      if (!equipment.draft || saved || !this.dirty() || JSON.stringify(equipment.draft) === JSON.stringify(schedule)) {
        equipment.draft = copy(schedule);
        equipment.draftBase = JSON.stringify(schedule);
      }
      return true;
    },
    dirty() {
      const { draft, draftBase } = AppState.equipment;
      return Boolean(draft && JSON.stringify(draft) !== draftBase);
    },
    conflicted() {
      const { schedule, draftBase, draft } = AppState.equipment;
      return Boolean(draft && schedule && this.dirty() && JSON.stringify(schedule) !== draftBase);
    },
    writing() {
      return Boolean(command_service.pending('set_schedule') || command_service.pending('remove_schedule'));
    },
    render() {
      const { draft, scheduleStale } = AppState.equipment;
      const locked = this.writing();
      const conflict = this.conflicted();
      const loading = Boolean(command_service.pending('get_schedule'));
      const slotWarning = this.slotWarning(draft);
      const save = $('save-schedule');
      save.disabled = !command_service.available() || locked || loading || scheduleStale || !this.dirty() || conflict || Boolean(slotWarning);
      $('load-schedule').disabled = !command_service.available() || loading || locked;
      $('discard-schedule').hidden = !this.dirty();
      $('discard-schedule').disabled = locked;
      const status = locked ? [UIController.phase(command_service.pending('set_schedule') || command_service.pending('remove_schedule')), 'info'] :
        conflict ? ['A agenda do equipamento mudou. Descarte o rascunho para editar a versão atual.', 'warning'] :
        loading ? ['Consultando agenda…', 'info'] : scheduleStale ? ['Consulte a agenda antes de salvar.', 'warning'] :
        slotWarning ? [slotWarning, 'warning'] :
        this.dirty() ? ['Alterações aguardando envio.', 'warning'] : draft ? ['Agenda confirmada pelo equipamento.', 'success'] :
        ['Aguardando a agenda do equipamento.', 'neutral'];
      UIController.setStatus($('schedule-status'), ...status);
      const signature = JSON.stringify([draft, locked]);
      if (signature === this.lastRender && draft === this.renderedDraft) return;
      this.lastRender = signature;
      this.renderedDraft = draft;
      const container = $('schedule-panels');
      const selections = [...container.querySelectorAll('.time-edit')].map((form) =>
        [form.elements.hours.value, form.elements.minutes.value]);
      container.replaceChildren();
      for (const [index, [module, info]] of Object.entries(MODULES).entries()) {
        const card = document.createElement('article');
        const schedule = draft?.[module];
        card.className = 'surface panel schedule-panel';
        card.dataset.category = module === 'feeding' ? 'food' : 'water';
        card.innerHTML = `
          <header class="schedule-heading">
            <div class="card-heading">
              <span class="icon-box"><i class="fa-solid fa-${module === 'feeding' ? 'seedling' : 'faucet-drip'}" aria-hidden="true"></i></span>
              <div><h2>${info.label}</h2><p class="muted schedule-count"></p></div>
            </div>
            <label class="check switch schedule-switch">
              <span class="schedule-mode"></span>
              <input type="checkbox" role="switch" aria-label="Rotina automática de ${info.label.toLowerCase()}">
            </label>
          </header>
          <div class="schedule-timeline" aria-hidden="true" hidden>
            <div class="schedule-track"></div>
            <div class="schedule-ticks"><span>00h</span><span>06h</span><span>12h</span><span>18h</span><span>24h</span></div>
          </div>
          <div class="schedule-slots">
            <ul class="time-list" aria-label="Horários de ${info.label.toLowerCase()}"></ul>
            <p class="muted schedule-empty" hidden>Nenhum horário adicionado.</p>
          </div>
          <form class="time-edit">
            <label>Hora <select name="hours" required><option value="">—</option>${scheduleHourOptions}</select></label>
            <label>Minutos <select name="minutes" required><option value="">—</option><option value="00">00</option><option value="30">30</option></select></label>
            <button class="button" type="submit"><i class="fa-solid fa-plus" aria-hidden="true"></i> Adicionar horário</button>
          </form>`;
        const toggle = card.querySelector('input[type="checkbox"]');
        const form = card.querySelector('.time-edit');
        const { hours: hourField, minutes: minuteField } = form.elements;
        const list = card.querySelector('ul');
        const add = form.querySelector('button');
        const mode = card.querySelector('.schedule-mode');
        const track = card.querySelector('.schedule-track');
        const full = schedule?.times.length === MAX_SCHEDULE_TIMES;
        toggle.checked = Boolean(schedule?.enabled);
        toggle.disabled = !schedule || locked;
        hourField.disabled = minuteField.disabled = add.disabled = !schedule || locked || full;
        [hourField.value, minuteField.value] = selections[index] || ['', ''];
        form.addEventListener('change', () => hourField.setCustomValidity(''));
        mode.textContent = !schedule ? 'Indisponível' : toggle.checked ? 'Ativa' : 'Pausada';
        card.querySelector('.schedule-timeline').hidden = !schedule;
        card.querySelector('.schedule-empty').hidden = !schedule || schedule.times.length > 0;
        card.querySelector('.schedule-count').textContent = schedule ?
          schedule.times.length + ' de ' + MAX_SCHEDULE_TIMES + ' horários' + (full ? ' · Limite atingido' : '') : 'Agenda ainda não recebida';
        toggle.addEventListener('change', () => {
          schedule.enabled = toggle.checked;
          mode.textContent = toggle.checked ? 'Ativa' : 'Pausada';
          this.lastRender = JSON.stringify([draft, locked]);
          this.render();
        });
        for (const slot of schedule?.times || []) {
          const [hours, minutes] = slot.split(':').map(Number);
          const mark = document.createElement('span');
          mark.className = 'schedule-mark';
          mark.style.setProperty('--slot-position', ((hours * 60 + minutes) / 1440 * 100) + '%');
          mark.title = slot;
          track.append(mark);
          const item = document.createElement('li');
          const slotTime = document.createElement('time');
          slotTime.dateTime = slot;
          slotTime.textContent = slot;
          const remove = document.createElement('button');
          remove.type = 'button';
          remove.className = 'icon-button';
          remove.disabled = locked;
          remove.setAttribute('aria-label', 'Remover horário ' + slot);
          remove.innerHTML = '<i class="fa-regular fa-trash-can" aria-hidden="true"></i>';
          remove.addEventListener('click', () => {
            schedule.times = schedule.times.filter((current) => current !== slot);
            this.render();
            container.querySelectorAll('input[type="checkbox"]')[index].focus();
          });
          item.append(slotTime, remove);
          list.append(item);
        }
        form.addEventListener('submit', (event) => {
          event.preventDefault();
          const time = hourField.value + ':' + minuteField.value;
          if (!schedule || this.writing() || schedule.times.length >= MAX_SCHEDULE_TIMES || !isScheduleSlot(time)) return;
          if (schedule.times.includes(time)) {
            hourField.setCustomValidity('Este horário já está na agenda.');
            hourField.reportValidity();
            return;
          }
          schedule.times.push(time);
          schedule.times.sort();
          form.reset();
          this.render();
          const field = container.querySelectorAll('.time-edit')[index].elements.hours;
          (field.disabled ? container.querySelectorAll('input[type="checkbox"]')[index] : field).focus();
        });
        container.append(card);
      }
    }
  };

  // ConfigManager
  const ConfigManager = {
    equipment: null,
    validate(module, values) {
      const errors = {};
      if (!isRecord(values) || Object.keys(values).length !== CONFIG_FIELDS[module].length) {
        return { schema: 'Informe todos os campos do módulo com os nomes definidos no protocolo.' };
      }
      for (const [key, label, , type] of CONFIG_FIELDS[module]) {
        const value = values[key];
        if (!Number.isFinite(value) || value < 0) errors[key] = label + ' deve ser um número maior ou igual a zero.';
        else if (type === 'percent' && value > 100) errors[key] = label + ' deve estar entre 0 e 100.';
        else if (type === 'integer' && !Number.isSafeInteger(value)) errors[key] = label + ' deve ser um número inteiro.';
      }
      if (values.low_threshold_percent >= values.full_threshold_percent) {
        errors.low_threshold_percent = 'O limiar de nível baixo deve ser menor que o de nível cheio.';
      }
      if (values.useful_height_cm > values.reference_distance_cm) {
        errors.useful_height_cm = 'A altura útil deve ser menor ou igual à distância de referência.';
      }
      if (values.cleaning_interval_seconds > MAX_CLEANING_INTERVAL_SECONDS) {
        errors.cleaning_interval_seconds = 'O intervalo de limpeza excede o limite de ' +
          numberFormat.format(MAX_CLEANING_INTERVAL_SECONDS) + ' segundos.';
      }
      return errors;
    },
    mount() {
      for (const [module, fields] of Object.entries(CONFIG_FIELDS)) {
        const form = document.createElement('form');
        form.id = 'config-' + module;
        form.className = 'surface panel config-panel';
        form.dataset.category = 'food';
        form.noValidate = true;
        form.innerHTML = `
          <h2 class="panel-title"><span class="icon-box"><i class="fa-solid fa-seedling" aria-hidden="true"></i></span></h2>
          <div class="config-fields"></div>
          <div class="config-actions">
            <p class="status-line config-status neutral" role="status"></p>
            <button class="button primary" type="submit">Salvar configuração</button>
            <button class="button" type="submit" value="target-weight">Salvar peso-alvo</button>
          </div>`;
        form.querySelector('h2').append(MODULES[module].label);
        for (const group of CONFIG_GROUPS[module]) {
          const fieldset = document.createElement('fieldset');
          fieldset.className = 'config-group';
          const legend = document.createElement('legend');
          legend.textContent = group.title;
          const grid = document.createElement('div');
          grid.className = 'field-grid';
          for (const [key, caption, initial, type, scale = 1] of group.fields) {
            const field = document.createElement('div');
            field.className = 'config-field';
            const label = document.createElement('label');
            const input = document.createElement('input');
            input.id = module + '-' + key;
            label.htmlFor = input.id;
            label.textContent = caption;
            input.type = 'number';
            input.min = '0';
            input.step = type === 'integer' && scale === 1 ? '1' : 'any';
            if (type === 'percent') input.max = '100';
            input.name = key;
            input.defaultValue = String(initial / scale);
            input.required = true;
            const error = document.createElement('p');
            error.className = 'error-text';
            error.id = input.id + '-error';
            error.hidden = true;
            input.setAttribute('aria-describedby', error.id);
            field.append(label, input, error);
            grid.append(field);
          }
          fieldset.append(legend, grid);
          form.querySelector('.config-fields').append(fieldset);
        }
        form.addEventListener('input', () => {
          AppState.equipment.configDrafts[module] = Object.fromEntries(fields.map(([key]) => [key, form.elements[key].value]));
          this.render();
        });
        form.addEventListener('submit', (event) => {
          event.preventDefault();
          const values = this.read(form, module);
          if (event.submitter?.value === 'target-weight') {
            if (isFirmwareTargetWeight(values.target_weight_g)) {
              command_service.send('set_feeding_config', { target_weight_g: values.target_weight_g }, { quiet: true });
            }
            return;
          }
          if (!Object.keys(this.validate(module, values)).length) {
            command_service.send('set_' + module + '_config', values, { quiet: true });
          }
        });
        $('config-panels').prepend(form);
      }
    },
    read(form, module) {
      return Object.fromEntries(CONFIG_FIELDS[module].map(([key, , , , scale = 1]) => {
        const value = form.elements[key].valueAsNumber;
        return [key, scale !== 1 && value >= 0 ? Math.round(value * scale) : value];
      }));
    },
    render() {
      const equipment = AppState.equipment;
      for (const [module, fields] of Object.entries(CONFIG_FIELDS)) {
        const form = $('config-' + module);
        const draft = equipment.configDrafts[module];
        const confirmed = equipment.confirmedConfig[module];
        if (this.equipment !== equipment) {
          for (const [key, , initial, , scale = 1] of fields) {
            form.elements[key].value = draft?.[key] ?? (confirmed?.[key] ?? initial) / scale;
          }
        }
        const values = this.read(form, module);
        const errors = this.validate(module, values);
        const hasErrors = Object.keys(errors).length > 0;
        const pending = command_service.pending('set_' + module + '_config');
        const same = confirmed && fields.every(([key]) => confirmed[key] === values[key]);
        const sameTarget = confirmed && confirmed.target_weight_g === values.target_weight_g;
        for (const [key] of fields) {
          const input = form.elements[key];
          input.disabled = Boolean(pending);
          input.setAttribute('aria-invalid', String(Boolean(errors[key])));
          const error = $(input.id + '-error');
          const errorMessage = errors[key] || '';
          if (error.textContent !== errorMessage) error.textContent = errorMessage;
          error.hidden = !errorMessage;
        }
        const status = form.querySelector('.config-status');
        const message = hasErrors ? [errors.schema || 'Revise os campos destacados.', 'danger'] :
          pending ? [UIController.phase(pending), 'info'] : same ? ['Configuração confirmada pelo equipamento.', 'success'] :
          sameTarget ? ['Peso-alvo confirmado pelo equipamento. Há outros campos a confirmar.', 'success'] :
          confirmed ? ['Alterações ainda não confirmadas.', 'warning'] :
          ['Valores de referência. O protocolo não consulta a configuração atual do Arduino.', 'neutral'];
        UIController.setStatus(status, ...message);
        const locked = !command_service.available() || Boolean(pending);
        form.querySelector('button[value="target-weight"]').disabled = locked || !isFirmwareTargetWeight(values.target_weight_g);
        form.querySelector('button.primary').disabled = locked || hasErrors;
      }
      this.equipment = equipment;
    }
  };

  // UIController
  const STATUS_LABELS = {
    initializing: ['Inicializando sistema', 'info', 'hourglass-start'],
    operational: ['Sistema operacional', 'success', 'circle-check'],
    alert: ['Atenção necessária', 'warning', 'triangle-exclamation'],
    error: ['Falha', 'danger', 'circle-exclamation'],
    idle: ['Em espera', 'neutral', 'circle-pause'],
    checking: ['Verificando sensores', 'warning', 'magnifying-glass'],
    running: ['Em operação física', 'warning', 'hourglass-half'],
    completed: ['Ciclo concluído', 'success', 'circle-check'],
    available: ['RTC sincronizado', 'success', 'clock'],
    invalid: ['Horário não validado · sincronize o RTC', 'warning', 'clock'],
    failure: ['Falha no módulo RTC', 'danger', 'circle-exclamation']
  };
  const UIController = {
    phase(command) {
      return command?.state === 'waiting_ack' ? 'Aguardando ACK (máx 5s)…' : 'Executando no Arduino (máx 15s)…';
    },
    openView(view) {
      const section = $('view-' + view);
      if (!section) return;
      AppState.view = view;
      for (const button of document.querySelectorAll('#navigation [data-view]')) {
        const active = button.dataset.view === view;
        button.toggleAttribute('aria-current', active);
        if (active) button.setAttribute('aria-current', 'page');
        const page = $('view-' + button.dataset.view);
        if (page) page.hidden = !active;
      }
      if (AppState.signedIn && !$('dashboard-app').hidden) {
        $(view + '-title').focus({ preventScroll: true });
        window.scrollTo(0, 0);
      }
    },
    setStatus(element, text, tone, icon = STATUS_ICONS[tone]) {
      const signature = [text, tone, icon].join('|');
      if (element.dataset.badge === signature) return;
      element.dataset.badge = signature;
      element.classList.remove(...Object.keys(STATUS_ICONS));
      element.classList.add(tone);
      element.replaceChildren(makeIcon(icon, ' status-icon'), text);
    },
    mountReservoirs() {
      for (const [key, name, icon] of [['food', 'Reservatório de ração', 'box-archive'], ['water', 'Reservatório de água', 'water']]) {
        const article = document.createElement('article');
        article.id = 'reservoir-' + key;
        article.className = 'surface measurement reservoir';
        article.dataset.category = key;
        article.innerHTML = `
          <div class="reservoir-details">
            <span class="icon-box"><i class="fa-solid fa-${icon}" aria-hidden="true"></i></span>
            <div><h3>${name}</h3><span class="badge" role="status"></span></div>
          </div>
          <div class="gauge">
            <svg viewBox="0 0 120 120" aria-hidden="true">
              <circle class="track" cx="60" cy="60" r="51"></circle>
              <circle class="level" cx="60" cy="60" r="51" pathLength="100"></circle>
            </svg>
            <strong></strong>
          </div>
          <details class="reading-disclosure">
            <summary class="reading-toggle" aria-label="Detalhes da leitura do ${name.toLowerCase()}">
              Detalhes da leitura <span class="reading-chevron" aria-hidden="true"></span>
            </summary>
            <dl class="reading-details">
              <div><dt>Sensor</dt><dd>Ultrassônico</dd></div>
              <div><dt>Distância medida</dt><dd class="reading-distance">Indisponível</dd></div>
            </dl>
          </details>`;
        $('reservoirs').append(article);
      }
    },
    reservoir(key, reading) {
      const article = $('reservoir-' + key);
      const valid = reading?.valid === true;
      const level = valid ? reading.level_percent : 0;
      const text = reading?.low === true ? 'Nível baixo · Reabastecer' :
        reading?.low === false ? 'Nível normal' : 'Leitura válida';
      this.readingBadge(article, reading, text, reading?.low === true ? 'warning' : 'success');
      const arc = article.querySelector('.level');
      arc.hidden = !valid || level === 0;
      arc.setAttribute('visibility', valid && level > 0 ? 'visible' : 'hidden');
      arc.setAttribute('stroke-dasharray', level + ' 100');
      article.querySelector('.gauge strong').textContent = valid ? numberFormat.format(level) + '%' : '—';
      article.querySelector('.reading-distance').textContent = valid && reading.distance_cm !== undefined ?
        numberFormat.format(reading.distance_cm) + ' cm' : 'Indisponível';
    },
    readingBadge(article, reading, text, tone) {
      if (!reading) {
        text = AppState.equipment.updatedAt ? 'Leitura indisponível' : 'Aguardando leitura';
        tone = 'neutral';
      } else if (reading.stale) {
        text = 'Leitura desatualizada';
        tone = 'warning';
      } else if (!reading.valid) {
        text = 'Leitura inválida';
        tone = 'danger';
      }
      this.setStatus(article.querySelector('.badge'), text, tone);
    },
    renderContainers({ food_bowl: food, water_bowl: water }) {
      const foodCard = $('food-bowl');
      const waterCard = $('water-bowl');
      const targetWeight = AppState.equipment.confirmedConfig.feeding?.target_weight_g;
      const hasTargetWeight = Number.isFinite(targetWeight) && targetWeight > 0;
      const limit = targetWeight / 5;
      // Evita um aviso no próprio limiar por arredondamento de ponto flutuante.
      const foodLow = hasTargetWeight && food?.weight_g < limit - limit * Number.EPSILON;
      const low = water?.level_state === 'low';
      const foodText = !hasTargetWeight ? 'Peso-alvo não confirmado' : foodLow ? 'Nível baixo' : 'Leitura válida';
      this.readingBadge(foodCard, food, foodText, !hasTargetWeight ? 'neutral' : foodLow ? 'warning' : 'success');
      this.readingBadge(waterCard, water, 'Leitura da boia', low ? 'warning' : 'success');
      foodCard.querySelector('.reading-value').textContent = food?.valid ? numberFormat.format(food.weight_g) + ' g' : '—';
      const waterValue = waterCard.querySelector('.reading-value');
      const level = water?.valid ? (low ? 'Baixo' : 'Adequado') : '—';
      if (waterValue.textContent !== level) waterValue.textContent = level;
    },
    render() {
      $('login').hidden = AppState.signedIn;
      $('dashboard-app').hidden = !AppState.signedIn;
      $('connect').disabled = AppState.connection === 'connecting';
      $('connect-label').textContent = AppState.connection === 'connecting' ? 'Conectando ao EMQX…' : 'Conectar à gaiola';
      if (!AppState.signedIn) return;
      const simulated = Boolean(diagnostics.active?.());
      const { equipment, connection, settings, history } = AppState;
      const unread = history.filter((entry) => entry.unread).length;
      const connectionLabel = {
        online: ['Conectado ao EMQX', 'success', 'link'],
        reconnecting: ['Reconectando…', 'warning', 'hourglass-half'],
        offline: ['Desconectado', 'danger', 'link-slash']
      };
      this.setStatus($('connection-badge'), ...connectionLabel[connection]);
      $('active-device').textContent = settings.deviceId;
      $('reconnect-banner').hidden = connection !== 'reconnecting';
      $('simulation-banner').hidden = !simulated;
      $('missed-banner').hidden = !equipment.missed.length;
      const missed = equipment.missed[0];
      if (missed) {
        const extra = equipment.missed.length - 1;
        $('missed-summary').textContent = 'Rotina perdida · ' + MODULES[missed.module].label + ' às ' + missed.scheduled_time +
          (extra ? ' · +' + extra + ' pendente(s)' : '');
      }
      for (const badge of document.querySelectorAll('[data-unread]')) {
        badge.hidden = !unread;
        badge.textContent = unread > 99 ? '99+' : String(unread);
        badge.parentElement.setAttribute('aria-label', 'Notificações' + (unread ? ' · ' + unread + ' não lidas' : ''));
      }
      for (const key of ['system', 'rtc', 'feeding', 'watering']) {
        const state = equipment.status[key];
        this.setStatus($(key + '-state'), ...(owns(STATUS_LABELS, state) ? STATUS_LABELS[state] : ['Aguardando status', 'neutral']));
      }
      const cleaning = equipment.cleaningDue;
      const interval = equipment.confirmedConfig.feeding?.cleaning_interval_seconds;
      const intervalLabel = interval % SECONDS_PER_HOUR === 0 ?
        numberFormat.format(interval / SECONDS_PER_HOUR) + ' h' : numberFormat.format(interval) + ' s';
      const cleaningText = cleaning === true ? (interval == null ? 'Limpeza necessária · Prazo atingido' : 'Limpeza necessária · Prazo de ' + intervalLabel + ' atingido') :
        cleaning === false ? 'Higienização em dia' : 'Aguardando status';
      this.setStatus($('cleaning-state'), cleaningText, cleaning === true ? 'warning' : cleaning === false ? 'success' : 'neutral');
      this.reservoir('food', equipment.reservoirs.food);
      this.reservoir('water', equipment.reservoirs.water);
      this.renderContainers(equipment.containers);
      $('pump-status').textContent = equipment.pumpOn === null ? 'Comando da bomba indisponível' :
        equipment.pumpOn ? 'Comando da bomba ativo' : 'Comando da bomba inativo';
      const lastUpdate = equipment.updatedAt;
      const stale = [...Object.values(equipment.reservoirs), ...Object.values(equipment.containers)].some((reading) => reading.stale);
      $('last-update').textContent = lastUpdate ?
        'Última atualização recebida às ' + lastUpdate.toLocaleTimeString('pt-BR') + (stale ? ' · Há leituras desatualizadas' : '') :
        'Aguardando a primeira leitura do equipamento.';
      const info = equipment.systemInfo;
      const firmware = typeof info.firmware_version === 'string' ? info.firmware_version : '—';
      const protocol = typeof info.protocol_version === 'string' ? info.protocol_version : '—';
      $('firmware-footer').textContent = 'Firmware ' + firmware + ' · Protocolo ' + protocol;
      diagnostics.render?.();
      this.renderControls();
      ScheduleManager.render();
      ConfigManager.render();
    },
    renderControls() {
      const available = command_service.available();
      for (const [module, info] of Object.entries(MODULES)) {
        const pending = command_service.pending(info.action);
        const running = AppState.equipment.status[module] === 'running';
        const button = $(info.button);
        button.disabled = !command_service.canActuate(info.action);
        this.buttonText(button, pending ? this.phase(pending) : running ? 'Em operação física…' : info.caption, pending || running);
      }
      for (const [id, action, label] of [
        ['sync', 'sync_time', 'Sincronizar relógio (RTC)'], ['cleaning', 'ack_cleaning', 'Confirmar limpeza realizada']
      ]) {
        const pending = command_service.pending(action);
        $(id).disabled = !available || Boolean(pending);
        this.buttonText($(id), pending ? this.phase(pending) : label, pending);
      }
      $('refresh').disabled = !available || Boolean(command_service.pending('get_status') || command_service.pending('get_telemetry'));
      const dialog = $('action-dialog');
      if (dialog.open) {
        const action = dialog.dataset.kind === 'missed' ?
          MODULES[AppState.equipment.missed[0]?.module]?.action : dialog.dataset.kind;
        $('action-confirm').disabled = !action || !command_service.canActuate(action);
      }
    },
    buttonText(button, text, busy) {
      if (button.textContent === text && Boolean(button.querySelector('i')) === Boolean(busy)) return;
      button.textContent = text;
      if (busy) button.prepend(makeIcon('hourglass-half'));
    },
    refresh() {
      if (!command_service.available()) return false;
      const status = command_service.send('get_status');
      const telemetry = command_service.send('get_telemetry');
      return status || telemetry;
    },
    confirmManual(action) {
      if (!command_service.canActuate(action)) return;
      if (AppState.equipment.missed.length) {
        event_handler.review();
        return;
      }
      const feeding = action === 'feed_manual';
      ModalService.open({
        title: feeding ? 'Confirmar alimentação manual' : 'Confirmar hidratação manual',
        description: diagnostics.active?.() ? 'Iniciar este ciclo no simulador?' :
          feeding ? 'Enviar ao equipamento a solicitação para dispensar ração agora?' :
            'Enviar ao equipamento a solicitação para repor água agora?',
        kind: action, confirm: feeding ? 'Alimentar agora' : 'Repor água agora', cancel: 'Cancelar',
        onClose: (decision) => { if (decision === 'confirm') command_service.send(action); }
      });
      this.renderControls();
    },
    rememberAccess() {
      try {
        if (AppState.settings.remember) localStorage.setItem(STORAGE_KEY, JSON.stringify(AppState.settings));
        else localStorage.removeItem(STORAGE_KEY);
      } catch {
        if (AppState.settings.remember) ToastService.show('Acesso não salvo', 'O navegador não permitiu armazenar as credenciais.', 'warning');
      }
    },
    restoreAccess(form) {
      try {
        const saved = JSON.parse(localStorage.getItem(STORAGE_KEY));
        if (!isRecord(saved) || saved.remember !== true) return;
        for (const key of ['username', 'password', 'host', 'port', 'path', 'deviceId']) {
          if (typeof saved[key] === 'string' || typeof saved[key] === 'number') form.elements[key].value = saved[key];
        }
        form.elements.remember.checked = true;
      } catch { /* O armazenamento pode estar indisponível. */ }
    },
    submitLogin(event) {
      event.preventDefault();
      if (AppState.connection === 'connecting') return;
      const values = new FormData(event.currentTarget);
      const settings = {
        username: values.get('username').trim(), password: values.get('password'), remember: values.has('remember'),
        host: values.get('host').trim(), port: Number(values.get('port')), path: values.get('path').trim(),
        deviceId: values.get('deviceId').trim()
      };
      if (!settings.username || !settings.password || !/^[a-z\d.-]+$/i.test(settings.host) ||
        !Number.isInteger(settings.port) || settings.port < 1 || settings.port > 65535 ||
        !/^\/[^\s#]*$/.test(settings.path) || !/^[a-z\d_-]+$/i.test(settings.deviceId)) {
        $('login-error').textContent = 'Verifique os campos. Use um domínio sem protocolo ou porta, um path iniciado por / e um ID de dispositivo válido.';
        $('login-error').hidden = false;
        return;
      }
      $('login-error').hidden = true;
      AppState.connection = 'connecting';
      AppState.settings = settings;
      this.render();
      mqtt_client.connect();
    },
    logout() {
      AppState.signedIn = false;
      mqtt_client.closeClient();
      command_service.cancelAll();
      AppState.connection = 'offline';
      AppState.settings = null;
      AppState.equipment = equipmentState();
      AppState.history = [];
      diagnostics.reset?.();
      ModalService.clear();
      if ($('events-dialog').open) $('events-dialog').close();
      $('toasts').replaceChildren();
      $('login-error').hidden = true;
      $('login-form').reset();
      $('password').type = 'password';
      $('show-password').setAttribute('aria-pressed', 'false');
      $('show-password').setAttribute('aria-label', 'Mostrar senha');
      $('show-password').querySelector('i').className = 'fa-solid fa-eye';
      try { localStorage.removeItem(STORAGE_KEY); } catch { /* O armazenamento pode estar bloqueado. */ }
      this.render();
      $('username').focus();
    },
    mount() {
      const form = $('login-form');
      for (const [name, value] of Object.entries(CONFIG_DEFAULTS)) form.elements[name].value = value;
      this.restoreAccess(form);
      form.addEventListener('submit', (event) => this.submitLogin(event));
      // Revela os campos avançados quando a validação nativa encontra um campo inválido.
      form.addEventListener('invalid', (event) => {
        const details = event.target.closest('details');
        if (details) details.open = true;
      }, true);
      $('show-password').addEventListener('click', (event) => {
        const button = event.currentTarget;
        const show = $('password').type === 'password';
        $('password').type = show ? 'text' : 'password';
        button.setAttribute('aria-pressed', String(show));
        button.setAttribute('aria-label', show ? 'Ocultar senha' : 'Mostrar senha');
        button.querySelector('i').className = 'fa-solid ' + (show ? 'fa-eye-slash' : 'fa-eye');
      });
      this.mountReservoirs();
      ConfigManager.mount();
      $('logout').addEventListener('click', () => this.logout());
      $('navigation').addEventListener('click', (event) => {
        const button = event.target.closest('[data-view]');
        if (button) this.openView(button.dataset.view);
      });
      $('refresh').addEventListener('click', this.refresh);
      $('sync').addEventListener('click', () => time_service.sync());
      $('cleaning').addEventListener('click', () => command_service.send('ack_cleaning'));
      for (const info of Object.values(MODULES)) $(info.button).addEventListener('click', () => this.confirmManual(info.action));
      $('load-schedule').addEventListener('click', () => command_service.send('get_schedule', {}, { quiet: true }));
      $('save-schedule').addEventListener('click', () => {
        if (!$('save-schedule').disabled) command_service.send('set_schedule', AppState.equipment.draft, { quiet: true });
      });
      $('discard-schedule').addEventListener('click', () => {
        if (ScheduleManager.writing() || !AppState.equipment.schedule) return;
        const stale = AppState.equipment.scheduleStale;
        ScheduleManager.load(AppState.equipment.schedule, true);
        AppState.equipment.scheduleStale = stale;
        ScheduleManager.render();
        $('load-schedule').focus();
      });
      $('review-missed').addEventListener('click', () => event_handler.review());
      $('action-dialog').addEventListener('submit', (event) => {
        event.preventDefault();
        if (event.submitter === $('action-confirm')) ModalService.decide('confirm');
        if (event.submitter === $('action-cancel')) ModalService.decide('cancel');
      });
      $('action-dialog').addEventListener('cancel', (event) => {
        event.preventDefault();
        ModalService.decide('');
      });
      for (const button of document.querySelectorAll('[data-open-events]')) {
        button.addEventListener('click', () => {
          for (const entry of AppState.history) entry.unread = false;
          event_handler.renderHistory();
          $('events-dialog').showModal();
          this.render();
        });
      }
      $('events-dialog').addEventListener('close', () => event_handler.review());
      diagnostics.mount?.();
      this.render();
    }
  };

  /* === INÍCIO DO MÓDULO DE TESTES E SIMULAÇÃO (PODE SER REMOVIDO EM PRODUÇÃO) === */
  const TerminalLogger = {
    paused: false,
    log(direction, topic, payload, transport) {
      const terminal = $('terminal');
      const entry = document.createElement('div');
      entry.className = 'terminal-entry ' + direction.toLowerCase();
      const title = document.createElement('strong');
      const route = direction === 'TX' ? 'TX Web → Arduino' : 'RX Arduino → Web';
      title.textContent = '[' + route + '] ' + new Date().toLocaleTimeString('pt-BR') + ' · ' + transport + ' · ' + topic;
      const code = document.createElement('code');
      code.textContent = payload;
      entry.append(title, code);
      terminal.append(entry);
      if (terminal.childElementCount > 250) terminal.firstElementChild.remove();
      if (!this.paused) terminal.scrollTop = terminal.scrollHeight;
    }
  };
  const mock_simulator = {
    realEquipment: null,
    schedule: null,
    pumpFailure: false,
    timers: new Set(),
    active() { return this.realEquipment !== null; },
    reset() {
      for (const timer of this.timers) clearTimeout(timer);
      this.timers.clear();
      this.realEquipment = null;
      this.schedule = null;
      this.pumpFailure = false;
      $('simulate').checked = false;
      $('terminal').replaceChildren();
      $('manual-form').reset();
      TerminalLogger.paused = false;
      $('pause-terminal').setAttribute('aria-pressed', 'false');
      $('pause-terminal').textContent = 'Pausar scroll';
    },
    switchMode(enabled) {
      if (enabled === this.active() || pendingCommands.size) return;
      ModalService.clear();
      $('toasts').replaceChildren();
      if (enabled) {
        this.realEquipment = AppState.equipment;
        AppState.equipment = equipmentState();
        this.schedule = {
          feeding: { enabled: true, times: ['08:00', '18:00'] },
          watering: { enabled: true, times: ['08:00', '18:00'] }
        };
      } else {
        AppState.equipment.eventScope.closed = true;
        AppState.equipment = this.realEquipment;
        this.realEquipment = null;
        this.pumpFailure = false;
        telemetry_handler.invalidateFreshness(AppState.equipment);
        AppState.equipment.scheduleStale = Boolean(AppState.equipment.schedule);
        AppState.equipment.status = {};
      }
      UIController.render();
      //if (command_service.available()) mqtt_client.initialize();
      if (!enabled) event_handler.review();
    },
    emit(type, fields) {
      const message = JSON.stringify({ v: '1.0', type, source: 'arduino', ...fields });
      response_handler.receive(message, mqtt_client.topic('status'), 'Simulador');
    },
    after(delay, run) {
      const timer = setTimeout(() => {
        this.timers.delete(timer);
        if (this.active()) run();
      }, delay);
      this.timers.add(timer);
    },
    telemetry() {
      const equipment = AppState.equipment;
      const foodBowl = equipment.containers.food_bowl || { weight_g: 34.5, valid: true };
      const waterBowl = equipment.containers.water_bowl || { level_state: 'ok', low: false, valid: true };
      return {
        reservoirs: {
          food: equipment.reservoirs.food || { level_percent: 76, distance_cm: 12.4, valid: true, low: false },
          water: equipment.reservoirs.water || { level_percent: 62, distance_cm: 15.1, valid: true, low: false }
        },
        containers: { food_bowl: foodBowl, water_bowl: waterBowl },
        feeding: { state: equipment.status.feeding || 'idle', bowl_weight_g: foodBowl.weight_g },
        watering: { state: equipment.status.watering || 'idle', bowl_level: waterBowl.level_state, pump_on: equipment.pumpOn ?? false },
        maintenance: { cleaning_due: Boolean(equipment.cleaningDue) }
      };
    },
    pumpError() {
      return { code: 'WATER_SENSOR_FAILURE', message: 'Water level could not be verified safely' };
    },
    wateringError(telemetry, starting) {
      const bowl = telemetry.containers.water_bowl;
      const reservoir = telemetry.reservoirs.water;
      if (!bowl.valid) return this.pumpError();
      if (!reservoir.valid) return { code: 'RESERVOIR_SENSOR_FAILURE', message: 'Source water reservoir reading is invalid' };
      if (reservoir.low !== false) return { code: 'SAFETY_BLOCK', message: 'Source water reservoir availability is not confirmed' };
      if (starting && bowl.level_state !== 'low') return { code: 'SAFETY_BLOCK', message: 'Water bowl level is already adequate' };
      return null;
    },
    send(command) {
      const watering = command.action === 'water_manual';
      const feeding = command.action === 'feed_manual';
      this.after(400, () => {
        if (!pendingCommands.has(command.id)) return;
        const telemetry = watering ? this.telemetry() : null;
        let error = null;
        if (watering) error = this.wateringError(telemetry, true);
        if (feeding) error = feedingSafetyError(AppState.equipment);
        if (error) {
          this.emit('ack', { id: command.id, status: 'rejected', error });
          return;
        }
        this.emit('ack', { id: command.id, status: 'accepted' });
        if (watering && !this.pumpFailure) {
          telemetry.watering.state = 'running';
          telemetry.watering.pump_on = true;
          this.emit('telemetry', { data: telemetry });
        }
      });
      this.after(2000, () => {
        if (!pendingCommands.has(command.id)) return;
        if (watering) {
          const telemetry = this.telemetry();
          const error = this.pumpFailure ? this.pumpError() : this.wateringError(telemetry, false);
          this.pumpFailure = false;
          telemetry.watering.pump_on = false;
          telemetry.watering.state = error ? 'error' : 'completed';
          if (!error) {
            telemetry.containers.water_bowl = { level_state: 'ok', low: false, valid: true };
            telemetry.watering.bowl_level = 'ok';
          }
          this.emit('telemetry', { data: telemetry });
          if (error) {
            this.emit('response', { id: command.id, status: 'failure', error });
            return;
          }
        }
        const data = this.result(command);
        this.emit('response', { id: command.id, status: 'success', data });
      });
    },
    result({ action, data }) {
      switch (action) {
        case 'get_status': {
          const { status, cleaningDue } = AppState.equipment;
          const { reservoirs, containers } = this.telemetry();
          return {
            system: 'operational', feeding: status.feeding || 'idle', watering: status.watering || 'idle',
            reservoirs, containers,
            maintenance: cleaningDue ? 'cleaning_due' : 'normal', rtc: status.rtc || 'available'
          };
        }
        case 'get_telemetry': return this.telemetry();
        case 'get_schedule': return this.schedule;
        case 'get_system_info':
          return { device: 'smartbird', controller: 'arduino_mega_2560', firmware_version: '0.1.0', protocol_version: '1.0' };
        case 'feed_manual': return { module: 'feeding', result: 'completed' };
        case 'water_manual': return { module: 'watering', result: 'completed' };
        case 'ack_cleaning': return { cleaning_due: false, maintenance_timer_reset: true };
        case 'set_schedule':
          this.schedule = copy(data);
          return { schedule_updated: true };
        case 'remove_schedule':
          this.schedule[data.module].times = this.schedule[data.module].times.filter((time) => time !== data.time);
          return { ...data, removed: true };
        case 'set_feeding_config':
          return Object.keys(data).length === 1 ? { configuration_updated: true, target_weight_g: data.target_weight_g } :
            { configuration_updated: true };
        case 'sync_time': return { rtc_updated: true, datetime: data.datetime };
      }
    },
    scenario(name) {
      if (!this.active()) return;
      const timestamp = time_service.getLocalISOStringWithOffset();
      const telemetry = this.telemetry();
      switch (name) {
        case 'normal':
          telemetry.reservoirs = {
            food: { level_percent: 76, distance_cm: 12.4, valid: true, low: false },
            water: { level_percent: 62, distance_cm: 15.1, valid: true, low: false }
          };
          telemetry.containers = {
            food_bowl: { weight_g: 34.5, valid: true },
            water_bowl: { level_state: 'ok', low: false, valid: true }
          };
          telemetry.feeding = { state: 'idle', bowl_weight_g: 34.5 };
          telemetry.watering = { state: 'idle', bowl_level: 'ok', pump_on: false };
          this.pumpFailure = false;
          this.emit('telemetry', { timestamp, data: telemetry });
          break;
        case 'invalid':
          telemetry.reservoirs.food = { valid: false };
          this.emit('telemetry', { timestamp, data: telemetry });
          break;
        case 'low':
        case 'source-water-low': {
          const reservoir = name === 'low' ? 'food' : 'water';
          const alreadyLow = telemetry.reservoirs[reservoir].low === true;
          telemetry.reservoirs[reservoir] = { level_percent: 15, valid: true, low: true };
          this.emit('telemetry', { timestamp, data: telemetry });
          if (!alreadyLow) this.emit('event', { timestamp, data: { name: 'reservoir_low', reservoir, level_percent: 15 } });
          break;
        }
        case 'bowl-low': {
          const alreadyLow = telemetry.containers.water_bowl.level_state === 'low';
          telemetry.containers.water_bowl = { level_state: 'low', low: true, valid: true };
          telemetry.watering.bowl_level = 'low';
          this.emit('telemetry', { timestamp, data: telemetry });
          if (!alreadyLow) this.emit('event', { timestamp, data: { name: 'container_level_low', container: 'water_bowl', level_state: 'low' } });
          break;
        }
        case 'food-bowl-low': {
          const targetWeight = AppState.equipment.confirmedConfig.feeding?.target_weight_g ?? 20;
          const weight_g = targetWeight * 0.195;
          telemetry.containers.food_bowl = { weight_g, valid: true };
          telemetry.feeding.bowl_weight_g = weight_g;
          this.emit('telemetry', { timestamp, data: telemetry });
          break;
        }
        case 'bowls-invalid':
          telemetry.containers = { food_bowl: { valid: false }, water_bowl: { valid: false } };
          delete telemetry.feeding.bowl_weight_g;
          delete telemetry.watering.bowl_level;
          telemetry.watering.pump_on = false;
          this.emit('telemetry', { timestamp, data: telemetry });
          break;
        case 'cleaning':
          this.emit('event', { timestamp, data: { name: 'cleaning_due' } });
          break;
        case 'missed':
          this.emit('event', { timestamp, data: {
            name: 'scheduled_execution_missed', module: 'feeding', scheduled_time: '14:00', action_required: 'user_decision'
          } });
          break;
        case 'pump':
          this.pumpFailure = true;
          telemetry.containers.water_bowl = { level_state: 'low', low: true, valid: true };
          telemetry.watering = { state: 'error', bowl_level: 'low', pump_on: false };
          this.emit('telemetry', { timestamp, data: telemetry });
          this.emit('error', { error: this.pumpError() });
          break;
      }
    }
  };

  function submitDiagnostic(event) {
    event.preventDefault();
    let input;
    try {
      input = JSON.parse($('manual-json').value);
    } catch {
      ToastService.show('JSON inválido', 'Informe um objeto JSON válido.', 'danger');
      return;
    }
    if (!isRecord(input) || typeof input.action !== 'string' || !isRecord(input.data)) {
      ToastService.show('Comando inválido', 'Informe uma action em texto e um objeto data.', 'danger');
      return;
    }
    const { action, data } = input;
    if (action === 'set_watering_config') {
      ToastService.show('Configuração indisponível', 'A configuração de hidratação pela Web está pendente no protocolo.', 'warning');
      return;
    }
    const emptyActions = ['get_status', 'get_telemetry', 'get_schedule', 'get_system_info', 'feed_manual', 'water_manual', 'ack_cleaning'];
    let payload = null;
    if (emptyActions.includes(action) && !Object.keys(data).length) payload = data;
    if (action === 'set_schedule') payload = data;
    if (action === 'remove_schedule' && Object.keys(data).length === 2 && owns(MODULES, data.module) && isTime(data.time)) payload = data;
    if (action === 'sync_time' && Object.keys(data).length === 1 && typeof data.datetime === 'string' &&
      /^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}[+-]\d{2}:\d{2}$/.test(data.datetime) && Number.isFinite(Date.parse(data.datetime))) payload = data;
    for (const module of Object.keys(CONFIG_FIELDS)) {
      if (action === 'set_' + module + '_config' && !Object.keys(ConfigManager.validate(module, data)).length) payload = data;
    }
    if (action === 'set_feeding_config' && Object.keys(data).length === 1 && isFirmwareTargetWeight(data.target_weight_g)) {
      payload = data;
    }
    if (!payload) {
      ToastService.show('Comando inválido', 'Use uma ação do protocolo v1.0 e os parâmetros previstos para ela.', 'danger');
      return;
    }
    if (['feed_manual', 'water_manual'].includes(action)) {
      if (command_service.canActuate(action)) UIController.confirmManual(action);
      else ToastService.show('Comando indisponível', 'Aguarde o equipamento ficar disponível para acionar o módulo.', 'warning');
      return;
    }
    command_service.send(action, payload);
  }

  Object.assign(diagnostics, {
    active: () => mock_simulator.active(),
    log: (...args) => TerminalLogger.log(...args),
    send: (command) => mock_simulator.send(command),
    reset: () => mock_simulator.reset(),
    render() {
      const info = AppState.equipment.systemInfo;
      $('system-info').textContent = typeof info.controller === 'string' ?
        info.controller + ' · Firmware ' + info.firmware_version + ' · Protocolo ' + info.protocol_version :
        'Aguardando identificação do equipamento.';
      $('simulate').checked = mock_simulator.active();
      $('simulate').disabled = pendingCommands.size > 0;
      for (const button of $('scenarios').querySelectorAll('button')) button.disabled = !mock_simulator.active();
      $('send-manual').disabled = !command_service.available();
    },
    mount() {
      $('simulate').addEventListener('change', (event) => mock_simulator.switchMode(event.target.checked));
      $('scenarios').addEventListener('click', (event) => {
        const button = event.target.closest('[data-scenario]');
        if (button) mock_simulator.scenario(button.dataset.scenario);
      });
      $('clear-terminal').addEventListener('click', () => $('terminal').replaceChildren());
      $('pause-terminal').addEventListener('click', (event) => {
        TerminalLogger.paused = !TerminalLogger.paused;
        event.currentTarget.setAttribute('aria-pressed', String(TerminalLogger.paused));
        event.currentTarget.textContent = TerminalLogger.paused ? 'Retomar scroll' : 'Pausar scroll';
        if (!TerminalLogger.paused) $('terminal').scrollTop = $('terminal').scrollHeight;
      });
      $('manual-form').addEventListener('submit', submitDiagnostic);
    }
  });
  /* === FIM DO MÓDULO DE TESTES E SIMULAÇÃO === */

  UIController.mount();
})();
