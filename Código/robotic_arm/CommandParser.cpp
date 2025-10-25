/**
 * @file CommandParser.cpp
 * @brief Implementação do parser de comandos seriais, roteando as ações para os módulos apropriados.
 */

#include "CommandParser.h"
#include "MotionController.h"
#include "PoseManager.h"
#include "Calibration.h"
#include "Storage.h"

void printHelp() {
  Serial.println("\n--- Comandos Disponíveis (v5.0 - Modular) ---");
  Serial.println("  Todos os comandos de movimento são executados imediatamente e podem ser combinados.");
  Serial.println("  [tempo_ms] é opcional e anula a velocidade padrão.");
  Serial.println("  ---------------------------------------------------------");
  Serial.println("  set <servo> <angulo> [tempo_ms] -> Move servo individual (Imediato)");
  Serial.println("  set ombro <angulo> [tempo_ms]   -> Move os dois servos do ombro (Imediato)");
  Serial.println("  loadpose <nome> [tempo_ms]      -> Carrega pose (Imediato)");
  Serial.println("  align ombro [tempo_ms]          -> Alinha ombros na média (Imediato)");
  Serial.println("  ---------------------------------------------------------");
  Serial.println("  min <servo> <angulo>            -> Define ângulo mínimo (Calibração)");
  Serial.println("  max <servo> <angulo>            -> Define ângulo máximo (Calibração)");
  Serial.println("  offset <servo> <valor>          -> Define offset de calibração (-90 a 90)");
  Serial.println("  mostrar                         -> Exibe valores atuais, limites e offsets");
  Serial.println("  listposes                       -> Lista todas as poses salvas");
  Serial.println("  save                            -> Salva estado atual na EEPROM (Persistência)");
  Serial.println("  load                            -> Carrega estado da EEPROM (Persistência)");
  Serial.println("  savepose <nome>                 -> Salva pose atual (Gerenciamento de Poses)");
  Serial.println("  delpose <nome> | all            -> Apaga pose(s) (Gerenciamento de Poses)");
  Serial.println("  help                            -> Mostra esta ajuda");
  Serial.println("---------------------------------------------------------");
}

void handleSerialInput() {
  if (!Serial.available()) return;

  String input = Serial.readStringUntil('\n');
  input.trim();
  if (input.length() == 0) return;

  // --- Roteamento de Comandos (Calibration, Storage, PoseManager) ---
  if (input.equalsIgnoreCase("mostrar")) {
    showCurrentCalibration();
  } else if (input.equalsIgnoreCase("listposes")) {
    listPoses();
  } else if (input.startsWith("savepose ")) {
    savePose(input.substring(9));
  } else if (input.startsWith("save")) {
    saveToEEPROM();
  } else if (input.startsWith("load")) {
    loadFromEEPROM();
  } else if (input.equalsIgnoreCase("help")) {
    printHelp();
  } else if (input.startsWith("delpose ")) {
    String name = input.substring(8);
    name.trim();
    if (name.equalsIgnoreCase("all")) {
      // Deletar todas as poses requer loop na EEPROM
      for (int i = 0; i < MAX_POSES; i++) {
        Pose empty = {0};
        EEPROM.put(sizeof(StoredData) + i * sizeof(Pose), empty);
      }
      EEPROM.commit();
      Serial.println("Todas as poses foram apagadas.");
    } else {
      deletePose(name);
    }
  }
  
  // --- Comandos de Calibração ---
  else if (input.startsWith("min ")) {
    int idx, val;
    if (sscanf(input.c_str(), "min %d %d", &idx, &val) == 2) {
      setMinAngle(idx, val);
    } else Serial.println("Formato inválido.");
  } else if (input.startsWith("max ")) {
    int idx, val;
    if (sscanf(input.c_str(), "max %d %d", &idx, &val) == 2) {
      setMaxAngle(idx, val);
    } else Serial.println("Formato inválido.");
  } else if (input.startsWith("offset ")) {
    int idx, val;
    if (sscanf(input.c_str(), "offset %d %d", &idx, &val) == 2) {
      setOffset(idx, val);
    } else Serial.println("Formato inválido.");
  }

  // --- Comandos de Movimento (MotionController, PoseManager) ---
  
  else if (input.startsWith("loadpose ")) {
    char name[POSE_NAME_LEN] = {0};
    unsigned long duration = 0; 
    
    int params = sscanf(input.c_str(), "loadpose %s %lu", name, &duration);
    
    if(params >= 1) {
        loadPose(String(name), duration); 
    } else {
        Serial.println("Formato inválido. Use: loadpose <nome> [tempo_ms]");
    }
  } 
  
  else if (input.startsWith("set ")) {
    // Cria um novo destino temporário baseado na posição atual (currentAngles)
    int tempTarget[NUM_SERVOS];
    for(int i=0; i<NUM_SERVOS; i++) tempTarget[i] = currentAngles[i];

    if (input.startsWith("set ombro ")) {
      int angle;
      unsigned long duration = 0; 
      
      sscanf(input.c_str(), "set ombro %d %lu", &angle, &duration);
      
      tempTarget[1] = angle;
      tempTarget[2] = angle;
      
      if (duration == 0) duration = calculateDurationBySpeed(tempTarget);
      
      Serial.printf("Movendo ombro para %d° em %lu ms (Movimento Concorrente)....\n", angle, duration);
      startSmoothMove(tempTarget, duration);
      
    } else {
      int servo_idx, angle;
      unsigned long duration = 0; 
      int params = sscanf(input.c_str(), "set %d %d %lu", &servo_idx, &angle, &duration);
      
      if (params >= 2 && servo_idx >= 0 && servo_idx < NUM_SERVOS) {
        tempTarget[servo_idx] = angle;
        
        if (duration == 0) duration = calculateDurationBySpeed(tempTarget);
        
        Serial.printf("Movendo servo %d para %d° em %lu ms (Movimento Concorrente)....\n", servo_idx, angle, duration);
        startSmoothMove(tempTarget, duration);
      } else {
        Serial.println("Formato inválido. Use: set <servo> <angulo> [tempo_ms]");
      }
    }
  } 
  
  else if (input.startsWith("align ombro")) {
      unsigned long duration = 0; 
      sscanf(input.c_str(), "align ombro %lu", &duration);
      
      int tempTarget[NUM_SERVOS];
      for(int i=0; i<NUM_SERVOS; i++) tempTarget[i] = currentAngles[i];
      
      int media = (currentAngles[1] + currentAngles[2]) / 2;
      tempTarget[1] = media;
      tempTarget[2] = media;
      
      if (duration == 0) duration = calculateDurationBySpeed(tempTarget);
      
      Serial.printf("Alinhando ombros para %d° em %lu ms (Movimento Concorrente)....\n", media, duration);
      startSmoothMove(tempTarget, duration);
  } 
  
  else {
    Serial.printf("Comando não reconhecido: '%s'\n", input.c_str());
  }
}
