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
const int servoPins[NUM_SERVOS] = {13, 14, 15, 16, 17, 18, 19};

// --- Configuração de Poses ---
const int MAX_POSES = 10;
const int POSE_NAME_LEN = 10;

// --- Configuração de Velocidade ---
const int DEFAULT_SPEED_MS_PER_DEGREE = 25;
const int MIN_MOVE_DURATION = 300;

// --- Estruturas de Dados ---

/**
 * @brief Estrutura para salvar o estado de calibração e última posição na EEPROM.
 */
struct StoredData
{
  uint32_t magic;          /**< Identificador de dados válidos. */
  int current[NUM_SERVOS]; /**< Última posição conhecida. */
  int minv[NUM_SERVOS];    /**< Limites mínimos. */
  int maxv[NUM_SERVOS];    /**< Limites máximos. */
  int offs[NUM_SERVOS];    /**< Offsets de calibração. */
};

// --- Endereço de Memória (Start) ---
// O endereço inicial das poses é imediatamente após o bloco StoredData,
// que agora está completo e permite o cálculo do sizeof().
const int POSES_START = sizeof(StoredData);

/**
 * @brief Estrutura para salvar uma pose (conjunto de ângulos).
 */
struct Pose
{
  char name[POSE_NAME_LEN]; /**< Nome da pose. */
  int angles[NUM_SERVOS];   /**< Ângulos para cada servo. */
};

// --- Variáveis Globais Core (Extern) ---

// Variáveis de calibração e posição
extern int currentAngles[NUM_SERVOS]; /**< Última posição lógica interpolada de cada servo (0-180°). */ 
extern int minAngles[NUM_SERVOS];     /**< Ângulo mínimo permitido (limite de software). */
extern int maxAngles[NUM_SERVOS];     /**< Ângulo máximo permitido (limite de software). */
extern int offsets[NUM_SERVOS];       /**< Offset de calibração aplicado antes de escrever no servo (-90 a +90). */

// Objetos de Hardware
extern Servo servos[NUM_SERVOS];

#endif // CONFIG_H
