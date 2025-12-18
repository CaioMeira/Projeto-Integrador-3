// Arquivo de Configurações Globais e Estruturas
// Define todas as constantes, inclui bibliotecas essenciais e declara variáveis globais.

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <EEPROM.h>
#include <ESP32Servo.h>

// --- Configurações Globais ---
const int NUM_SERVOS = 7; // [0]Base, [1]Ombro1, [2]Ombro2, [3]Cotovelo, [4]Mão, [5]Pulso, [6]Garra
const int EEPROM_SIZE = 4096;
const uint32_t EEPROM_MAGIC = 0xDEADBEEF;

// Mapeamento dos pinos do ESP32 para cada servo
const int servoPins[NUM_SERVOS] = {18, 4, 13, 27, 26, 33, 32};

// --- Configuração de Poses e Macros ---
const int MAX_POSES = 10;
const int POSE_NAME_LEN = 10;
const int MAX_MACROS = 5;           // Número máximo de rotinas (Macros) que podem ser salvas
const int MAX_STEPS_PER_MACRO = 16; // Número máximo de passos em uma Macro

// --- Configuração de Velocidade ---
const int DEFAULT_SPEED_MS_PER_DEGREE = 25;
const int MIN_MOVE_DURATION = 300;

struct ArmKinematicsConfig
{
  float baseHeightMm;
  float upperLenMm;
  float forearmLenMm;
  float wristOffsetMm;
  float gripperLenMm;
  float minReachMm;
  float maxReachMm;
};

struct NeutralPose
{
  int base;
  int shoulder;
  int elbow;
  int hand;
  int wristRotate;
  int gripper;
};

constexpr ArmKinematicsConfig ARM_KINEMATICS = {
    100.0f, // baseHeightMm
    120.0f, // upperLenMm
    130.0f, // forearmLenMm
    40.0f,  // wristOffsetMm
    50.0f,  // gripperLenMm
    80.0f,  // minReachMm
    330.0f  // maxReachMm
};

constexpr NeutralPose IK_NEUTRAL = {
    90,  // base
    130, // shoulder
    100, // elbow
    70,  // hand
    120, // wristRotate
    100  // gripper
};

// --- Estruturas de Dados ---

/**
 * @brief Estrutura para salvar o estado de calibração e última posição na EEPROM (V1 - LEGACY).
 */
struct StoredData
{
  uint32_t magic;          /**< Identificador de dados válidos. */
  int current[NUM_SERVOS]; /**< Última posição conhecida. */
  int minv[NUM_SERVOS];    /**< Limites mínimos. */
  int maxv[NUM_SERVOS];    /**< Limites máximos. */
  int offs[NUM_SERVOS];    /**< Offsets de calibração. */
};

/**
 * @brief Estrutura OTIMIZADA V2 para salvar estado (35 bytes vs 116 bytes V1).
 * Usa uint8_t para ângulos (0-180° cabe em 1 byte) e inclui CRC16 para validação.
 */
struct StoredDataV2
{
  uint32_t magic;              /**< Identificador de dados válidos (0xDEADBEEF). */
  uint8_t version;             /**< Versão do formato (2). */
  uint8_t current[NUM_SERVOS]; /**< Última posição conhecida (0-180°). */
  uint8_t minv[NUM_SERVOS];    /**< Limites mínimos (0-180°). */
  uint8_t maxv[NUM_SERVOS];    /**< Limites máximos (0-180°). */
  int8_t offs[NUM_SERVOS];     /**< Offsets de calibração (-127 a +127). */
  uint16_t crc16;              /**< CRC16 para validação de integridade. */
} __attribute__((packed));

/**
 * @brief Estrutura para salvar uma pose (conjunto de ângulos) - V1 LEGACY.
 */
struct Pose
{
  char name[POSE_NAME_LEN]; /**< Nome da pose. */
  int angles[NUM_SERVOS];   /**< Ângulos para cada servo. */
};

/**
 * @brief Estrutura OTIMIZADA para pose (8 bytes vs 38 bytes V1).
 */
struct PoseCompact
{
  uint8_t angles[NUM_SERVOS]; /**< Ângulos (0-180°, 1 byte cada). */
  uint8_t flags;              /**< Flags reservadas para uso futuro. */
} __attribute__((packed));

// --- Endereços de Memória (Start) ---
// V1 (LEGACY - compatibilidade temporária)
const int POSES_START = sizeof(StoredData);
const int MACROS_START = POSES_START + (MAX_POSES * sizeof(struct Pose));

// V2 (OTIMIZADO - novo formato)
const int POSES_START_V2 = sizeof(StoredDataV2);
const int MACROS_START_V2 = POSES_START_V2 + (MAX_POSES * sizeof(PoseCompact));

/**
 * @brief Estrutura para definir um único passo dentro de uma Macro - V1 LEGACY.
 */
struct MacroStep
{
  char poseName[POSE_NAME_LEN]; /**< Nome da pose a ser carregada. */
  unsigned long delay_ms;       /**< Tempo de espera após atingir a pose (em ms). */
};

/**
 * @brief Estrutura OTIMIZADA para passo de macro (3 bytes vs 14 bytes V1).
 */
struct MacroStepCompact
{
  uint8_t poseIndex; /**< Índice da pose (0-9 para MAX_POSES=10). */
  uint16_t delay_ms; /**< Delay em ms (0-65535, ~65 segundos). */
} __attribute__((packed));

/**
 * @brief Estrutura para definir uma sequência completa de movimentos e esperas - V1 LEGACY.
 */
struct Macro
{
  char name[POSE_NAME_LEN];             /**< Nome da Macro. */
  int numSteps;                         /**< Número de passos atualmente usados na sequência. */
  MacroStep steps[MAX_STEPS_PER_MACRO]; /**< Lista de passos da rotina. */
};

/**
 * @brief Estrutura OTIMIZADA de Macro (50 bytes vs 238 bytes V1).
 */
struct MacroCompact
{
  uint8_t numSteps;                            /**< Número de passos (0-16). */
  uint8_t flags;                               /**< Flags reservadas. */
  MacroStepCompact steps[MAX_STEPS_PER_MACRO]; /**< Passos compactos. */
} __attribute__((packed));

// --- Tabela de Nomes (para UI) ---
/**
 * @brief Tabela de nomes das poses e macros (mantida em RAM para interface).
 * Estruturas compactas não armazenam nomes para economizar EEPROM.
 */
struct NameTable
{
  char poseNames[MAX_POSES][POSE_NAME_LEN];   /**< Nomes das poses. */
  char macroNames[MAX_MACROS][POSE_NAME_LEN]; /**< Nomes das macros. */
};

// --- Funções Utilitárias ---
/**
 * @brief Calcula CRC16 (padrão Modbus) para validação de dados.
 * @param data Ponteiro para os dados.
 * @param len Tamanho dos dados em bytes.
 * @return Valor CRC16 calculado.
 */
inline uint16_t calcCRC16(const uint8_t *data, size_t len)
{
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++)
  {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++)
    {
      if (crc & 0x0001)
      {
        crc = (crc >> 1) ^ 0xA001;
      }
      else
      {
        crc >>= 1;
      }
    }
  }
  return crc;
}

// --- Variáveis Globais Core (Extern) ---

// Variáveis de calibração e posição
extern int currentAngles[NUM_SERVOS]; /**< Última posição lógica interpolada de cada servo (0-180°). */
extern int minAngles[NUM_SERVOS];     /**< Ângulo mínimo permitido (limite de software). */
extern int maxAngles[NUM_SERVOS];     /**< Ângulo máximo permitido (limite de software). */
extern int offsets[NUM_SERVOS];       /**< Offset de calibração aplicado antes de escrever no servo (-90 a +90). */

// Objetos de Hardware
extern Servo servos[NUM_SERVOS];

#endif // CONFIG_H
