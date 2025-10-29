/**
 * CommandParser.cpp
 * Implementação do parser de comandos seriais.
 */
#include "CommandParser.h"
#include "Config.h"
#include "MotionController.h"
#include "Calibration.h" 
#include "Storage.h"     
#include "PoseManager.h"
#include "MacroManager.h"
#include "Sequencer.h"

namespace CommandParser
{
    // --- Variáveis de Estado para Gravação de Macro ---
    static bool isRecording = false; // Flag que indica se estamos no modo de gravação
    static Macro recordingMacro;     // Buffer temporário para a macro sendo gravada

    /**
     * @brief Função auxiliar interna para tratar comandos 'set'.
     */
    void handleSetCommand(String input)
    {
        int tempTarget[NUM_SERVOS];
        for (int i = 0; i < NUM_SERVOS; i++)
            tempTarget[i] = currentAngles[i];

        unsigned long duration = 0; // 0 = gatilho para cálculo automático
        int angle = 0;
        int servo_idx = -1;

        if (input.startsWith(F("set ombro ")))
        {
            int params = sscanf(input.c_str(), "set ombro %d %lu", &angle, &duration);
            if (params >= 1)
            {
                tempTarget[1] = angle;
                tempTarget[2] = angle;
                Serial.printf(F("Movendo ombro para %d°"), angle);
            }
            else
            {
                Serial.println(F("Formato: set ombro <angulo> [tempo]"));
                return;
            }
        }
        else
        {
            int params = sscanf(input.c_str(), "set %d %d %lu", &servo_idx, &angle, &duration);
            if (params >= 2 && servo_idx >= 0 && servo_idx < NUM_SERVOS)
            {
                tempTarget[servo_idx] = angle;
                Serial.printf(F("Movendo servo %d para %d°"), servo_idx, angle);
            }
            else
            {
                Serial.println(F("Formato: set <servo> <angulo> [tempo]"));
                return;
            }
        }

        // Se a duração não foi fornecida (continuou 0), calcula automaticamente
        if (duration == 0)
        {
            duration = MotionController::calculateDurationBySpeed(tempTarget);
            Serial.printf(F(" (duração calc: %lu ms)...\n"), duration);
        }
        else
        {
            Serial.printf(F(" (duração: %lu ms)...\n"), duration);
        }

        MotionController::startSmoothMove(tempTarget, duration);
    }

    /**
     * @brief Imprime o menu de ajuda completo na Serial.
     */
    void printHelp()
    {
        Serial.println(F("\n--- Comandos (v5.0 com Macros) ---"));
        Serial.println(F("--- Controle de Movimento ---"));
        Serial.println(F(" set <servo> <angulo> [tempo] - Move servo individual"));
        Serial.println(F(" set ombro <angulo> [tempo]   - Move ambos os ombros"));
        Serial.println(F("--- Calibração ---"));
        Serial.println(F(" min <servo> <angulo>         - Define ângulo mínimo"));
        Serial.println(F(" max <servo> <angulo>         - Define ângulo máximo"));
        Serial.println(F(" offset <servo> <valor>       - Define offset de calibração"));
        Serial.println(F(" align ombro [tempo]        - Alinha ombros na média"));
        Serial.println(F("--- Poses (Pontos Estáticos) ---"));
        Serial.println(F(" savepose <nome>              - Salva pose atual"));
        Serial.println(F(" loadpose <nome> [tempo]    - Carrega pose"));
        Serial.println(F(" delpose <nome> | all        - Apaga pose(s)"));
        Serial.println(F(" listposes                    - Lista poses salvas"));
        Serial.println(F("--- Macros (Rotinas/Sequências) ---"));
        Serial.println(F(" runmacro <nome>              - Executa uma macro (sequência)"));
        Serial.println(F(" stopmacro                    - Para a macro em execução"));
        Serial.println(F(" listmacros                   - Lista macros salvas"));
        Serial.println(F(" delmacro <nome> | all        - Apaga macro(s)"));
        Serial.println(F("--- Gravação de Macro ---"));
        Serial.println(F(" startrecord <nome>           - Inicia a gravação de uma macro"));
        Serial.println(F(" addstep <pose_name> <delay>  - Adiciona passo (pose + espera em ms)"));
        Serial.println(F(" saverecord                   - Salva a macro gravada"));
        Serial.println(F(" cancelrecord                 - Cancela a gravação"));
        Serial.println(F("--- Sistema ---"));
        Serial.println(F(" save                         - Salva calibração/posição na EEPROM"));
        Serial.println(F(" load                         - Carrega calibração/posição da EEPROM"));
        Serial.println(F(" mostrar                      - Exibe status atual dos servos"));
        Serial.println(F(" help                         - Mostra esta ajuda"));
        Serial.println(F("------------------------------------"));
    }

    void setup()
    {
        printHelp();
    }

    void handleSerialInput()
    {
        if (!Serial.available())
            return;

        String input = Serial.readStringUntil('\n');
        input.trim();
        if (input.length() == 0)
            return;

        // --- Comandos de Interrupção (Devem ser checados primeiro) ---
        if (input.equalsIgnoreCase(F("stopmacro")))
        {
            Sequencer::stopMacro();
            return;
        }

        // --- Comandos de Macro (Execução e Gerenciamento) ---
        if (input.startsWith(F("runmacro ")))
        {
            char name[POSE_NAME_LEN] = {0};
            sscanf(input.c_str(), "runmacro %s", name);
            if (name[0] != 0)
                Sequencer::startMacro(name);
            return; // Retorna para não conflitar com outros comandos
        }
        else if (input.equalsIgnoreCase(F("listmacros")))
        {
            MacroManager::listMacros();
            return;
        }
        else if (input.startsWith(F("delmacro ")))
        {
            char name[POSE_NAME_LEN] = {0};
            sscanf(input.c_str(), "delmacro %s", name);
            if (name[0] != 0)
                MacroManager::deleteMacro(name);
            return;
        }

        // --- Máquina de Estados de Gravação de Macro ---

        // Comando para INICIAR a gravação
        if (input.startsWith(F("startrecord ")))
        {
            if (isRecording)
            {
                Serial.println(F("ERRO: Gravação já em progresso. Use 'saverecord' ou 'cancelrecord'."));
                return;
            }
            char name[POSE_NAME_LEN] = {0};
            sscanf(input.c_str(), "startrecord %s", name);
            if (name[0] == 0)
            {
                Serial.println(F("ERRO: Nome da macro não pode ser vazio."));
                return;
            }

            // Verifica se o nome já existe (para evitar confusão)
            Macro tempMacro;
            if (MacroManager::loadMacroByName(name, tempMacro))
            {
                Serial.println(F("AVISO: Já existe uma macro com este nome. Ela será sobrescrita ao salvar."));
            }

            memset(&recordingMacro, 0, sizeof(Macro)); // Limpa o buffer
            strncpy(recordingMacro.name, name, POSE_NAME_LEN - 1);
            recordingMacro.name[POSE_NAME_LEN - 1] = '\0';
            recordingMacro.numSteps = 0;
            isRecording = true;
            Serial.printf(F("Iniciando gravação da Macro '%s'.\n"), name);
            Serial.println(F("Use 'addstep <pose_name> <delay_ms>' para adicionar passos."));
            return; // Retorna para garantir que estamos no modo de gravação
        }

        // Comandos VÁLIDOS APENAS DURANTE A GRAVAÇÃO
        if (isRecording)
        {
            if (input.startsWith(F("addstep ")))
            {
                if (recordingMacro.numSteps >= MAX_STEPS_PER_MACRO)
                {
                    Serial.println(F("ERRO: Limite de passos da macro atingido."));
                    return;
                }
                char poseName[POSE_NAME_LEN] = {0};
                unsigned long delayMs = 0;
                int params = sscanf(input.c_str(), "addstep %s %lu", poseName, &delayMs);

                if (params < 1 || poseName[0] == 0)
                {
                    Serial.println(F("Formato: addstep <pose_name> [delay_ms]"));
                    return;
                }

                // Verifica se a pose existe antes de adicionar
                // Usamos loadPoseByName com 0 para "não mover", apenas verificar se existe
                if (!PoseManager::loadPoseByName(poseName, 0))
                {
                    Serial.printf(F("ERRO: Pose '%s' não encontrada. Passo não adicionado.\n"), poseName);
                    Serial.println(F("Crie a pose primeiro com 'savepose'."));
                    return;
                }

                // Adiciona o passo
                MacroStep &step = recordingMacro.steps[recordingMacro.numSteps];
                strncpy(step.poseName, poseName, POSE_NAME_LEN - 1);
                step.poseName[POSE_NAME_LEN - 1] = '\0';
                step.delay_ms = delayMs;
                recordingMacro.numSteps++;

                Serial.printf(F(" Passo %d adicionado: Pose '%s', Espera %lu ms.\n"),
                              recordingMacro.numSteps, poseName, delayMs);
            }
            else if (input.equalsIgnoreCase(F("saverecord")))
            {
                if (recordingMacro.numSteps == 0)
                {
                    Serial.println(F("ERRO: Macro está vazia. Adicione passos com 'addstep'."));
                    return;
                }
                if (MacroManager::saveMacro(recordingMacro))
                {
                    Serial.println(F("Gravação concluída e salva."));
                }
                else
                {
                    Serial.println(F("ERRO ao salvar macro."));
                }
                isRecording = false; // Sai do modo de gravação
            }
            else if (input.equalsIgnoreCase(F("cancelrecord")))
            {
                isRecording = false; // Sai do modo de gravação
                Serial.println(F("Gravação de macro cancelada."));
            }
            else
            {
                Serial.println(F("ERRO: Em modo de gravação. Use 'addstep', 'saverecord' ou 'cancelrecord'."));
            }

            return; // Bloqueia outros comandos enquanto grava
        }

        // --- Comandos Gerais (se não estiver gravando) ---
        if (input.equalsIgnoreCase(F("mostrar")))
        {
            Calibration::printStatus();
        }
        else if (input.equalsIgnoreCase(F("listposes")))
        {
            PoseManager::listPoses();
        }
        else if (input.startsWith(F("savepose ")))
        {
            char name[POSE_NAME_LEN] = {0};
            sscanf(input.c_str(), "savepose %s", name);
            PoseManager::savePose(name);
        }
        else if (input.startsWith(F("delpose ")))
        {
            char name[POSE_NAME_LEN] = {0};
            sscanf(input.c_str(), "delpose %s", name);
            PoseManager::deletePose(name);
        }
        else if (input.startsWith(F("loadpose ")))
        {
            char name[POSE_NAME_LEN] = {0};
            unsigned long duration = 0; // 0 será usado como flag para "velocidade padrão"
            int params = sscanf(input.c_str(), "loadpose %s %lu", name, &duration);

            if (params == 1)
            {
                PoseManager::loadPoseByName(name); // Usa velocidade padrão
            }
            else if (params == 2)
            {
                PoseManager::loadPoseByName(name, duration); // Usa duração customizada
            }
            else if (name[0] == 0)
            {
                Serial.println(F("Formato: loadpose <nome> [tempo]"));
            }
        }
        else if (input.startsWith(F("set ")))
        {
            handleSetCommand(input);
        }
        else if (input.startsWith(F("min ")))
        {
            Calibration::setMin(input); 
        }
        else if (input.startsWith(F("max ")))
        {
            Calibration::setMax(input); 
        }
        else if (input.startsWith(F("offset ")))
        {
            Calibration::setOffset(input); 
        }
        else if (input.startsWith(F("align ombro")))
        {
            Calibration::alignShoulders(input); 
        }
        else if (input.equalsIgnoreCase(F("save")))
        {
            Storage::saveToEEPROM(); 
        }
        else if (input.equalsIgnoreCase(F("load")))
        {
            Storage::loadFromEEPROM(true);
        }
        else if (input.equalsIgnoreCase(F("help")))
        {
            printHelp();
        }
        else
        {
            Serial.printf(F("Comando não reconhecido: '%s'\n"), input.c_str());
        }
    }

} // namespace CommandParser
