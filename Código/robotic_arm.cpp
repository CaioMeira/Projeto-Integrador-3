#include <EEPROM.h>
#include <ESP32Servo.h>

// --- Configurações Globais ---
const int NUM_SERVOS = 7; // Base, Ombro1, Ombro2, Cotovelo, Mão, Pulso, Garra
Servo servos[NUM_SERVOS];
int servoPins[NUM_SERVOS] = {13, 14, 15, 16, 17, 18, 19};

// --- Variáveis de Estado dos Servos ---
int currentAngles[NUM_SERVOS];
int minAngles[NUM_SERVOS];
int maxAngles[NUM_SERVOS];
int offsets[NUM_SERVOS];

// --- Configurações de EEPROM e Poses ---
const int MAX_POSES = 10;
const int POSE_NAME_LEN = 10;
const int EEPROM_SIZE = 4096;
const uint32_t EEPROM_MAGIC = 0xDEADBEEF;

struct StoredData {
  uint32_t magic;
  int current[NUM_SERVOS];
  int minv[NUM_SERVOS];
  int maxv[NUM_SERVOS];
  int offs[NUM_SERVOS];
};

struct Pose {
  char name[POSE_NAME_LEN];
  int angles[NUM_SERVOS];
};

const int POSES_START = sizeof(StoredData);

// --- Variáveis de Controle de Movimento Suave (NÃO-BLOQUEANTE) ---
bool isMoving = false;
unsigned long moveStartTime;
unsigned long moveDuration;
int startAngles[NUM_SERVOS];
int targetAngles[NUM_SERVOS];

// --- Funções de Movimento Suave ---
void startSmoothMove(const int newTargetAngles[NUM_SERVOS], unsigned long duration) {
  if (duration == 0) { // Movimento instantâneo
    for (int i = 0; i < NUM_SERVOS; i++) {
      currentAngles[i] = constrain(newTargetAngles[i], minAngles[i], maxAngles[i]);
      int correctedAngle = constrain(currentAngles[i] + offsets[i], 0, 180);
      servos[i].write(correctedAngle);
    }
    isMoving = false;
    return;
  }

  // Prepara para o movimento suave
  moveStartTime = millis();
  moveDuration = duration;
  for (int i = 0; i < NUM_SERVOS; i++) {
    startAngles[i] = currentAngles[i];
    targetAngles[i] = constrain(newTargetAngles[i], minAngles[i], maxAngles[i]);
  }
  isMoving = true;
}

void updateSmoothMovement() {
  if (!isMoving) return;

  unsigned long elapsedTime = millis() - moveStartTime;

  if (elapsedTime >= moveDuration) {
    // Movimento concluído, garante a posição final
    for (int i = 0; i < NUM_SERVOS; i++) {
      currentAngles[i] = targetAngles[i];
      int correctedAngle = constrain(currentAngles[i] + offsets[i], 0, 180);
      servos[i].write(correctedAngle);
    }
    isMoving = false;
    Serial.println("✅ Movimento concluído.");
  } else {
    // Interpola a posição para cada servo
    float progress = (float)elapsedTime / (float)moveDuration;
    for (int i = 0; i < NUM_SERVOS; i++) {
      int interpolatedAngle = startAngles[i] + (targetAngles[i] - startAngles[i]) * progress;
      currentAngles[i] = interpolatedAngle; // Atualiza a posição atual "lógica"
      int correctedAngle = constrain(interpolatedAngle + offsets[i], 0, 180);
      servos[i].write(correctedAngle);
    }
  }
}


// --- Funções de EEPROM (sem alterações significativas na lógica) ---
void saveToEEPROM() {
  StoredData sd;
  sd.magic = EEPROM_MAGIC;
  for (int i = 0; i < NUM_SERVOS; i++) {
    sd.current[i] = currentAngles[i];
    sd.minv[i] = minAngles[i];
    sd.maxv[i] = maxAngles[i];
    sd.offs[i] = offsets[i];
  }
  EEPROM.put(0, sd);
  EEPROM.commit();
  Serial.println("💾 Posições, limites e offsets salvos na EEPROM.");
}

bool loadFromEEPROM() {
  StoredData sd;
  EEPROM.get(0, sd);
  if (sd.magic != EEPROM_MAGIC) return false;

  int initialAngles[NUM_SERVOS];
  for (int i = 0; i < NUM_SERVOS; i++) {
    minAngles[i] = constrain(sd.minv[i], 0, 180);
    maxAngles[i] = constrain(sd.maxv[i], 0, 180);
    offsets[i] = constrain(sd.offs[i], -180, 180);
    initialAngles[i] = sd.current[i];
  }
  
  // Inicia na posição salva com um movimento suave
  startSmoothMove(initialAngles, 1500); // Move para a posição salva em 1.5s
  
  Serial.println("✅ Posições, limites e offsets restaurados da EEPROM.");
  return true;
}

// --- Funções de Pose (adaptadas para o novo sistema de movimento) ---
void savePose(String name) {
  if (name.length() > POSE_NAME_LEN - 1) name = name.substring(0, POSE_NAME_LEN - 1);
  char cname[POSE_NAME_LEN];
  name.toCharArray(cname, POSE_NAME_LEN);

  for (int i = 0; i < MAX_POSES; i++) {
    Pose p;
    EEPROM.get(POSES_START + i * sizeof(Pose), p);
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

bool loadPose(String name, unsigned long duration) {
  if (name.length() > POSE_NAME_LEN - 1) name = name.substring(0, POSE_NAME_LEN - 1);
  char cname[POSE_NAME_LEN];
  name.toCharArray(cname, POSE_NAME_LEN);

  for (int i = 0; i < MAX_POSES; i++) {
    Pose p;
    EEPROM.get(POSES_START + i * sizeof(Pose), p);
    if (p.name[0] != 0 && strncmp(p.name, cname, POSE_NAME_LEN) == 0) {
      Serial.printf("Carregando pose '%s' em %lu ms...\n", cname, duration);
      startSmoothMove(p.angles, duration); // Dispara o movimento suave
      return true;
    }
  }
  Serial.printf("Pose '%s' não encontrada\n", cname);
  return false;
}

void deletePose(String name) {
    if (name.length() > POSE_NAME_LEN - 1) name = name.substring(0, POSE_NAME_LEN - 1);
    char cname[POSE_NAME_LEN];
    name.toCharArray(cname, POSE_NAME_LEN);

    for (int i = 0; i < MAX_POSES; i++) {
        Pose p;
        EEPROM.get(POSES_START + i * sizeof(Pose), p);
        if (strncmp(p.name, cname, POSE_NAME_LEN) == 0) {
            memset(&p, 0, sizeof(Pose)); // Limpa toda a estrutura da pose
            EEPROM.put(POSES_START + i * sizeof(Pose), p);
            EEPROM.commit();
            Serial.printf("Pose '%s' deletada.\n", cname);
            return;
        }
    }
    Serial.printf("Pose '%s' não encontrada para deletar.\n", cname);
}

// --- Funções Auxiliares (Listar, Ajuda, etc.) ---
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

void printHelp() {
  Serial.println("\n--- Comandos Disponíveis (v2.0 com Movimento Suave) ---");
  Serial.println("  set <servo> <angulo> [tempo_ms] -> Move servo individual");
  Serial.println("  set ombro <angulo> [tempo_ms]   -> Move os dois servos do ombro");
  Serial.println("  min <servo> <angulo>            -> Define ângulo mínimo");
  Serial.println("  max <servo> <angulo>            -> Define ângulo máximo");
  Serial.println("  offset <servo> <valor>          -> Define offset de calibração");
  Serial.println("  align ombro [tempo_ms]          -> Alinha ombros na média");
  Serial.println("  mostrar                         -> Exibe valores atuais e limites");
  Serial.println("  listposes                       -> Lista todas as poses salvas");
  Serial.println("  save                            -> Salva estado atual na EEPROM");
  Serial.println("  load                            -> Carrega estado da EEPROM");
  Serial.println("  savepose <nome>                 -> Salva pose atual com um nome");
  Serial.println("  loadpose <nome> [tempo_ms]      -> Carrega pose com movimento suave");
  Serial.println("  delpose <nome> | all            -> Apaga pose(s)");
  Serial.println("  help                            -> Mostra esta ajuda");
  Serial.println("---------------------------------------------------------");
}

void handleSerialInput(); // Declaração antecipada

// --- SETUP e LOOP ---
void setup() {
  Serial.begin(115200);
  delay(100);
  EEPROM.begin(EEPROM_SIZE);

  // Limites e posição inicial padrão (fallback)
  int safeMin[NUM_SERVOS]     = {0, 95, 95, 50, 0, 60, 55};
  int safeMax[NUM_SERVOS]     = {180, 180, 180, 180, 180, 180, 155};
  int safeNeutral[NUM_SERVOS] = {90, 130, 130, 100, 70, 120, 100};

  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].attach(servoPins[i]);
    minAngles[i] = safeMin[i];
    maxAngles[i] = safeMax[i];
    offsets[i] = 0;
    currentAngles[i] = safeNeutral[i];
    servos[i].write(constrain(safeNeutral[i], 0, 180));
    delay(50);
  }

  if (!loadFromEEPROM()) {
    Serial.println("⚠️ EEPROM vazia ou inválida. Usando valores iniciais.");
  }
  
  printHelp();
}

void loop() {
  updateSmoothMovement(); 
  handleSerialInput();
}

// --- Lógica de Processamento de Comandos ---
void handleSerialInput() {
  if (!Serial.available()) return;

  String input = Serial.readStringUntil('\n');
  input.trim();
  if (input.length() == 0) return;

  if (input.equalsIgnoreCase("mostrar")) {
    for (int i = 0; i < NUM_SERVOS; i++) {
      Serial.printf("Servo %d | atual:%3d | min:%3d | max:%3d | offset:%+3d | out:%3d\n",
        i, currentAngles[i], minAngles[i], maxAngles[i], offsets[i],
        constrain(currentAngles[i] + offsets[i], 0, 180));
    }
  } else if (input.equalsIgnoreCase("listposes")) {
    listPoses();
  } else if (input.startsWith("savepose ")) {
    savePose(input.substring(9));
  } else if (input.startsWith("loadpose ")) {
    char name[POSE_NAME_LEN] = {0};
    int duration = 1000; // Duração padrão de 1s
    sscanf(input.c_str(), "loadpose %s %d", name, &duration);
    loadPose(String(name), duration);
  } else if (input.startsWith("set ")) {
    int tempTarget[NUM_SERVOS];
    for(int i=0; i<NUM_SERVOS; i++) tempTarget[i] = currentAngles[i];

    if (input.startsWith("set ombro ")) {
      int angle, duration = 200; // Duração padrão de 200ms
      sscanf(input.c_str(), "set ombro %d %d", &angle, &duration);
      tempTarget[1] = angle;
      tempTarget[2] = angle;
      Serial.printf("Movendo ombro para %d° em %d ms...\n", angle, duration);
      startSmoothMove(tempTarget, duration);
    } else {
      int servo_idx, angle, duration = 200;
      int params = sscanf(input.c_str(), "set %d %d %d", &servo_idx, &angle, &duration);
      if (params >= 2 && servo_idx >= 0 && servo_idx < NUM_SERVOS) {
        tempTarget[servo_idx] = angle;
        Serial.printf("Movendo servo %d para %d° em %d ms...\n", servo_idx, angle, duration);
        startSmoothMove(tempTarget, duration);
      } else {
        Serial.println("Formato inválido. Use: set <servo> <angulo> [tempo_ms]");
      }
    }
  } else if (input.startsWith("min ")) {
    int idx, val;
    if (sscanf(input.c_str(), "min %d %d", &idx, &val) == 2 && idx >= 0 && idx < NUM_SERVOS) {
      minAngles[idx] = constrain(val, 0, 180);
      Serial.printf("Mínimo do servo %d definido para %d°\n", idx, val);
    } else Serial.println("Formato inválido.");
  } else if (input.startsWith("max ")) {
    int idx, val;
    if (sscanf(input.c_str(), "max %d %d", &idx, &val) == 2 && idx >= 0 && idx < NUM_SERVOS) {
      maxAngles[idx] = constrain(val, 0, 180);
      Serial.printf("Máximo do servo %d definido para %d°\n", idx, val);
    } else Serial.println("Formato inválido.");
  } else if (input.startsWith("offset ")) {
    int idx, val;
    if (sscanf(input.c_str(), "offset %d %d", &idx, &val) == 2 && idx >= 0 && idx < NUM_SERVOS) {
      offsets[idx] = constrain(val, -90, 90);
      // Aplica o offset imediatamente para visualização
      servos[idx].write(constrain(currentAngles[idx] + offsets[idx], 0, 180));
      Serial.printf("Offset do servo %d ajustado para %+d°\n", idx, val);
    } else Serial.println("Formato inválido.");
  } else if (input.startsWith("align ombro")) {
      int duration = 500;
      sscanf(input.c_str(), "align ombro %d", &duration);
      int media = (currentAngles[1] + currentAngles[2]) / 2;
      int tempTarget[NUM_SERVOS];
      for(int i=0; i<NUM_SERVOS; i++) tempTarget[i] = currentAngles[i];
      tempTarget[1] = media;
      tempTarget[2] = media;
      Serial.printf("Alinhando ombros para %d° em %dms...\n", media, duration);
      startSmoothMove(tempTarget, duration);
  } else if (input.equalsIgnoreCase("save")) {
    saveToEEPROM();
  } else if (input.equalsIgnoreCase("load")) {
    loadFromEEPROM();
  } else if (input.equalsIgnoreCase("help")) {
    printHelp();
  } else if (input.startsWith("delpose ")) {
    String name = input.substring(8);
    name.trim();
    if (name.equalsIgnoreCase("all")) {
      for (int i = 0; i < MAX_POSES; i++) {
        Pose empty = {0};
        EEPROM.put(POSES_START + i * sizeof(Pose), empty);
      }
      EEPROM.commit();
      Serial.println("Todas as poses foram apagadas.");
    } else {
      deletePose(name);
    }
  } else {
    Serial.printf("Comando não reconhecido: '%s'\n", input.c_str());
  }
}
