/**
 * MacroManager.h
 * Responsável pela lógica de salvar, carregar e gerenciar
 * as 'Macros' (sequências de poses) na EEPROM.
 */
#ifndef MACRO_MANAGER_H
#define MACRO_MANAGER_H

#include "Config.h"

namespace MacroManager
{

    /**
     * @brief Lista todas as macros salvas na Serial.
     */
    void listMacros();

    /**
     * @brief Salva uma estrutura de Macro em um slot da EEPROM.
     * Procura um slot vazio ou com o mesmo nome.
     * @param macro A macro a ser salva.
     * @return true se foi salva com sucesso, false se a EEPROM estiver cheia.
     */
    bool saveMacro(const Macro &macro);

    /**
     * @brief Carrega uma macro da EEPROM pelo nome.
     * @param name Nome da macro a ser procurada.
     * @param macro [out] Referência onde a macro será carregada.
     * @return true se encontrada, false se não.
     */
    bool loadMacroByName(const char *name, Macro &macro);

    /**
     * @brief Deleta uma macro por nome, ou todas.
     * @param name Nome da macro, ou "all" para apagar todas.
     */
    void deleteMacro(const char *name);

} // namespace MacroManager

#endif // MACRO_MANAGER_H
