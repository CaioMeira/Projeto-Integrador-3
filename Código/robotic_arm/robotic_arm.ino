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
#include <esp_task_wdt.h>

#include "RosInterface.h"

// Configuração do Watchdog Timer (15 segundos)
// #define WDT_TIMEOUT 15

/**
 * @brief Configuração inicial do sistema.
 */
void setup()
{
  Serial.begin(115200);
  while (!Serial)
    ;         // Espera a Serial conectar (para placas com USB nativo)
  delay(500); // Dá um tempo para a Serial estabilizar
  Serial.println(F("\nIniciando Sistema do Braco Robotico v5.0..."));

  // Configura o Watchdog Timer
  // esp_task_wdt_init(WDT_TIMEOUT, true); // 15 segundos, panic em timeout
  // esp_task_wdt_add(NULL);               // Adiciona a task atual (loop)
  Serial.println(F("Watchdog Timer configurado (15s)."));

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

  // Inicializa a interface micro-ROS (Serial/USB)
  // IMPORTANTE: Deve ser chamado após todos os outros módulos estarem configurados
  RosInterface::setup();
  Serial.println(F("Sistema completo inicializado. Pronto para comandos Serial e ROS."));
}

/**
 * @brief Loop principal de execução.
 * Este loop é não-bloqueante e roda o mais rápido possível.
 */
void loop()
{
  // Reset do Watchdog Timer a cada iteração do loop
  // esp_task_wdt_reset();

  // 1. Atualiza a máquina de estados do movimento (interpolação)
  // Isso move fisicamente os servos a cada passo.
  MotionController::update();

  // 2. Atualiza a máquina de estados do sequenciador (macros)
  // Isso verifica se um movimento terminou para iniciar uma espera ou o próximo passo.
  Sequencer::update();

  // 3. Verifica e processa novos comandos da Serial
  // Isso lê a entrada do usuário.
  CommandParser::handleSerialInput();

  // 4. Processa mensagens ROS (subscribers, publishers, timers)
  // Isso permite controle via ROS 2 topics (não-bloqueante)
  RosInterface::update();
}
