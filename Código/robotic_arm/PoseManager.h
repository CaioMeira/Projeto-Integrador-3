/**
 * @file PoseManager.h
 * @brief Define a interface do módulo responsável por salvar, carregar, listar e deletar poses na EEPROM.
 */

#ifndef POSE_MANAGER_H
#define POSE_MANAGER_H

#include "Config.h"

// --- Protótipos de Funções ---

/**
 * @brief Salva a posição atual (`currentAngles`) em um slot de pose.
 * @param name Nome da pose a ser salva.
 */
void savePose(String name);

/**
 * @brief Carrega uma pose salva e inicia um movimento suave para essa pose.
 * @param name Nome da pose a ser carregada.
 * @param duration Duração opcional do movimento (0 para calcular automaticamente).
 * @return true Se a pose foi encontrada e o movimento iniciado.
 * @return false Se a pose não foi encontrada.
 */
bool loadPose(String name, unsigned long duration);

/**
 * @brief Deleta uma pose da EEPROM.
 * @param name Nome da pose a ser deletada.
 */
void deletePose(String name);

/**
 * @brief Lista todas as poses salvas na EEPROM.
 */
void listPoses();

#endif // POSE_MANAGER_H
