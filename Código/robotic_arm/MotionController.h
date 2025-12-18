/**
 * @file MotionController.h
 * @brief Define a interface do módulo responsável por gerenciar o movimento suave (smooth move)
 * e o estado (posição, velocidade) dos servos.
 */
#ifndef MOTION_CONTROLLER_H
#define MOTION_CONTROLLER_H

#include "Config.h"

namespace MotionController
{

    /**
     * @brief Inicializa os pinos dos servos, define a posição neutra inicial e
     * configura os limites seguros.
     */
    void setup(bool hasCalibration);

    /**
     * @brief Inicia um movimento suave (interpolado) para a posição alvo.
     * @param target Array com os ângulos alvo lógicos (0-180°).
     * @param duration Duração total do movimento em milissegundos.
     */
    void startSmoothMove(const int target[NUM_SERVOS], unsigned long duration);

    /**
     * @brief Atualiza a posição dos servos com base no tempo decorrido para um movimento suave.
     * Deve ser chamado repetidamente na função loop().
     */
    void update(); // <-- NOME PADRONIZADO: Agora corresponde à chamada em robotic_arm.ino

    /**
     * @brief Verifica se um movimento suave está em progresso.
     * @return true se estiver movendo, false caso contrário.
     */
    bool isMoving();

    /**
     * @brief Calcula a duração do movimento necessária para manter a velocidade padrão.
     * @param target Array com os ângulos alvo lógicos.
     * @return Duração mínima do movimento em milissegundos.
     */
    unsigned long calculateDurationBySpeed(const int target[NUM_SERVOS]);

} // namespace MotionController

#endif // MOTION_CONTROLLER_H
