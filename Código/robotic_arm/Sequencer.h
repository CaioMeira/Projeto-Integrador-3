/**
 * Sequencer.h
 * Responsável pela MÁQUINA DE ESTADOS que executa
 * uma macro (sequência de poses e esperas) de forma não-bloqueante.
 */
#ifndef SEQUENCER_H
#define SEQUENCER_H

namespace Sequencer {

/**
 * @brief Inicializa e inicia a execução de uma macro pelo nome.
 * @param name Nome da macro a ser carregada e executada.
 */
void startMacro(const char* name);

/**
 * @brief Para imediatamente a execução da macro atual.
 */
void stopMacro();

/**
 * @brief Retorna se o sequenciador está atualmente executando uma macro.
 */
bool isRunning();

/**
 * @brief Função de atualização da máquina de estados.
 * Deve ser chamada a cada iteração do loop() principal.
 */
void update();

} // namespace Sequencer

#endif // SEQUENCER_H
