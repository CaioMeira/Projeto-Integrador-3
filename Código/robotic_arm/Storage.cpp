/**
 * @file Storage.cpp
 * @brief Implementação da lógica de persistência (EEPROM).
 */
#include "Storage.h"
#include "MotionController.h" // Para iniciar o movimento ao carregar

// As variáveis globais (currentAngles, minAngles, etc.) são definidas em MotionController.cpp
// e declaradas 'extern' em Config.h, portanto, estão disponíveis aqui.

namespace Storage {

void saveToEEPROM() {
    StoredData sd;
    sd.magic = EEPROM_MAGIC;
    for (int i = 0; i < NUM_SERVOS; i++) {
        // Acesso direto às variáveis globais
        sd.current[i] = currentAngles[i]; 
        sd.minv[i] = minAngles[i];
        sd.maxv[i] = maxAngles[i];
        sd.offs[i] = offsets[i];
    }
    EEPROM.put(0, sd);
    EEPROM.commit();
    Serial.println(F("💾 Posições, limites e offsets salvos na EEPROM."));
}

bool loadFromEEPROM(bool move) {
    StoredData sd;
    EEPROM.get(0, sd);
    if (sd.magic != EEPROM_MAGIC) {
        return false; // EEPROM vazia ou inválida
    }

    int initialAngles[NUM_SERVOS];
    for (int i = 0; i < NUM_SERVOS; i++) {
        // Acesso direto às variáveis globais
        minAngles[i] = constrain(sd.minv[i], 0, 180);
        maxAngles[i] = constrain(sd.maxv[i], 0, 180);
        offsets[i] = constrain(sd.offs[i], -180, 180);
        initialAngles[i] = sd.current[i]; // Posição lógica salva
    }

    if (move) {
        Serial.println(F("📦 Posições restauradas. Movendo para o último estado..."));
        // Usa a função de cálculo do MotionController
        unsigned long autoDuration = MotionController::calculateDurationBySpeed(initialAngles);
        MotionController::startSmoothMove(initialAngles, autoDuration);
    } else {
        // Apenas define os valores, sem mover
        for (int i = 0; i < NUM_SERVOS; i++) {
            currentAngles[i] = initialAngles[i];
            int correctedAngle = constrain(currentAngles[i] + offsets[i], 0, 180);
            servos[i].write(correctedAngle);
        }
        Serial.println(F("📦 Posições, limites e offsets restaurados (sem movimento)."));
    }
    return true;
}

} // namespace Storage
