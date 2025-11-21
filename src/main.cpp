/*
 * PROJETO: Eco-Work (Sustentabilidade e Gestão de Salas)
 * - PIR: Detecta presença. Zera o timer de ociosidade.
 * - TEMPO SEM MOVIMENTO: > 10 seg (para teste) desliga RELÉ e LUZES.
 * - LDR: Se ocupado + muita luz externa -> Diminui LED (Harvesting).
 * - DHT: Se ocupado + Temp > 24C -> Flag de Alerta.
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// --- Configurações de Rede e MQTT ---
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "44.223.43.74"; // Broker público
const int mqtt_port = 1883;
const char* mqtt_topic = "ecowork/sala01"; // Tópico atualizado

WiFiClient espClient;
PubSubClient client(espClient);

// --- Hardware ---
LiquidCrystal_I2C lcd(0x27, 16, 2);
#define PIR_PIN 26
#define DHTPIN 27
#define DHTTYPE DHT22
#define LDR_PIN 34     // Sensor de Luz
#define RELAY_PIN 5    // Controle de Ar/Energia Geral
#define LED_PIN 18     // Simulação de Luz Dimerizável

DHT dht(DHTPIN, DHTTYPE);

// --- Variáveis de Controle ---
unsigned long lastMotionTime = 0;
// IMPORTANTE: Para teste rápido no Wokwi usei 10000 (10 segundos). 
// Para produção (10 min), use: 10 * 60 * 1000 = 600000
const unsigned long TIMEOUT_NO_MOTION = 10000; 

bool salaOcupada = false;
bool alertaTemp = false;
int luzArtificial = 0; // Valor PWM (0-255)

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando em ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
}

void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Tentando MQTT...");
    if (client.connect("EcoWork_ESP32_Sala01")) {
      Serial.println("Conectado!");
    } else {
      Serial.print("Falha rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Configuração de Pinos
  pinMode(PIR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(LDR_PIN, INPUT);

  // Estado inicial
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  dht.begin();
  lcd.init();
  lcd.backlight();
  
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) reconnect_mqtt();
  client.loop();

  // --- 1. Leitura de Sensores ---
  int movimento = digitalRead(PIR_PIN);
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int ldrValue = analogRead(LDR_PIN); // 0 (escuro) a 4095 (claro)

  // Mapear LDR para porcentagem de luz ambiente (aprox)
  int luzAmbientePct = map(ldrValue, 0, 4095, 0, 100); 
  // No Wokwi, o LDR inverte as vezes dependendo da conexão, ajuste se necessário.
  // Assumindo: 0 = escuro, 100 = muito claro (sol na janela)

  // --- 2. Lógica de Presença (Timer) ---
  if (movimento == HIGH) {
    lastMotionTime = millis(); // Reseta o cronômetro
    salaOcupada = true;
  }

  // Verifica se o tempo limite expirou
  if (salaOcupada && (millis() - lastMotionTime > TIMEOUT_NO_MOTION)) {
    salaOcupada = false;
    Serial.println("Sala Vazia por tempo excessivo - Modo ECO ativado");
  }

  // --- 3. Atuação baseada no Estado ---
  
  if (salaOcupada) {
    // Liga energia geral (Ar condicionado / Tomadas)
    digitalWrite(RELAY_PIN, HIGH); 

    // Lógica de "Light Harvesting" (Economia de luz)
    // Se tem muita luz natural (>70%), diminui a luz artificial
    if (luzAmbientePct > 70) {
      luzArtificial = 0; // Desliga luz artificial, sol é suficiente
    } else if (luzAmbientePct > 40) {
      luzArtificial = 100; // Luz média (PWM)
    } else {
      luzArtificial = 255; // Luz máxima (PWM)
    }
    analogWrite(LED_PIN, luzArtificial);

    // Lógica de Conforto Térmico
    if (temp > 24.0) {
      alertaTemp = true; // Sala cheia e quente
    } else {
      alertaTemp = false;
    }

  } else {
    // Sala vazia: Modo ECO Total
    digitalWrite(RELAY_PIN, LOW); // Desliga Ar/Tomadas
    analogWrite(LED_PIN, 0);      // Desliga Luzes
    luzArtificial = 0;
    alertaTemp = false;
  }

  // --- 4. LCD ---
  lcd.setCursor(0, 0);
  if(salaOcupada) lcd.print("OCUPADA ");
  else lcd.print("LIVRE (ECO)");

  lcd.setCursor(10, 0);
  lcd.print("T:");
  lcd.print((int)temp);
  lcd.print("C");

  lcd.setCursor(0, 1);
  lcd.print("LuzAmb:");
  lcd.print(luzAmbientePct);
  lcd.print("% ");
  
  // --- 5. Publicação MQTT ---
  // Enviar a cada 2 segundos para não flodar o broker
  static unsigned long lastMsg = 0;
  if (millis() - lastMsg > 2000) {
    lastMsg = millis();
    
    char jsonPayload[256];
    snprintf(jsonPayload, sizeof(jsonPayload),
        "{\"sala\":\"Sala01\",\"status\":\"%s\",\"temp\":%.1f,\"hum\":%.1f,\"luz_nat\":%d,\"luz_art\":%d,\"alerta\":%d}",
        salaOcupada ? "Ocupada" : "Livre",
        temp,
        hum,
        luzAmbientePct,
        luzArtificial, // Mostra o quanto estamos gastando de luz
        alertaTemp
    );

    Serial.print("MQTT: ");
    Serial.println(jsonPayload);
    client.publish(mqtt_topic, jsonPayload);
  }
}