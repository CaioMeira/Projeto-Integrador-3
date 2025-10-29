/**
 * MacroManager.cpp
 * Implementação da lógica de persistência das Macros.
 */
#include "MacroManager.h"

namespace MacroManager
{

    // Função auxiliar interna para ler uma macro
    void readMacro(int index, Macro &macro)
    {
        EEPROM.get(MACROS_START + index * sizeof(Macro), macro);
    }

    // Função auxiliar interna para escrever uma macro
    void writeMacro(int index, const Macro &macro)
    {
        EEPROM.put(MACROS_START + index * sizeof(Macro), macro);
    }

    void listMacros()
    {
        Serial.println(F("\n--- Macros Salvas ---"));
        int count = 0;
        for (int i = 0; i < MAX_MACROS; i++)
        {
            Macro m;
            readMacro(i, m);
            if (m.name[0] != 0)
            {
                Serial.printf(" [%d] %s (%d passos)\n", i, m.name, m.numSteps);
                count++;
            }
        }
        if (count == 0)
        {
            Serial.println(F(" Nenhuma macro encontrada."));
        }
        Serial.printf(F(" Total: %d de %d slots usados.\n"), count, MAX_MACROS);
    }

    bool saveMacro(const Macro &macro)
    {
        int emptySlot = -1;
        for (int i = 0; i < MAX_MACROS; i++)
        {
            Macro m;
            readMacro(i, m);
            if (m.name[0] == 0)
            {
                if (emptySlot == -1)
                    emptySlot = i;
            }
            else if (strncmp(m.name, macro.name, POSE_NAME_LEN) == 0)
            {
                emptySlot = i; // Sobrescreve
                break;
            }
        }

        if (emptySlot != -1)
        {
            writeMacro(emptySlot, macro);
            EEPROM.commit();
            Serial.printf(F("Macro '%s' salva no slot %d.\n"), macro.name, emptySlot);
            return true;
        }
        else
        {
            Serial.println(F("ERRO: Não há slots de macro disponíveis."));
            return false;
        }
    }

    bool loadMacroByName(const char *name, Macro &macro)
    {
        for (int i = 0; i < MAX_MACROS; i++)
        {
            readMacro(i, macro);
            if (macro.name[0] != 0 && strncmp(macro.name, name, POSE_NAME_LEN) == 0)
            {
                return true;
            }
        }
        return false; // Não encontrada
    }

    void deleteMacro(const char *name)
    {
        if (strncmp(name, "all", 3) == 0)
        {
            for (int i = 0; i < MAX_MACROS; i++)
            {
                Macro emptyMacro = {0};
                writeMacro(i, emptyMacro);
            }
            EEPROM.commit();
            Serial.println(F("Todas as macros foram apagadas."));
            return;
        }

        for (int i = 0; i < MAX_MACROS; i++)
        {
            Macro m;
            readMacro(i, m);
            if (m.name[0] != 0 && strncmp(m.name, name, POSE_NAME_LEN) == 0)
            {
                Macro emptyMacro = {0};
                writeMacro(i, emptyMacro);
                EEPROM.commit();
                Serial.printf(F("Macro '%s' apagada.\n"), name);
                return;
            }
        }
        Serial.printf(F("Macro '%s' não encontrada.\n"), name);
    }

} // namespace MacroManager
