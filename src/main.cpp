#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>

// Incluir ArduinoJson ANTES de RoboEyes para evitar conflito de macro 'N'
#include <ArduinoJson.h>
#include <FluxGarage_RoboEyes.h>

// --- CONFIGURAÇÕES ---
// Wi-Fi
const char* ssid = "Quiet House";
const char* password = "quiethouse2025@";

// Display OLED (128x64) - Pinos I2C: SDA=21, SCL=22
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SDA_PIN 21
#define SCL_PIN 22

// Inicializar I2C e display com pinos corretos
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- OBJETOS GLOBAIS ---
RoboEyes<Adafruit_SSD1306> eyes(display);

// Servo Motor SG90 - Pino: 12
const int servo_pin = 12;
Servo myServo;
int servoAngle = 90; // Ângulo inicial (posição central)

// Servidor Web na porta 80
AsyncWebServer server(80);

// Variável para armazenar texto customizado
String customText = "";
unsigned long textDisplayTime = 0;
bool showingCustomText = false;

// --- PÁGINA WEB (HTML + CSS + JavaScript) ---
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Controle do Robô</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; background-color: #282c34; color: white; text-align: center; }
        .container { max-width: 400px; margin: 20px auto; padding: 20px; background-color: #3c4049; border-radius: 10px; }
        h1 { color: #61dafb; }
        .slider-container { margin: 20px 0; }
        .slider { -webkit-appearance: none; width: 100%; height: 15px; border-radius: 5px; background: #5c6370; outline: none; }
        .slider::-webkit-slider-thumb { -webkit-appearance: none; appearance: none; width: 25px; height: 25px; border-radius: 50%; background: #61dafb; cursor: pointer; }
        .btn-group { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-top: 20px; }
        button { background-color: #61dafb; color: #282c34; border: none; padding: 15px; font-size: 16px; border-radius: 5px; cursor: pointer; transition: background-color 0.3s; }
        button:hover { background-color: #21a1f2; }
        button:active { background-color: #1a8fc9; }
        #angleValue { font-size: 18px; color: #61dafb; }
        #status { margin-top: 20px; padding: 10px; background-color: #2c3032; border-radius: 5px; font-size: 12px; color: #90ee90; }
        .text-input-section { margin: 20px 0; padding: 15px; background-color: #2c3032; border-radius: 5px; }
        .text-input-section input { width: 100%; padding: 10px; border: none; border-radius: 3px; font-size: 14px; box-sizing: border-box; }
        .text-input-section button { width: 100%; margin-top: 10px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Controle do Robô</h1>

        <div class="slider-container">
            <label for="angle">Ângulo do Servo</label>
            <input type="range" min="0" max="180" value="90" class="slider" id="angle" onchange="updateAngle(this.value)">
            <p>Ângulo: <span id="angleValue">90</span>°</p>
        </div>

        <div class="btn-group">
            <button onclick="changeFace('happy')">Feliz 😊</button>
            <button onclick="changeFace('angry')">Zangado 😠</button>
            <button onclick="changeFace('tired')">Cansado 😴</button>
            <button onclick="changeFace('default')">Neutro 😐</button>
            <button onclick="changeFace('open')">Abrir Olhos 👀</button>
            <button onclick="changeFace('close')">Fechar Olhos 💤</button>
        </div>

        <div class="text-input-section">
            <label for="customText" style="display: block; margin-bottom: 10px; color: #61dafb;">Enviar Texto para Display</label>
            <input type="text" id="customText" placeholder="Digite seu texto aqui..." maxlength="50">
            <button onclick="sendText()">Enviar Texto 📝</button>
        </div>

        <div id="status">Status: Conectando...</div>
    </div>

    <script>
        function updateAngle(value) {
            document.getElementById('angleValue').innerText = value;
            fetch('/angle?value=' + value)
                .then(response => response.text())
                .then(data => console.log('Resposta:', data))
                .catch(error => console.error('Erro:', error));
        }

        function changeFace(face) {
            console.log('Enviando comando:', face);
            fetch('/face?expression=' + face)
                .then(response => response.text())
                .then(data => {
                    console.log('Resposta recebida:', data);
                    document.getElementById('status').innerText = 'Status: ' + data;
                })
                .catch(error => {
                    console.error('Erro ao enviar comando:', error);
                    document.getElementById('status').innerText = 'Status: Erro na comunicação';
                });
        }

        function sendText() {
            const textInput = document.getElementById('customText');
            const texto = textInput.value.trim();

            if (texto === '') {
                alert('Digite algo no campo de texto!');
                return;
            }

            console.log('Enviando texto:', texto);
            fetch('/text?message=' + encodeURIComponent(texto))
                .then(response => response.text())
                .then(data => {
                    console.log('Resposta:', data);
                    document.getElementById('status').innerText = 'Texto enviado: ' + texto;
                    textInput.value = '';
                })
                .catch(error => {
                    console.error('Erro:', error);
                    document.getElementById('status').innerText = 'Erro ao enviar texto';
                });
        }

        // Permitir enviar texto ao pressionar Enter
        document.getElementById('customText').addEventListener('keypress', function(event) {
            if (event.key === 'Enter') {
                sendText();
            }
        });

        document.addEventListener('DOMContentLoaded', function() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('angle').value = data.angle;
                    document.getElementById('angleValue').innerText = data.angle;
                    document.getElementById('status').innerText = 'Status: Conectado (Ângulo: ' + data.angle + '°)';
                });
        });
    </script>
</body>
</html>
)rawliteral";

// Função para desenhar texto embaixo dos olhos
void drawTextUnderEyes(String text) {
    // Desenhar os olhos normalmente (o RoboEyes faz isso)
    eyes.update();

    // Agora adicionar o texto embaixo
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 50); // Posição embaixo dos olhos
    display.println(text);
    display.display();
}

// --- SETUP ---
void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n\n╔════════════════════════════════════════╗");
    Serial.println("║     ROBO EYES - ESP32 + OLED + SERVO   ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.println("\n[CONFIG]");
    Serial.println("  Servo Motor: Pino 12");
    Serial.println("  Display OLED: SDA=21, SCL=22");
    Serial.println("  Wi-Fi SSID: Quiet House");
    Serial.println();

    // Inicializar I2C com pinos corretos ANTES do display
    Serial.println("[STEP 1] Inicializando I2C (SDA=21, SCL=22)...");
    Wire.begin(SDA_PIN, SCL_PIN);
    delay(100);
    Serial.println("  ✓ I2C OK!");

    // Inicializa o display
    Serial.println("[STEP 2] Inicializando Display OLED...");
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("  ✗ ERRO: Display não respondeu!");
        for(;;);
    }
    Serial.println("  ✓ Display OK!");

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Robo Eyes");
    display.println("Iniciando...");
    display.display();

    // Inicializar RoboEyes
    Serial.println("[STEP 3] Inicializando RoboEyes...");
    eyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 30); // 30 FPS
    eyes.setAutoblinker(ON, 3, 2);
    eyes.setIdleMode(ON, 2, 2);
    eyes.open();
    Serial.println("  ✓ RoboEyes OK!");

    // Configura o servo
    Serial.println("[STEP 4] Inicializando Servo (Pino 12)...");
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    myServo.setPeriodHertz(50);
    myServo.attach(servo_pin, 500, 2400);
    myServo.write(servoAngle);
    Serial.println("  ✓ Servo OK!");

    // Conecta ao Wi-Fi
    Serial.println("[STEP 5] Conectando ao Wi-Fi...");
    Serial.print("  SSID: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

    int tentativas = 0;
    while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
        delay(500);
        Serial.print(".");
        tentativas++;
    }

    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("  ✓ Wi-Fi conectado!");
        Serial.print("  IP: ");
        Serial.println(WiFi.localIP());

        // Mostrar IP no display
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("Robo Eyes Online");
        display.setCursor(0, 15);
        display.print("IP: ");
        display.println(WiFi.localIP());
        display.setCursor(0, 30);
        display.println("Aguardando comandos...");
        display.display();
        delay(2000);
    } else {
        Serial.println("  ✗ ERRO: Falha ao conectar!");
    }

    // --- ROTAS DO SERVIDOR WEB ---
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        Serial.println("[WEB] GET / - Enviando página HTML");
        request->send_P(200, "text/html", index_html);
    });

    server.on("/angle", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("value")) {
            String value = request->getParam("value")->value();
            servoAngle = value.toInt();

            Serial.print("[SERVO] Ângulo recebido: ");
            Serial.print(servoAngle);
            Serial.println("°");

            myServo.write(servoAngle);

            String response = "Servo movido para " + String(servoAngle) + " graus";
            request->send(200, "text/plain", response);
        } else {
            request->send(400, "text/plain", "Parametro 'value' ausente");
        }
    });

    server.on("/face", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("expression")) {
            String expr = request->getParam("expression")->value();
            Serial.print("[FACE] Expressão recebida: ");
            Serial.println(expr);

            if (expr == "happy") {
                eyes.setMood(HAPPY);
                Serial.println("  -> HAPPY");
            }
            else if (expr == "angry") {
                eyes.setMood(ANGRY);
                Serial.println("  -> ANGRY");
            }
            else if (expr == "tired") {
                eyes.setMood(TIRED);
                Serial.println("  -> TIRED");
            }
            else if (expr == "default") {
                eyes.setMood(DEFAULT);
                Serial.println("  -> DEFAULT");
            }
            else if (expr == "open") {
                eyes.open();
                Serial.println("  -> OPEN");
            }
            else if (expr == "close") {
                eyes.close();
                Serial.println("  -> CLOSE");
            }

            String response = "Expressao: " + expr;
            request->send(200, "text/plain", response);
        } else {
            request->send(400, "text/plain", "Parametro 'expression' ausente");
        }
    });

    // ROTA: Receber e exibir texto customizado
    server.on("/text", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("message")) {
            customText = request->getParam("message")->value();
            textDisplayTime = millis();
            showingCustomText = true;

            Serial.print("[TEXT] Texto recebido: ");
            Serial.println(customText);

            String response = "Texto exibido por 5 segundos: " + customText;
            request->send(200, "text/plain", response);
        } else {
            request->send(400, "text/plain", "Parametro 'message' ausente");
        }
    });

    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
        JsonDocument doc;
        doc["angle"] = servoAngle;
        doc["ip"] = WiFi.localIP().toString();

        AsyncResponseStream *response = request->beginResponseStream("application/json");
        serializeJson(doc, *response);
        request->send(response);
    });

    // Inicia o servidor
    server.begin();
    Serial.println("[STEP 6] Servidor HTTP iniciado na porta 80");
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║           SETUP COMPLETO!              ║");
    Serial.println("╚════════════════════════════════════════╝\n");
}

// --- LOOP PRINCIPAL ---
unsigned long lastUpdate = 0;

void loop() {
    // Se está mostrando texto customizado
    if (showingCustomText) {
        // Verificar se já passou 5 segundos
        if ((millis() - textDisplayTime) < 5000) {
            // Desenhar olhos com texto embaixo
            drawTextUnderEyes(customText);
        } else {
            // Tempo expirou, voltar apenas aos olhos
            showingCustomText = false;
            Serial.println("[TEXT] Tempo de 5 segundos expirou, voltando aos olhos");
        }
    } else {
        // Apenas atualizar os olhos normalmente
        eyes.update();
    }

    // Imprimir debug a cada 10 segundos
    if (millis() - lastUpdate > 10000) {
        lastUpdate = millis();
        Serial.print("[LOOP] Funcionando - IP: ");
        Serial.println(WiFi.localIP());
    }
}