/**
 * PoseManager.cpp
 * Implementação da lógica de persistência das Poses.
 */
#include "PoseManager.h"
#include "MotionController.h" 

namespace PoseManager
{
  void readPose(int index, Pose &pose)
  {
    EEPROM.get(POSES_START + index * sizeof(Pose), pose);
  }

  void writePose(int index, const Pose &pose)
  {
    EEPROM.put(POSES_START + index * sizeof(Pose), pose);
  }

  void listPoses()
  {
    Serial.println(F("\n--- Poses Salvas ---"));
    int count = 0;
    for (int i = 0; i < MAX_POSES; i++)
    {
      Pose p;
      readPose(i, p);
      // Verifica se o primeiro caractere do nome não é nulo
      if (p.name[0] != 0)
      {
        Serial.printf(" [%d] %s\n", i, p.name);
        count++;
      }
    }
    if (count == 0)
    {
      Serial.println(F(" Nenhuma pose encontrada."));
    }
    Serial.printf(F(" Total: %d de %d slots usados.\n"), count, MAX_POSES);
  }

  void savePose(const char *name)
  {
    if (name[0] == 0)
      return; // Não salva com nome vazio

    int emptySlot = -1;
    // Procura um slot vazio ou um com o mesmo nome (para sobrescrever)
    for (int i = 0; i < MAX_POSES; i++)
    {
      Pose p;
      readPose(i, p);
      if (p.name[0] == 0)
      {
        if (emptySlot == -1)
          emptySlot = i; // Guarda o primeiro slot vazio
      }
      else if (strncmp(p.name, name, POSE_NAME_LEN) == 0)
      {
        emptySlot = i; // Encontrou slot com mesmo nome
        break;
      }
    }

    if (emptySlot != -1)
    {
      Pose newPose;
      strncpy(newPose.name, name, POSE_NAME_LEN - 1);
      newPose.name[POSE_NAME_LEN - 1] = '\0'; // Garante terminação nula

      for (int j = 0; j < NUM_SERVOS; j++)
      {
        newPose.angles[j] = currentAngles[j]; // Salva a posição LÓGICA atual
      }
      writePose(emptySlot, newPose);
      EEPROM.commit();
      Serial.printf(F("Pose '%s' salva no slot %d.\n"), name, emptySlot);
    }
    else
    {
      Serial.println(F("ERRO: Não há slots de pose disponíveis."));
    }
  }

  void deletePose(const char *name)
  {
    if (name[0] == 0)
      return;

    // Comando especial para apagar todas
    if (strncmp(name, "all", 3) == 0)
    {
      for (int i = 0; i < MAX_POSES; i++)
      {
        Pose emptyPose = {0}; // Cria uma estrutura zerada
        writePose(i, emptyPose);
      }
      EEPROM.commit();
      Serial.println(F("Todas as poses foram apagadas."));
      return;
    }

    // Procura a pose pelo nome para apagar
    for (int i = 0; i < MAX_POSES; i++)
    {
      Pose p;
      readPose(i, p);
      if (p.name[0] != 0 && strncmp(p.name, name, POSE_NAME_LEN) == 0)
      {
        Pose emptyPose = {0};
        writePose(i, emptyPose);
        EEPROM.commit();
        Serial.printf(F("Pose '%s' apagada.\n"), name);
        return;
      }
    }
    Serial.printf(F("Pose '%s' não encontrada.\n"), name);
  }

  /**
   * @brief Implementação da função centralizada (com duração customizada).
   */
  bool loadPoseByName(const char *name, unsigned long duration)
  {
    if (name[0] == 0)
      return false;

    Pose p;
    for (int i = 0; i < MAX_POSES; i++)
    {
      readPose(i, p);
      if (p.name[0] != 0 && strncmp(p.name, name, POSE_NAME_LEN) == 0)
      {
        Serial.printf(F("Carregando pose '%s' (duração: %lu ms)...\n"), name, duration);
        // Inicia o movimento via MotionController
        MotionController::startSmoothMove(p.angles, duration);
        return true;
      }
    }
    Serial.printf(F("ERRO: Pose '%s' não encontrada.\n"), name);
    return false;
  }

  /**
   * @brief Implementação da sobrecarga (com velocidade padrão).
   */
  bool loadPoseByName(const char *name)
  {
    if (name[0] == 0)
      return false;

    Pose p;
    for (int i = 0; i < MAX_POSES; i++)
    {
      readPose(i, p);
      if (p.name[0] != 0 && strncmp(p.name, name, POSE_NAME_LEN) == 0)
      {
        // Calcula a duração automaticamente
        unsigned long duration = MotionController::calculateDurationBySpeed(p.angles); // <-- CORRIGIDO
        Serial.printf(F("Carregando pose '%s' (duração calc: %lu ms)...\n"), name, duration);
        MotionController::startSmoothMove(p.angles, duration);
        return true;
      }
    }
    Serial.printf(F("ERRO: Pose '%s' não encontrada.\n"), name);
    return false;
  }

} // namespace PoseManager
