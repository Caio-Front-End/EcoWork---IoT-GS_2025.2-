# EcoWork - Sistema Inteligente de Gestão de Salas e Sustentabilidade

![Status do Projeto](https://img.shields.io/badge/Status-Concluído-brightgreen)
![Plataforma](https://img.shields.io/badge/Plataforma-ESP32-blue)

## 👨‍💻 Integrantes da Equipe
* **Caio Nascimento Battista** - RM561383
* **Manoah Leão** - RM563713

---

## 📝 Descrição do Projeto

### O Problema
Muitos escritórios e ambientes corporativos desperdiçam energia elétrica mantendo luzes artificiais e sistemas de climatização ligados em salas vazias ou em momentos onde a luz natural seria suficiente.

### A Solução: EcoWork
O **EcoWork** é um sistema IoT baseado em ESP32 focado na eficiência energética. Ele monitora a ocupação da sala e as condições ambientais para automatizar o controle de dispositivos, garantindo economia sem sacrificar o conforto.

**Principais Funcionalidades:**
1.  [cite_start]**Detecção de Presença (PIR):** Ativa o sistema apenas quando a sala está ocupada[cite: 10, 25].
2.  [cite_start]**Modo ECO Automático:** Se não houver movimento por um tempo determinado (configurado para 10s em testes), o sistema desliga relés e luzes[cite: 12, 27].
3.  **Light Harvesting (Colheita de Luz):** Ajusta a intensidade da iluminação LED (via PWM) inversamente à luz natural detectada pelo LDR. [cite_start]Se houver muita luz solar, a luz artificial é reduzida ou desligada [cite: 28-31].
4.  [cite_start]**Monitoramento Climático:** Alerta se a temperatura ultrapassar 24°C enquanto a sala estiver ocupada[cite: 32].
5.  **Dashboard em Tempo Real:** Integração via MQTT com Node-RED para visualização de dados.

---

## 🛠️ Hardware e Tecnologias

* **Microcontrolador:** ESP32
* **Sensores:**
    * DHT22 (Temperatura e Umidade)
    * PIR (Sensor de Movimento)
    * LDR (Sensor de Luz - Fotoresistor)
* **Atuadores:**
    * Relé (Simulando Ar Condicionado/Tomadas gerais)
    * LED (Simulando iluminação dimerizável)
    * Display LCD 16x2 I2C
* **Protocolos:** MQTT, WiFi.

---

## 🔌 Documentação da Interface MQTT

O dispositivo publica dados periodicamente no broker MQTT público.

* [cite_start]**Broker:** `44.223.43.74` [cite: 8]
* **Porta:** `1883`
* [cite_start]**Tópico de Publicação:** `ecowork/sala01` [cite: 9]

### Estrutura do Payload (JSON)
[cite_start]O dispositivo envia uma string JSON a cada 2 segundos (para fins de teste) com o seguinte formato[cite: 38, 39]:

```json
{
  "sala": "Sala01",
  "status": "Ocupada",    // "Ocupada" ou "Livre"
  "temp": 25.5,           // Temperatura em Celsius
  "hum": 60.0,            // Umidade Relativa em %
  "luz_nat": 85,          // Luz Natural em % (0-100)
  "luz_art": 0,           // Nível do PWM do LED (0-255)
  "alerta": 1             // 1 se Temp > 24°C, 0 caso contrário
}
