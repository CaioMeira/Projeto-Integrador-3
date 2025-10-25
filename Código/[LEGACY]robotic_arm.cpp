#include <EEPROM.h>
#include <ESP32Servo.h>

// --- Configurações Globais ---
const int NUM_SERVOS = 7; // [0]Base, [1]Ombro1, [2]Ombro2, [3]Cotovelo, [4]Mão, [5]Pulso, [6]Garra
Servo servos[NUM_SERVOS];
int servoPins[NUM_SERVOS] = {13, 14, 15, 16, 17, 18, 19};

// --- Variáveis de Estado e Calibração ---
int currentAngles[NUM_SERVOS]; /**< Última posição lógica interpolada de cada servo (0-180°). */
int minAngles[NUM_SERVOS];     /**< Ângulo mínimo permitido (limite de software). */
int maxAngles[NUM_SERVOS];     /**< Ângulo máximo permitido (limite de software). */
int offsets[NUM_SERVOS];       /**< Offset de calibração aplicado antes de escrever no servo (-90 a +90). */

// --- Configurações de EEPROM e Poses ---
const int MAX_POSES = 10;          /**< Número máximo de poses que podem ser salvas. */
const int POSE_NAME_LEN = 10;      /**< Tamanho máximo do nome da pose. */
const int EEPROM_SIZE = 4096;      /**< Tamanho da memória EEPROM virtual do ESP32. */
const uint32_t EEPROM_MAGIC = 0xDEADBEEF; /**< Número mágico para validar dados na EEPROM. */

// --- Configuração de Velocidade ---
// Define a velocidade padrão em milissegundos por grau.
const int DEFAULT_SPEED_MS_PER_DEGREE = 25; /**< 25 ms por 1 grau de movimento. */
const int MIN_MOVE_DURATION = 300;         /**< Duração mínima para qualquer movimento suave (em ms). */

/**
 * @brief Estrutura para salvar o estado de calibração e posição na EEPROM.
 */
struct StoredData {
  uint32_t magic;           /**< Identificador de dados válidos. */
  int current[NUM_SERVOS];  /**< Última posição conhecida. */
  int minv[NUM_SERVOS];     /**< Limites mínimos. */
  int maxv[NUM_SERVOS];     /**< Limites máximos. */
  int offs[NUM_SERVOS];     /**< Offsets de calibração. */
};

/**
 * @brief Estrutura para salvar uma pose (conjunto de ângulos).
 */
struct Pose {
  char name[POSE_NAME_LEN];   /**< Nome da pose. */
  int angles[NUM_SERVOS];     /**< Ângulos para cada servo. */
};

const int POSES_START = sizeof(StoredData); /**< Endereço inicial de armazenamento das Poses. */

// --- Variáveis de Controle de Movimento Suave ---
bool isMoving = false;             /**< Flag que indica se há um movimento ativo em progresso. */
unsigned long moveStartTime;       /**< Timestamp em ms quando o movimento foi iniciado ou reiniciado. */
unsigned long moveDuration;        /**< Duração total (em ms) do movimento atual. */
int startAngles[NUM_SERVOS];       /**< Posição de cada servo no início do movimento. */
int targetAngles[NUM_SERVOS];      /**< Posição final desejada para cada servo. */

// =================================================================
// FUNÇÕES DE MOVIMENTO CONCORRENTE
// =================================================================

/**
 * @brief Calcula a duração necessária para um movimento com base na velocidade padrão.
 * * A duração é determinada pelo servo que tiver a maior diferença de ângulo
 * entre a posição atual (`currentAngles`) e o novo destino.
 * * @param newTargetAngles Array de 7 elementos com os novos ângulos de destino.
 * @return unsigned long Duração calculada do movimento em milissegundos (ms).
 */
unsigned long calculateDurationBySpeed(const int newTargetAngles[NUM_SERVOS]) {
  int maxDelta = 0;
  for (int i = 0; i < NUM_SERVOS; i++) {
    // Calcula o delta entre a POSIÇÃO LÓGICA ATUAL e o NOVO DESTINO
    int delta = abs(constrain(newTargetAngles[i], minAngles[i], maxAngles[i]) - currentAngles[i]);
    if (delta > maxDelta) {
      maxDelta = delta;
    }
  }
  unsigned long duration = (unsigned long)maxDelta * DEFAULT_SPEED_MS_PER_DEGREE;
  if (duration < MIN_MOVE_DURATION) {
    duration = MIN_MOVE_DURATION;
  }
  return duration;
}

/**
 * @brief Inicia ou atualiza um movimento suave para um novo destino.
 * * Esta função implementa a concorrência: se um movimento já está em andamento,
 * ele é imediatamente interrompido e um NOVO movimento é iniciado a partir da 
 * posição atual (`currentAngles`) em direção ao `newTargetAngles`.
 * * @param newTargetAngles Array de 7 elementos com os novos ângulos de destino.
 * @param duration Duração do novo movimento em milissegundos.
 */
void startSmoothMove(const int newTargetAngles[NUM_SERVOS], unsigned long duration) {
  if (duration == 0) { 
    // Movimento instantâneo
    for (int i = 0; i < NUM_SERVOS; i++) {
      currentAngles[i] = constrain(newTargetAngles[i], minAngles[i], maxAngles[i]);
      int correctedAngle = constrain(currentAngles[i] + offsets[i], 0, 180);
      servos[i].write(correctedAngle);
    }
    isMoving = false;
    Serial.println("✅ Movimento instantâneo concluído.");
    return;
  }
  
  // O ponto de partida é a posição atual (`currentAngles`), mesmo que seja um valor
  // interpolado de um movimento anterior que estava em progresso.
  moveStartTime = millis();
  moveDuration = duration;
  for (int i = 0; i < NUM_SERVOS; i++) {
    startAngles[i] = currentAngles[i];
    targetAngles[i] = constrain(newTargetAngles[i], minAngles[i], maxAngles[i]);
  }
  isMoving = true;
}


/**
 * @brief Executa a interpolação suave (easing) de todos os servos.
 * * Esta função deve ser chamada repetidamente no `loop()`. Ela calcula a posição 
 * atual de cada servo com base no tempo decorrido, aplicando uma curva de 
 * suavização Quadrática (Ease In/Out).
 */
void updateSmoothMovement() {
  if (!isMoving) return;
  unsigned long elapsedTime = millis() - moveStartTime;

  if (elapsedTime >= moveDuration) {
    // 1. Movimento concluído: Garante a posição final
    for (int i = 0; i < NUM_SERVOS; i++) {
      currentAngles[i] = targetAngles[i];
      int correctedAngle = constrain(currentAngles[i] + offsets[i], 0, 180);
      servos[i].write(correctedAngle);
    }
    isMoving = false;
    Serial.println("✅ Movimento concluído.");
  } else {
    // 2. Interpolação em progresso:
    float progress = (float)elapsedTime / (float)moveDuration;
    float easeProgress;

    // Função de Easing (Quadrático In/Out) para aceleração/desaceleração suave
    if (progress < 0.5f) {
      easeProgress = 2.0f * progress * progress; // EaseIn (aceleração)
    } else {
      // EaseOut (desaceleração)
      float t = -2.0f * progress + 2.0f;
      easeProgress = 1.0f - (t * t) / 2.0f; 
    }
    
    // Interpola a posição para cada servo
    for (int i = 0; i < NUM_SERVOS; i++) {
      int interpolatedAngle = startAngles[i] + (targetAngles[i] - startAngles[i]) * easeProgress;
      currentAngles[i] = interpolatedAngle; 
      int correctedAngle = constrain(interpolatedAngle + offsets[i], 0, 180);
      servos[i].write(correctedAngle);
    }
  }
}

// =================================================================
// FUNÇÕES DE EEPROM E POSE
// =================================================================

/**
 * @brief Salva o estado atual do braço (ângulos, limites, offsets) na EEPROM.
 */
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

/**
 * @brief Carrega o estado de calibração e a última posição salva da EEPROM.
 * * Se for bem-sucedido, inicia um movimento suave da posição de setup para a 
 * posição restaurada.
 * * @return true Se os dados foram carregados e validados.
 * @return false Se a EEPROM estiver vazia ou inválida.
 */
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
  
  unsigned long autoDuration = calculateDurationBySpeed(initialAngles);
  Serial.printf("✅ Restaurando posição da EEPROM em %lu ms...\n", autoDuration);
  startSmoothMove(initialAngles, autoDuration); 
  
  Serial.println("✅ Posições, limites e offsets restaurados da EEPROM.");
  return true;
}

/**
 * @brief Salva a posição atual (`currentAngles`) em um slot de pose.
 * * Se o nome já existir, a pose é sobrescrita.
 * * @param name Nome da pose a ser salva.
 */
void savePose(String name) {
  if (name.length() > POSE_NAME_LEN - 1) name = name.substring(0, POSE_NAME_LEN - 1);
  char cname[POSE_NAME_LEN];
  name.toCharArray(cname, POSE_NAME_LEN);

  for (int i = 0; i < MAX_POSES; i++) {
    Pose p;
    EEPROM.get(POSES_START + i * sizeof(Pose), p);
    // Verifica se o slot está vazio (p.name[0] == 0) ou se o nome já existe (sobrescreve)
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
 * * O movimento é imediato e concorre com qualquer movimento anterior.
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
  Serial.println("\n--- Comandos Disponíveis (v5.0 com Movimento Imediato) ---");
  Serial.println("  Todos os comandos de movimento são executados imediatamente e podem ser combinados.");
  Serial.println("  [tempo_ms] é opcional e anula a velocidade padrão.");
  Serial.println("  ---------------------------------------------------------");
  Serial.println("  set <servo> <angulo> [tempo_ms] -> Move servo individual (Imediato)");
  Serial.println("  set ombro <angulo> [tempo_ms]   -> Move os dois servos do ombro (Imediato)");
  Serial.println("  loadpose <nome> [tempo_ms]      -> Carrega pose (Imediato)");
  Serial.println("  align ombro [tempo_ms]          -> Alinha ombros na média (Imediato)");
  Serial.println("  ---------------------------------------------------------");
  Serial.println("  min <servo> <angulo>            -> Define ângulo mínimo (Limites de software)");
  Serial.println("  max <servo> <angulo>            -> Define ângulo máximo (Limites de software)");
  Serial.println("  offset <servo> <valor>          -> Define offset de calibração (-90 a 90)");
  Serial.println("  mostrar                         -> Exibe valores atuais, limites e offsets");
  Serial.println("  listposes                       -> Lista todas as poses salvas");
  Serial.println("  save                            -> Salva estado atual na EEPROM");
  Serial.println("  load                            -> Carrega estado da EEPROM");
  Serial.println("  savepose <nome>                 -> Salva pose atual com um nome");
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

  // Posições de segurança inicial, usadas se a EEPROM estiver vazia.
  int safeMin[NUM_SERVOS]     = {0, 95, 95, 50, 0, 60, 55};
  int safeMax[NUM_SERVOS]     = {180, 180, 180, 180, 180, 180, 155};
  int safeNeutral[NUM_SERVOS] = {90, 130, 130, 100, 70, 120, 100};

  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].attach(servoPins[i]);
    minAngles[i] = safeMin[i];
    maxAngles[i] = safeMax[i];
    offsets[i] = 0;
    currentAngles[i] = safeNeutral[i];
    servos[i].write(constrain(safeNeutral[i] + offsets[i], 0, 180));
    delay(50);
  }

  if (!loadFromEEPROM()) {
    Serial.println("⚠️ EEPROM vazia ou inválida. Usando valores iniciais.");
  }
  
  printHelp();
}

void loop() {
  // 1. Sempre atualiza o movimento em progresso.
  // Esta chamada é crucial para a concorrência e suavização.
  updateSmoothMovement(); 

  // 2. Processa novos comandos seriais imediatamente (não bloqueia).
  handleSerialInput();
}


/**
 * @brief Lógica principal para processar comandos recebidos via porta serial.
 * * Verifica a disponibilidade da serial e analisa a string de comando.
 */
void handleSerialInput() {
  if (!Serial.available()) return;

  String input = Serial.readStringUntil('\n');
  input.trim();
  if (input.length() == 0) return;

  // --- PARSING DE COMANDOS DE CALIBRAÇÃO E EEPROM ---
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
      servos[idx].write(constrain(currentAngles[idx] + offsets[idx], 0, 180)); 
      Serial.printf("Offset do servo %d ajustado para %+d°\n", idx, val);
    } else Serial.println("Formato inválido.");
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
  } 
  
  // --- PARSING DE COMANDOS DE MOVIMENTO (IMEDIATO/CONCORRENTE) ---
  
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
    // A chave da concorrência: o destino é baseado na posição atual (currentAngles)
    // para que movimentos incompletos sejam continuados/combinados.
    int tempTarget[NUM_SERVOS];
    for(int i=0; i<NUM_SERVOS; i++) tempTarget[i] = currentAngles[i];

    if (input.startsWith("set ombro ")) {
      int angle;
      unsigned long duration = 0; 
      
      sscanf(input.c_str(), "set ombro %d %lu", &angle, &duration);
      
      // Altera múltiplos servos no destino
      tempTarget[1] = angle;
      tempTarget[2] = angle;
      
      if (duration == 0) {
        duration = calculateDurationBySpeed(tempTarget);
      }
      
      Serial.printf("Movendo ombro para %d° em %lu ms (Movimento Concorrente)....\n", angle, duration);
      startSmoothMove(tempTarget, duration);
      
    } else {
      int servo_idx, angle;
      unsigned long duration = 0; 
      int params = sscanf(input.c_str(), "set %d %d %lu", &servo_idx, &angle, &duration);
      
      if (params >= 2 && servo_idx >= 0 && servo_idx < NUM_SERVOS) {
        // Altera apenas um servo no destino
        tempTarget[servo_idx] = angle;
        
        if (duration == 0) {
          duration = calculateDurationBySpeed(tempTarget);
        }
        
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
      
      if (duration == 0) {
        duration = calculateDurationBySpeed(tempTarget);
      }
      
      Serial.printf("Alinhando ombros para %d° em %lu ms (Movimento Concorrente)....\n", media, duration);
      startSmoothMove(tempTarget, duration);
  } 
  
  else {
    Serial.printf("Comando não reconhecido: '%s'\n", input.c_str());
  }
}
