/**
 * @file MotionController.cpp
 * @brief Implementação das funções de movimento suave, interpolação (easing) e concorrência.
 * * Contém a definição das variáveis globais principais do sistema.
 */

#include "MotionController.h"

// --- Definições de Variáveis Globais Principais ---

Servo servos[NUM_SERVOS];
int currentAngles[NUM_SERVOS];
int minAngles[NUM_SERVOS];
int maxAngles[NUM_SERVOS];
int offsets[NUM_SERVOS];

// Variáveis de Controle de Movimento
bool isMoving = false;
unsigned long moveStartTime;
unsigned long moveDuration;
int startAngles[NUM_SERVOS];
int targetAngles[NUM_SERVOS];


// =================================================================
// FUNÇÕES DE MOVIMENTO CONCORRENTE
// =================================================================

unsigned long calculateDurationBySpeed(const int newTargetAngles[NUM_SERVOS]) {
  int maxDelta = 0;
  for (int i = 0; i < NUM_SERVOS; i++) {
    int delta = abs(constrain(newTargetAngles[i], minAngles[i], maxAngles[i]) - currentAngles[i]);
    if (delta > maxDelta) {
      maxDelta = delta;
    }
  }
  unsigned long duration = (unsigned long)maxDelta * DEFAULT_SPEED_MS_PER_DEGREE;
  if (duration < MIN_MOVE_DURATION) {
    duration = MIN_MOVE_DURATION;
  }
  return duration;
}

void startSmoothMove(const int newTargetAngles[NUM_SERVOS], unsigned long duration) {
  if (duration == 0) { 
    // Movimento instantâneo (sem suavização)
    for (int i = 0; i < NUM_SERVOS; i++) {
      currentAngles[i] = constrain(newTargetAngles[i], minAngles[i], maxAngles[i]);
      int correctedAngle = constrain(currentAngles[i] + offsets[i], 0, 180);
      servos[i].write(correctedAngle);
    }
    isMoving = false;
    Serial.println("✅ Movimento instantâneo concluído.");
    return;
  }
  
  // O ponto de partida é a posição atual para garantir a concorrência
  moveStartTime = millis();
  moveDuration = duration;
  for (int i = 0; i < NUM_SERVOS; i++) {
    startAngles[i] = currentAngles[i];
    targetAngles[i] = constrain(newTargetAngles[i], minAngles[i], maxAngles[i]);
  }
  isMoving = true;
}

void updateSmoothMovement() {
  if (!isMoving) return;
  unsigned long elapsedTime = millis() - moveStartTime;

  if (elapsedTime >= moveDuration) {
    // 1. Movimento concluído: Garante a posição final
    for (int i = 0; i < NUM_SERVOS; i++) {
      currentAngles[i] = targetAngles[i];
      int correctedAngle = constrain(currentAngles[i] + offsets[i], 0, 180);
      servos[i].write(correctedAngle);
    }
    isMoving = false;
    Serial.println("✅ Movimento concluído.");
  } else {
    // 2. Interpolação em progresso: Ease In/Out (Quadrático)
    float progress = (float)elapsedTime / (float)moveDuration;
    float easeProgress;

    if (progress < 0.5f) {
      easeProgress = 2.0f * progress * progress; 
    } else {
      float t = -2.0f * progress + 2.0f;
      easeProgress = 1.0f - (t * t) / 2.0f; 
    }
    
    // Aplica o ângulo interpolado
    for (int i = 0; i < NUM_SERVOS; i++) {
      int interpolatedAngle = startAngles[i] + (targetAngles[i] - startAngles[i]) * easeProgress;
      currentAngles[i] = interpolatedAngle; 
      int correctedAngle = constrain(interpolatedAngle + offsets[i], 0, 180);
      servos[i].write(correctedAngle);
    }
  }
}
