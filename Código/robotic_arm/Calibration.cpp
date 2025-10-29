/**
 * @file Calibration.cpp
 * @brief Implementação do módulo de calibração.
 * Contém a lógica para fazer o parsing dos comandos de calibração.
 */

#include "Calibration.h"
#include "MotionController.h" 
#include <Arduino.h>

namespace Calibration {

void setMin(String input) {
    int idx, val;
    if (sscanf(input.c_str(), "min %d %d", &idx, &val) == 2 && idx >= 0 && idx < NUM_SERVOS) {
        // Acesso direto às variáveis globais
        minAngles[idx] = constrain(val, 0, 180); 
        Serial.printf(F("Mínimo do servo %d definido para %d°\n"), idx, val);
    } else {
        Serial.println(F("Formato inválido. Use: min <servo> <angulo>"));
    }
}

void setMax(String input) {
    int idx, val;
    if (sscanf(input.c_str(), "max %d %d", &idx, &val) == 2 && idx >= 0 && idx < NUM_SERVOS) {
        // Acesso direto às variáveis globais
        maxAngles[idx] = constrain(val, 0, 180);
        Serial.printf(F("Máximo do servo %d definido para %d°\n"), idx, val);
    } else {
        Serial.println(F("Formato inválido. Use: max <servo> <angulo>"));
    }
}

void setOffset(String input) {
    int idx, val;
    if (sscanf(input.c_str(), "offset %d %d", &idx, &val) == 2 && idx >= 0 && idx < NUM_SERVOS) {
        // Acesso direto às variáveis globais
        offsets[idx] = constrain(val, -90, 90);
        
        // Aplica o offset imediatamente para visualização
        int correctedAngle = constrain(currentAngles[idx] + offsets[idx], 0, 180);
        servos[idx].write(correctedAngle);
        Serial.printf(F("Offset do servo %d ajustado para %+d°\n"), idx, val);
    } else {
        Serial.println(F("Formato inválido. Use: offset <servo> <valor>"));
    }
}

void alignShoulders(String input) {
    unsigned long duration = 0; // 0 = gatilho para auto-cálculo
    sscanf(input.c_str(), "align ombro %lu", &duration);

    // Calcula a posição média entre os dois ombros
    int media = (currentAngles[1] + currentAngles[2]) / 2;

    int tempTarget[NUM_SERVOS];
    for(int i=0; i<NUM_SERVOS; i++) tempTarget[i] = currentAngles[i];
    tempTarget[1] = media;
    tempTarget[2] = media;

    if (duration == 0) {
        duration = MotionController::calculateDurationBySpeed(tempTarget);
        Serial.printf(F("Alinhando ombros para %d° (duração calc: %lu ms)...\n"), media, duration);
    } else {
        Serial.printf(F("Alinhando ombros para %d° (duração: %lu ms)...\n"), media, duration);
    }
    
    MotionController::startSmoothMove(tempTarget, duration);
}

void printStatus() {
    Serial.println(F("\n--- Status Atual dos Servos ---"));
    for (int i = 0; i < NUM_SERVOS; i++) {
        Serial.printf("Servo %d | Lógico:%3d° | Min:%3d° | Max:%3d° | Offset:%+3d° | Físico(out):%3d°\n",
            i, 
            currentAngles[i], 
            minAngles[i], 
            maxAngles[i], 
            offsets[i],
            constrain(currentAngles[i] + offsets[i], 0, 180));
    }
}

} // namespace Calibration
