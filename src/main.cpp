#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <RoboEyes.h>

// --- Pinos ---
#define JOY_X_PIN A0
#define JOY_Y_PIN A1
#define JOY_SW_PIN 3 // Botão do Joystick

// --- Configuração do Display OLED ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_I2C_ADDRESS 0x3C

// --- OBJETOS ---
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
RoboEyes<Adafruit_SSD1306> eyes(oled);

// --- Variáveis de Controle ---
int currentMood = 0;
const int totalMoods = 3; // DEFAULT, HAPPY, ANGRY, TIRED
long lastSwitchTime = 0;
long debounceDelay = 500; // Atraso para evitar múltiplos cliques

void setup() {
  Serial.begin(9600);

  // Configura o pino do botão do joystick
  pinMode(JOY_SW_PIN, INPUT_PULLUP);

  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
    Serial.println(F("Falha ao iniciar SSD1306. Verifique a fiacao."));
    while (1);
  }
  Serial.println(F("Display OLED iniciado com sucesso."));
  oled.clearDisplay();
  oled.display();

  eyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 30);
  eyes.setAutoblinker(true, 2, 2);
}

void loop() {
  // Sempre atualiza as animações dos olhos
  eyes.update();
  eyes.setMood(HAPPY);

  // --- LÓGICA DO JOYSTICK ---

  // 1. Controle da posição do olhar
  int xVal = analogRead(JOY_X_PIN);
  int yVal = analogRead(JOY_Y_PIN);

  if (yVal < 200) { // Cima
    if (xVal < 200) eyes.setPosition(NW);
    else if (xVal > 800) eyes.setPosition(NE);
    else eyes.setPosition(N);
  } else if (yVal > 800) { // Baixo
    if (xVal < 200) eyes.setPosition(SW);
    else if (xVal > 800) eyes.setPosition(SE);
    else eyes.setPosition(S);
  } else { // Centro (vertical)
    if (xVal < 200) eyes.setPosition(W);
    else if (xVal > 800) eyes.setPosition(E);
    else eyes.setPosition(DEFAULT);
  }

  // 2. Controle do humor com o botão do joystick
  if (digitalRead(JOY_SW_PIN) == LOW && (millis() - lastSwitchTime) > debounceDelay) {
    currentMood = (currentMood + 1) % totalMoods;
    switch(currentMood) {
      case 0: eyes.setMood(DEFAULT); break;
      case 1: eyes.setMood(HAPPY); break;
      case 2: eyes.setMood(ANGRY); break;
      case 3: eyes.setMood(TIRED); break;
    }
    lastSwitchTime = millis(); // Reseta o timer do debounce
  }
}