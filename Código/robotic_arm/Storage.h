/**
 * @file Storage.h
 * @brief Define a interface do módulo de persistência (EEPROM).
 * Salva e carrega o estado de calibração e a última posição.
 */
#ifndef STORAGE_H
#define STORAGE_H

#include "Config.h"

namespace Storage
{

    /**
     * @brief Salva o estado atual (calibração e posição) na EEPROM.
     */
    void saveToEEPROM();

    /**
     * @brief Carrega o estado da EEPROM.
     * @param move Se true, move suavemente para a posição salva.
     * Se false, apenas define os valores sem mover.
     * @return true se dados válidos foram carregados, false caso contrário.
     */
    bool loadFromEEPROM(bool move);

} // namespace Storage

#endif // STORAGE_H
