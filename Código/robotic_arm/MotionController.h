/**
 * @file MotionController.h
 * @brief Define a interface do módulo responsável por gerenciar o movimento suave (easing)
 * e concorrente dos servos.
 */

#ifndef MOTION_CONTROLLER_H
#define MOTION_CONTROLLER_H

#include "Config.h"

// --- Variáveis de Controle de Movimento (Extern) ---
extern bool isMoving;
extern int startAngles[NUM_SERVOS];
extern int targetAngles[NUM_SERVOS];

// --- Protótipos de Funções ---

/**
 * @brief Calcula a duração necessária para um movimento com base na velocidade padrão.
 * @param newTargetAngles Array de 7 elementos com os novos ângulos de destino.
 * @return unsigned long Duração calculada do movimento em milissegundos (ms).
 */
unsigned long calculateDurationBySpeed(const int newTargetAngles[NUM_SERVOS]);

/**
 * @brief Inicia ou atualiza um movimento suave para um novo destino.
 * @param newTargetAngles Array de 7 elementos com os novos ângulos de destino.
 * @param duration Duração do novo movimento em milissegundos.
 */
void startSmoothMove(const int newTargetAngles[NUM_SERVOS], unsigned long duration);

/**
 * @brief Executa a interpolação suave (easing) de todos os servos. Deve ser chamada no loop().
 */
void updateSmoothMovement();

#endif // MOTION_CONTROLLER_H
