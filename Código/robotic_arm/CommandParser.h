/**
 * @file CommandParser.h
 * @brief Define a interface do módulo responsável por ler e interpretar comandos
 * recebidos via porta serial.
 */

#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include "Config.h"

// --- Protótipos de Funções ---

/**
 * @brief Lógica principal para processar comandos recebidos via porta serial.
 * * Não bloqueante.
 */
void handleSerialInput();

/**
 * @brief Exibe a lista de comandos disponíveis na Serial.
 */
void printHelp();

#endif // COMMAND_PARSER_H
