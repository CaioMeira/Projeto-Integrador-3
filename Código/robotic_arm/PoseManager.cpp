/**
 * @file PoseManager.cpp
 * @brief Implementação das funções de gerenciamento de poses (save, load, delete, list).
 */

#include "PoseManager.h"
#include "MotionController.h" // Necessário para acessar startSmoothMove e calculateDurationBySpeed

/**
 * @brief Salva a posição atual (`currentAngles`) em um slot de pose.
 * * @param name Nome da pose a ser salva.
 */
void savePose(String name) {
  if (name.length() > POSE_NAME_LEN - 1) name = name.substring(0, POSE_NAME_LEN - 1);
  char cname[POSE_NAME_LEN];
  name.toCharArray(cname, POSE_NAME_LEN);

  for (int i = 0; i < MAX_POSES; i++) {
    Pose p;
    EEPROM.get(POSES_START + i * sizeof(Pose), p);
    // Verifica se o slot está vazio ou se o nome já existe (sobrescreve)
    if (p.name[0] == 0 || strncmp(p.name, cname, POSE_NAME_LEN) == 0) {
      strncpy(p.name, cname, POSE_NAME_LEN);
      for (int j = 0; j < NUM_SERVOS; j++) p.angles[j] = currentAngles[j];
      EEPROM.put(POSES_START + i * sizeof(Pose), p);
      EEPROM.commit();
      Serial.printf("Pose '%s' salva na posição %d\n", cname, i);
      return;
    }
  }
  Serial.println("⚠️ Limite de poses atingido.");
}

/**
 * @brief Carrega uma pose salva e inicia um movimento suave para essa pose.
 * * @param name Nome da pose a ser carregada.
 * @param duration Duração opcional do movimento (0 para calcular automaticamente).
 * @return true Se a pose foi encontrada e o movimento iniciado.
 * @return false Se a pose não foi encontrada.
 */
bool loadPose(String name, unsigned long duration) {
  if (name.length() > POSE_NAME_LEN - 1) name = name.substring(0, POSE_NAME_LEN - 1);
  char cname[POSE_NAME_LEN];
  name.toCharArray(cname, POSE_NAME_LEN);

  for (int i = 0; i < MAX_POSES; i++) {
    Pose p;
    EEPROM.get(POSES_START + i * sizeof(Pose), p);
    if (p.name[0] != 0 && strncmp(p.name, cname, POSE_NAME_LEN) == 0) {
      
      if (duration == 0) {
        duration = calculateDurationBySpeed(p.angles);
      }
      
      Serial.printf("Carregando pose '%s' em %lu ms (Movimento Concorrente)....\n", cname, duration);
      startSmoothMove(p.angles, duration); 
      return true;
    }
  }
  Serial.printf("Pose '%s' não encontrada\n", cname);
  return false;
}

/**
 * @brief Deleta uma pose da EEPROM.
 * * @param name Nome da pose a ser deletada.
 */
void deletePose(String name) {
    if (name.length() > POSE_NAME_LEN - 1) name = name.substring(0, POSE_NAME_LEN - 1);
    char cname[POSE_NAME_LEN];
    name.toCharArray(cname, POSE_NAME_LEN);

    for (int i = 0; i < MAX_POSES; i++) {
        Pose p;
        EEPROM.get(POSES_START + i * sizeof(Pose), p);
        if (strncmp(p.name, cname, POSE_NAME_LEN) == 0) {
            memset(&p, 0, sizeof(Pose)); // Limpa o slot
            EEPROM.put(POSES_START + i * sizeof(Pose), p);
            EEPROM.commit();
            Serial.printf("Pose '%s' deletada.\n", cname);
            return;
        }
    }
    Serial.printf("Pose '%s' não encontrada para deletar.\n", cname);
}

/**
 * @brief Lista todas as poses salvas na EEPROM.
 */
void listPoses() {
  Serial.println("\nPoses salvas na EEPROM:");
  int count = 0;
  for (int i = 0; i < MAX_POSES; i++) {
    Pose p;
    EEPROM.get(POSES_START + i * sizeof(Pose), p);
    if (p.name[0] != 0) {
      Serial.printf("  - %s\n", p.name);
      count++;
    }
  }
  if (count == 0) Serial.println("  Nenhuma pose encontrada.");
  Serial.printf("Total: %d de %d slots usados.\n", count, MAX_POSES);
}
