/**
 * @file Storage.cpp
 * @brief Implementação das funções de persistência de dados (EEPROM) para estado e calibração.
 */

#include "Storage.h"
#include "MotionController.h" // Necessário para acessar startSmoothMove

/**
 * @brief Salva o estado atual do braço (ângulos, limites, offsets) na EEPROM.
 */
void saveToEEPROM() {
  StoredData sd;
  sd.magic = EEPROM_MAGIC;
  for (int i = 0; i < NUM_SERVOS; i++) {
    sd.current[i] = currentAngles[i];
    sd.minv[i] = minAngles[i];
    sd.maxv[i] = maxAngles[i];
    sd.offs[i] = offsets[i];
  }
  EEPROM.put(0, sd);
  EEPROM.commit();
  Serial.println("💾 Posições, limites e offsets salvos na EEPROM.");
}

/**
 * @brief Carrega o estado de calibração e a última posição salva da EEPROM.
 * * Se for bem-sucedido, inicia um movimento suave para a posição restaurada.
 * * @return true Se os dados foram carregados e validados.
 * @return false Se a EEPROM estiver vazia ou inválida.
 */
bool loadFromEEPROM() {
  StoredData sd;
  EEPROM.get(0, sd);
  if (sd.magic != EEPROM_MAGIC) return false;

  int initialAngles[NUM_SERVOS];
  for (int i = 0; i < NUM_SERVOS; i++) {
    minAngles[i] = constrain(sd.minv[i], 0, 180);
    maxAngles[i] = constrain(sd.maxv[i], 0, 180);
    offsets[i] = constrain(sd.offs[i], -180, 180);
    initialAngles[i] = sd.current[i];
  }
  
  // Calcula a duração automaticamente com base no destino restaurado
  unsigned long autoDuration = calculateDurationBySpeed(initialAngles);
  Serial.printf("✅ Restaurando posição da EEPROM em %lu ms...\n", autoDuration);
  startSmoothMove(initialAngles, autoDuration); 
  
  Serial.println("✅ Posições, limites e offsets restaurados da EEPROM.");
  return true;
}
