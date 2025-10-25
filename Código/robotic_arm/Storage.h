/**
 * @file Storage.h
 * @brief Define a interface para o módulo responsável por salvar e carregar o estado
 * de calibração e a última posição na EEPROM.
 */

#ifndef STORAGE_H
#define STORAGE_H

#include "Config.h"

// --- Protótipos de Funções ---

/**
 * @brief Salva o estado atual do braço (ângulos, limites, offsets) na EEPROM.
 */
void saveToEEPROM();

/**
 * @brief Carrega o estado de calibração e a última posição salva da EEPROM.
 * * @return true Se os dados foram carregados e validados.
 * @return false Se a EEPROM estiver vazia ou inválida.
 */
bool loadFromEEPROM();

#endif // STORAGE_H
