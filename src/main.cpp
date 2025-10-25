#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RoboEyes.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

// --- CONFIGURAÇÕES ---
// Wi-Fi
const char* ssid = "Quiet House";
const char* password = "quiethouse2025@";

// Display OLED (128x64)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RoboEyes eyes(display);

// Servo Motor SG90
const int servo_pin = 12;
Servo myServo;
int servoAngle = 90; // Ângulo inicial (posição central)

// Servidor Web na porta 80
AsyncWebServer server(80);

// --- PÁGINA WEB (HTML + CSS + JavaScript) ---
// O slider agora vai de 0 a 180 para representar o ângulo do servo
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
        #angleValue { font-size: 18px; color: #61dafb; }
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
            <button onclick="changeFace('happy')">Feliz</button>
            <button onclick="changeFace('angry')">Zangado</button>
            <button onclick="changeFace('sad')">Triste</button>
            <button onclick="changeFace('sleepy')">Dormindo</button>
            <button onclick="changeFace('neutral')">Neutro</button>
            <button onclick="changeFace('blink')">Piscar</button>
        </div>
    </div>

    <script>
        function updateAngle(value) {
            document.getElementById('angleValue').innerText = value;
            fetch('/angle?value=' + value);
        }

        function changeFace(face) {
            fetch('/face?expression=' + face);
        }

        document.addEventListener('DOMContentLoaded', function() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('angle').value = data.angle;
                    document.getElementById('angleValue').innerText = data.angle;
                });
        });
    </script>
</body>
</html>
)rawliteral";


// --- SETUP ---
void setup() {
    Serial.begin(115200);

    // Inicializa o display
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("Falha ao iniciar SSD1306"));
        for(;;);
    }
    display.clearDisplay();
    display.display();

    // Inicializa o RoboEyes
    eyes.begin();
    eyes.setEyeShape(RoboEyes::EYE_SHAPE_ROUND);
    eyes.setPupil(RoboEyes::PUPIL_SHAPE_CIRCLE);
    eyes.setExpression(RoboEyes::EXPRESSION_NEUTRAL);

    // Configura o servo
    // Permite alocar timers do ESP32 dinamicamente
    ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
    myServo.setPeriodHertz(50); // Frequência padrão para servos (50Hz)
    myServo.attach(servo_pin, 500, 2400); // Anexa o servo ao pino, com min/max de pulso
    myServo.write(servoAngle); // Move para a posição inicial

    // Conecta ao Wi-Fi
    Serial.print("Conectando a ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi conectado!");
    Serial.print("Endereço IP: ");
    Serial.println(WiFi.localIP());

    // --- ROTAS DO SERVIDOR WEB ---
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html);
    });

    // Rota para mudar o ângulo do servo
    server.on("/angle", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("value")) {
            String value = request->getParam("value")->value();
            servoAngle = value.toInt();
            myServo.write(servoAngle); // Comanda o servo para o novo ângulo
            request->send(200, "text/plain", "Angulo atualizado para " + value);
        } else {
            request->send(400, "text/plain", "Parametro 'value' ausente");
        }
    });

    // Rota para mudar a expressão (sem alteração)
    server.on("/face", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("expression")) {
            String expr = request->getParam("expression")->value();
            if (expr == "happy") eyes.setExpression(RoboEyes::EXPRESSION_HAPPY);
            else if (expr == "angry") eyes.setExpression(RoboEyes::EXPRESSION_ANGRY);
            else if (expr == "sad") eyes.setExpression(RoboEyes::EXPRESSION_SAD);
            else if (expr == "sleepy") eyes.setExpression(RoboEyes::EXPRESSION_SLEEPY);
            else if (expr == "neutral") eyes.setExpression(RoboEyes::EXPRESSION_NEUTRAL);
            else if (expr == "blink") eyes.blink();

            request->send(200, "text/plain", "Expressao mudada para " + expr);
        } else {
            request->send(400, "text/plain", "Parametro 'expression' ausente");
        }
    });

    // Rota para obter o status atual (ângulo)
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        JsonDocument doc;
        doc["angle"] = servoAngle;
        serializeJson(doc, *response);
        request->send(response);
    });

    // Inicia o servidor
    server.begin();
    Serial.println("Servidor HTTP iniciado");
}

// --- LOOP PRINCIPAL ---
void loop() {
    // A biblioteca RoboEyes precisa ser chamada repetidamente para animar
    eyes.update();
}