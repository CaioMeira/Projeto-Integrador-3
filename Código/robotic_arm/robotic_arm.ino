#include "Config.h"
#include "MotionController.h"
#include "Storage.h"
#include "CommandParser.h"

void setup() {
  Serial.begin(115200);
  delay(100);
  EEPROM.begin(EEPROM_SIZE);

  // Posições de segurança inicial (hardcoded, usadas se a EEPROM falhar)
  int safeMin[NUM_SERVOS]     = {0, 95, 95, 50, 0, 60, 55};
  int safeMax[NUM_SERVOS]     = {180, 180, 180, 180, 180, 180, 155};
  int safeNeutral[NUM_SERVOS] = {90, 130, 130, 100, 70, 120, 100};

  // Inicialização do hardware e variáveis
  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].attach(servoPins[i]);
    // Inicializa limites e posição com valores seguros
    minAngles[i] = safeMin[i];
    maxAngles[i] = safeMax[i];
    offsets[i] = 0;
    currentAngles[i] = safeNeutral[i];
    // Move para a posição neutra inicial
    servos[i].write(constrain(safeNeutral[i] + offsets[i], 0, 180));
    delay(50);
  }

  // Tenta carregar dados persistentes
  if (!loadFromEEPROM()) {
    Serial.println("⚠️ EEPROM vazia ou inválida. Usando valores iniciais.");
  }
  
  printHelp();
}

void loop() {
  // 1. Atualização Contínua do Movimento (Concorrência)
  updateSmoothMovement(); 

  // 2. Processamento de Comandos Seriais
  handleSerialInput();
}