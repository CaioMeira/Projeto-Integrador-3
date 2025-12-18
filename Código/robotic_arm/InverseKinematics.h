#ifndef INVERSE_KINEMATICS_H
#define INVERSE_KINEMATICS_H

#include "Config.h"

namespace InverseKinematics
{

    /**
     * @brief Resolve a cinemática inversa para um ponto cartesiano simples.
     * @param x Coordenada em mm no eixo X (frente do robô).
     * @param y Coordenada em mm no eixo Y (direita positiva).
     * @param z Coordenada em mm no eixo Z (cima positiva).
     * @param targetAngles Array preenchido com os ângulos desejados (0-180°) na ordem dos servos.
     * @return true se o ponto estiver dentro do alcance aproximado e os ângulos forem calculados.
     */
    bool solveXYZ(float x, float y, float z, int targetAngles[NUM_SERVOS]);

    /**
     * @brief Calcula a cinemática direta aproximada de um conjunto de ângulos.
     * @param angles Ângulos lógicos (0-180°) na mesma ordem dos servos.
     * @param x Saída X em mm (frente do robô).
     * @param y Saída Y em mm (direita positiva).
     * @param z Saída Z em mm (cima positiva).
     * @return true se o cálculo foi realizado com sucesso.
     */
    bool estimateXYZ(const int angles[NUM_SERVOS], float &x, float &y, float &z);

    /**
     * @brief Conveniência para obter o XYZ da pose atual (currentAngles).
     */
    inline bool estimateCurrentXYZ(float &x, float &y, float &z)
    {
        return estimateXYZ(currentAngles, x, y, z);
    }

} // namespace InverseKinematics

#endif // INVERSE_KINEMATICS_H
