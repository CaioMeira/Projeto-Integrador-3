/**
 * MotionController.cpp
 * Implementação da lógica de interpolação e controle de servos.
 */
#include "MotionController.h"
#include <Arduino.h>

// --- Variáveis de Estado de Movimento (Internas) ---
// Note: '_isMoving' usa um underscore para evitar conflito de nome
// com a função 'isMoving()'.
static bool _isMoving = false;
static unsigned long moveStartTime;
static unsigned long moveDuration;
static int startAngles[NUM_SERVOS];
static int targetAngles[NUM_SERVOS];

// Objetos de Hardware (definidos aqui, pois este módulo os controla)
Servo servos[NUM_SERVOS];

// Variáveis de Posição (definidas aqui, pois este módulo as controla)
// VALORES INICIAIS AQUI (DEFINIÇÃO) CORRIGEM O ERRO DE LINKER ANTERIOR
int currentAngles[NUM_SERVOS] = {90, 90, 90, 90, 90, 90, 90};
int minAngles[NUM_SERVOS] = {0, 0, 0, 0, 0, 0, 0};
int maxAngles[NUM_SERVOS] = {180, 180, 180, 180, 180, 180, 180};
int offsets[NUM_SERVOS] = {0, 0, 0, 0, 0, 0, 0};

namespace MotionController
{

  /**
   * @brief Inicializa os pinos dos servos, define a posição neutra inicial e
   * configura os limites seguros.
   */
  void setup()
  { // <-- DEFINIÇÃO PARA O LINKER
    int safeMin[NUM_SERVOS] = {0, 95, 95, 50, 0, 60, 55};
    int safeMax[NUM_SERVOS] = {180, 180, 180, 180, 180, 180, 155};
    int safeNeutral[NUM_SERVOS] = {90, 130, 130, 100, 70, 120, 100};

    for (int i = 0; i < NUM_SERVOS; i++)
    {
      servos[i].attach(servoPins[i]);
      minAngles[i] = safeMin[i];
      maxAngles[i] = safeMax[i];
      offsets[i] = 0;
      currentAngles[i] = safeNeutral[i];
      servos[i].write(constrain(safeNeutral[i], 0, 180));
      delay(50); // Delay curto para o servo "assentar" na inicialização
    }
  }

  /**
   * @brief Retorna o estado do movimento.
   */
  bool isMoving()
  {
    return _isMoving;
  }

  /**
   * @brief Calcula a duração ideal do movimento baseado na velocidade padrão.
   */
  unsigned long calculateDurationBySpeed(const int target[NUM_SERVOS])
  {
    int maxDelta = 0;
    // Encontra o servo que tem o maior trajeto a percorrer
    for (int i = 0; i < NUM_SERVOS; i++)
    {
      int delta = abs(target[i] - currentAngles[i]);
      if (delta > maxDelta)
      {
        maxDelta = delta;
      }
    }
    // Calcula a duração e garante que seja pelo menos o mínimo
    unsigned long duration = (unsigned long)maxDelta * DEFAULT_SPEED_MS_PER_DEGREE;
    return max(duration, (unsigned long)MIN_MOVE_DURATION);
  }

  void startSmoothMove(const int newTargetAngles[NUM_SERVOS], unsigned long duration)
  {
    // Validação de servos: verifica se os ângulos estão dentro dos limites permitidos
    for (int i = 0; i < NUM_SERVOS; i++)
    {
      if (newTargetAngles[i] < minAngles[i] || newTargetAngles[i] > maxAngles[i])
      {
        Serial.print(F("ERRO: Servo "));
        Serial.print(i);
        Serial.print(F(" fora dos limites ("));
        Serial.print(minAngles[i]);
        Serial.print(F("-"));
        Serial.print(maxAngles[i]);
        Serial.print(F("). Valor recebido: "));
        Serial.println(newTargetAngles[i]);
        return; // Aborta o movimento se qualquer servo estiver fora dos limites
      }
    }

    // Se a duração for 0, executa o movimento instantâneo
    if (duration == 0)
    {
      for (int i = 0; i < NUM_SERVOS; i++)
      {
        currentAngles[i] = constrain(newTargetAngles[i], minAngles[i], maxAngles[i]);
        int correctedAngle = constrain(currentAngles[i] + offsets[i], 0, 180);
        servos[i].write(correctedAngle);
      }
      _isMoving = false;
      return;
    }

    // Prepara para o movimento suave
    moveStartTime = millis();
    moveDuration = duration;
    for (int i = 0; i < NUM_SERVOS; i++)
    {
      startAngles[i] = currentAngles[i]; // Inicia da posição interpolada ATUAL
      targetAngles[i] = constrain(newTargetAngles[i], minAngles[i], maxAngles[i]);
    }
    _isMoving = true;
  }

  /**
   * @brief Atualiza a posição dos servos com base no tempo decorrido.
   * Deve ser chamado na função loop() principal.
   */
  void update()
  { // <-- DEFINIÇÃO PARA O LINKER
    if (!_isMoving)
      return; // Economiza processamento se não estiver movendo

    unsigned long elapsedTime = millis() - moveStartTime;

    if (elapsedTime >= moveDuration)
    {
      // Movimento concluído, garante a posição final
      for (int i = 0; i < NUM_SERVOS; i++)
      {
        currentAngles[i] = targetAngles[i]; // Define a posição lógica final
        int correctedAngle = constrain(currentAngles[i] + offsets[i], 0, 180);
        servos[i].write(correctedAngle);
      }
      _isMoving = false;
      // Serial.println(F("Movimento concluído.")); // Opcional: Debug
    }
    else
    {
      // Interpolação (Easing Quadrático)
      float progress = (float)elapsedTime / (float)moveDuration;
      float easeProgress;

      // Fórmula EaseInOutQuad (suave no início e no fim)
      if (progress < 0.5f)
      {
        easeProgress = 2.0f * progress * progress;
      }
      else
      {
        easeProgress = 1.0f - pow(-2.0f * progress + 2.0f, 2.0f) / 2.0f;
      }

      // Aplica o ângulo interpolado a cada servo
      for (int i = 0; i < NUM_SERVOS; i++)
      {
        int interpolatedAngle = startAngles[i] + (targetAngles[i] - startAngles[i]) * easeProgress;
        currentAngles[i] = interpolatedAngle; // Atualiza a posição lógica atual
        int correctedAngle = constrain(interpolatedAngle + offsets[i], 0, 180);
        servos[i].write(correctedAngle);
      }
    }
  }

} // namespace MotionController
