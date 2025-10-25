/**
 * @file Calibration.cpp
 * @brief Implementação das funções de controle de calibração (limites e offsets).
 */

#include "Calibration.h"

void setMinAngle(int servo_idx, int angle) {
    if (servo_idx >= 0 && servo_idx < NUM_SERVOS) {
        minAngles[servo_idx] = constrain(angle, 0, 180);
        Serial.printf("Mínimo do servo %d definido para %d°\n", servo_idx, angle);
    } else {
        Serial.println("⚠️ Índice de servo inválido.");
    }
}

void setMaxAngle(int servo_idx, int angle) {
    if (servo_idx >= 0 && servo_idx < NUM_SERVOS) {
        maxAngles[servo_idx] = constrain(angle, 0, 180);
        Serial.printf("Máximo do servo %d definido para %d°\n", servo_idx, angle);
    } else {
        Serial.println("⚠️ Índice de servo inválido.");
    }
}

void setOffset(int servo_idx, int offset) {
    if (servo_idx >= 0 && servo_idx < NUM_SERVOS) {
        offsets[servo_idx] = constrain(offset, -90, 90);
        // Aplica o offset imediatamente para teste/verificação
        servos[servo_idx].write(constrain(currentAngles[servo_idx] + offsets[servo_idx], 0, 180)); 
        Serial.printf("Offset do servo %d ajustado para %+d°\n", servo_idx, offset);
    } else {
        Serial.println("⚠️ Índice de servo inválido.");
    }
}

void showCurrentCalibration() {
    for (int i = 0; i < NUM_SERVOS; i++) {
      Serial.printf("Servo %d | atual:%3d | min:%3d | max:%3d | offset:%+3d | out:%3d\n",
        i, currentAngles[i], minAngles[i], maxAngles[i], offsets[i],
        constrain(currentAngles[i] + offsets[i], 0, 180));
    }
}
