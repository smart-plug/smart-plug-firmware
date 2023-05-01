#include <Arduino.h>

// Constantes que definem pinos de entrada e saída
#define RELAY_PIN 12
#define BUTTON_PIN 14

#define BUTTON_PRESSED LOW
#define RELAY_ACTIVE_STATUS HIGH

// Variaveis globais para debounce do botão
int buttonState;
int lastButtonState = !BUTTON_PRESSED;

unsigned int debounceDelay = 50;
unsigned long lastDebounceTime = 0;

// Variavel global para estado do relé
bool relay_status = !RELAY_ACTIVE_STATUS;
bool new_relay_status = relay_status;

// Função para manipular a interrupção do botão
void buttonInterrupt()
{
  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    new_relay_status = !relay_status;
    lastDebounceTime = millis();
  }
}

void setup()
{
  Serial.begin(115200);

  // Configurando pinos de entrada e saída para botão e led respectivamente;
  // Obs.: Pino do botão configurado com resistor de PULL-UP
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, relay_status);

  // Configurando a interrupção no pino do botão
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonInterrupt, RISING);
}

void loop()
{
  if (new_relay_status != relay_status)
  {
    relay_status = new_relay_status;

    digitalWrite(RELAY_PIN, relay_status);

    Serial.print("Relay status changed to: ");
    Serial.println(relay_status);
  }
}