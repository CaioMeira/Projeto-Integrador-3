/**
 * ROBOTIC ARM - ESP32
 * Versão 5.0 agora vai
 *
 * Ponto de entrada principal.
 * Coordena a inicialização dos módulos e o loop principal.
 */

// Inclui todos os cabeçalhos dos módulos necessários
#include "Config.h"
#include "MotionController.h"
#include "CommandParser.h"
#include "Storage.h"
#include "Sequencer.h"

/**
 * @brief Configuração inicial do sistema.
 */
void setup()
{
  Serial.begin(115200);
  while (!Serial)
    ;         // Espera a Serial conectar (para placas com USB nativo)
  delay(500); // Dá um tempo para a Serial estabilizar
  Serial.println(F("\nIniciando Sistema do Braço Robótico v5.0..."));

  // Inicializa a memória EEPROM
  EEPROM.begin(EEPROM_SIZE);

  // Configura os servos e os move para a posição neutra inicial
  MotionController::setup();

  // Tenta carregar o último estado salvo (calibração e posição)
  // O 'true' indica para mover suavemente para a posição salva.
  if (!Storage::loadFromEEPROM(true))
  {
    Serial.println(F("AVISO: EEPROM vazia ou inválida. Usando valores de fallback."));
  }

  // Mostra o menu de ajuda inicial
  CommandParser::setup();
}

/**
 * @brief Loop principal de execução.
 * Este loop é não-bloqueante e roda o mais rápido possível.
 */
void loop()
{
  // 1. Atualiza a máquina de estados do movimento (interpolação)
  // Isso move fisicamente os servos a cada passo.
  MotionController::update();

  // 2. Atualiza a máquina de estados do sequenciador (macros)
  // Isso verifica se um movimento terminou para iniciar uma espera ou o próximo passo.
  Sequencer::update();

  // 3. Verifica e processa novos comandos da Serial
  // Isso lê a entrada do usuário.
  CommandParser::handleSerialInput();
}
