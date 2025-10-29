/**
 * PoseManager.h
 * Responsável pela lógica de salvar, carregar e gerenciar
 * as 'Poses' (pontos estáticos) na EEPROM.
 */
#ifndef POSE_MANAGER_H
#define POSE_MANAGER_H

#include "Config.h"

namespace PoseManager
{

    /**
     * @brief Lista todas as poses salvas na Serial.
     */
    void listPoses();

    /**
     * @brief Salva a posição atual dos servos como uma nova pose.
     * @param name Nome da pose.
     */
    void savePose(const char *name);

    /**
     * @brief Deleta uma pose por nome, ou todas.
     * @param name Nome da pose, ou "all" para apagar todas.
     */
    void deletePose(const char *name);

    /**
     * @brief Carrega uma pose pelo nome e inicia o movimento.
     * Calcula a duração do movimento com base na velocidade padrão.
     * @param name Nome da pose a ser carregada.
     * @return true se a pose foi encontrada e o movimento iniciado, false caso contrário.
     */
    bool loadPoseByName(const char *name); // <-- FUNÇÃO CENTRALIZADA

    /**
     * @brief Carrega uma pose pelo nome com duração customizada (Sobrecarga).
     * @param name Nome da pose.
     * @param duration Duração do movimento em ms.
     * @return true se a pose foi encontrada, false caso contrário.
     */
    bool loadPoseByName(const char *name, unsigned long duration); // <-- SOBRECARGA ADICIONADA

} // namespace PoseManager

#endif // POSE_MANAGER_H
