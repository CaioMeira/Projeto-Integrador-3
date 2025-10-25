/**
 * @file Calibration.h
 * @brief Define a interface do módulo responsável por gerenciar limites (min/max) e offsets
 * de calibração para os servos.
 */

#ifndef CALIBRATION_H
#define CALIBRATION_H

#include "Config.h"

// --- Protótipos de Funções ---

/**
 * @brief Define o ângulo mínimo de software para um servo.
 * @param servo_idx Índice do servo (0-6).
 * @param angle Ângulo mínimo (0-180).
 */
void setMinAngle(int servo_idx, int angle);

/**
 * @brief Define o ângulo máximo de software para um servo.
 * @param servo_idx Índice do servo (0-6).
 * @param angle Ângulo máximo (0-180).
 */
void setMaxAngle(int servo_idx, int angle);

/**
 * @brief Define o offset de calibração para um servo.
 * * Aplica o offset imediatamente.
 * @param servo_idx Índice do servo (0-6).
 * @param offset Valor do offset (-90 a 90).
 */
void setOffset(int servo_idx, int offset);

/**
 * @brief Exibe os valores atuais, limites e offsets de todos os servos na Serial.
 */
void showCurrentCalibration();

#endif // CALIBRATION_H
