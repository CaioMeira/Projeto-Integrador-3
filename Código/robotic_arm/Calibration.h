/**
 * @file Calibration.h
 * @brief Define a interface do módulo responsável por gerenciar limites (min/max) e offsets
 * de calibração para os servos.
 */

#ifndef CALIBRATION_H
#define CALIBRATION_H

#include "Config.h"

namespace Calibration
{

    /**
     * @brief Define o ângulo mínimo de software para um servo.
     * @param input A string de comando completa (ex: "min 0 90").
     */
    void setMin(String input);

    /**
     * @brief Define o ângulo máximo de software para um servo.
     * @param input A string de comando completa (ex: "max 0 180").
     */
    void setMax(String input);

    /**
     * @brief Define o offset de calibração para um servo.
     * @param input A string de comando completa (ex: "offset 1 -5").
     */
    void setOffset(String input);

    /**
     * @brief Alinha os servos do ombro (1 e 2) pela média de suas posições.
     * @param input A string de comando completa (ex: "align ombro 1000").
     */
    void alignShoulders(String input);

    /**
     * @brief Exibe os valores atuais, limites e offsets de todos os servos na Serial.
     */
    void printStatus(); // Nome corrigido para 'printStatus'

} // namespace Calibration

#endif // CALIBRATION_H
