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
        Serial.print(" [");
        Serial.print(i);
        Serial.print("] ");
        Serial.println(p.name);
        count++;
      }
    }
    if (count == 0)
    {
      Serial.println(F(" Nenhuma pose encontrada."));
    }
    Serial.print(F(" Total: "));
    Serial.print(count);
    Serial.print(F(" de "));
    Serial.print(MAX_POSES);
    Serial.println(F(" slots usados."));
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
      Serial.print(F("Pose '"));
      Serial.print(name);
      Serial.print(F("' salva no slot "));
      Serial.print(emptySlot);
      Serial.println(F("."));
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
        Serial.print(F("Pose '"));
        Serial.print(name);
        Serial.println(F("' apagada."));
        return;
      }
    }
    Serial.print(F("Pose '"));
    Serial.print(name);
    Serial.println(F("' nao encontrada."));
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
        Serial.print(F("Carregando pose '"));
        Serial.print(name);
        Serial.print(F("' (duracao: "));
        Serial.print(duration);
        Serial.println(F(" ms)..."));
        // Inicia o movimento via MotionController
        MotionController::startSmoothMove(p.angles, duration);
        return true;
      }
    }
    Serial.print(F("ERRO: Pose '"));
    Serial.print(name);
    Serial.println(F("' nao encontrada."));
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
        Serial.print(F("Carregando pose '"));
        Serial.print(name);
        Serial.print(F("' (duracao calc: "));
        Serial.print(duration);
        Serial.println(F(" ms)..."));
        MotionController::startSmoothMove(p.angles, duration);
        return true;
      }
    }
    Serial.print(F("ERRO: Pose '"));
    Serial.print(name);
    Serial.println(F("' nao encontrada."));
    return false;
  }

} // namespace PoseManager
