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
                Serial.printf(F("Ajustando ombros para %d° (duração: %lu ms)...\\n"), angle, duration);
            }
            else
            {
                Serial.println(F("Formato inválido. Use: set ombro <angulo> [tempo]"));
                return;
            }
        }
        else if (input.startsWith(F("set ")))
        {
            int params = sscanf(input.c_str(), "set %d %d %lu", &servo_idx, &angle, &duration);
            if (params >= 2 && servo_idx >= 0 && servo_idx < NUM_SERVOS)
            {
                tempTarget[servo_idx] = angle;
                Serial.printf(F("Ajustando servo %d para %d° (duração: %lu ms)...\\n"), servo_idx, angle, duration);
            }
            else
            {
                Serial.println(F("Formato inválido. Use: set <servo> <angulo> [tempo]"));
                return;
            }
        } else {
            return;
        }

        // 1. Garante que os ângulos alvos estejam dentro dos limites de software (min/max)
        for (int i = 0; i < NUM_SERVOS; i++) 
        {
            tempTarget[i] = constrain(tempTarget[i], minAngles[i], maxAngles[i]);
        }

        // 2. Se a duração não foi fornecida (duration == 0), calcula a duração padrão
        if (duration == 0) 
        {
            duration = MotionController::calculateDurationBySpeed(tempTarget);
            Serial.printf(F("Duração não fornecida. Calculando duração automática: %lu ms.\\n"), duration);
        }

        // Inicia o movimento suave
        MotionController::startSmoothMove(tempTarget, duration);
    }
    
    /**
     * @brief Função auxiliar interna para tratar comandos 'move'.
     * Move todos os 7 servos de uma só vez.
     */
    void handleMoveCommand(String input)
    {
        int targetAngles[NUM_SERVOS];
        unsigned long duration = 0;
        int paramsFound = 0;

        // Formato: move S0 S1 S2 S3 S4 S5 S6 [tempo]
        paramsFound = sscanf(
            input.c_str(), 
            "move %d %d %d %d %d %d %d %lu", 
            &targetAngles[0], &targetAngles[1], &targetAngles[2], &targetAngles[3], 
            &targetAngles[4], &targetAngles[5], &targetAngles[6], &duration
        );

        // O comando 'move' deve ter exatamente NUM_SERVOS (7) parâmetros de ângulo
        if (paramsFound >= NUM_SERVOS) 
        {
            // 1. Garante que os ângulos alvos estejam dentro dos limites de software (min/max)
            for (int i = 0; i < NUM_SERVOS; i++) 
            {
                targetAngles[i] = constrain(targetAngles[i], minAngles[i], maxAngles[i]);
            }

            // 2. Se a duração não foi fornecida (paramsFound == NUM_SERVOS), calcula a duração padrão
            if (paramsFound == NUM_SERVOS) 
            {
                duration = MotionController::calculateDurationBySpeed(targetAngles);
                Serial.printf(F("Duração não fornecida. Calculando duração automática: %lu ms.\\n"), duration);
            }
            
            // 3. Inicia o movimento suave
            MotionController::startSmoothMove(targetAngles, duration);
        } 
        else 
        {
            Serial.println(F("Formato inválido. Use: move <s0> <s1> <s2> <s3> <s4> <s5> <s6> [tempo]"));
        }
    }


    /**
     * @brief Exibe o menu de ajuda na Serial.
     */
    void printHelp()
    {
        Serial.println(F("\n--- Comandos Serial ---"));
        Serial.println(F(":-----------------------------------------Comandos de Movimento-----------------------------------------"));
        Serial.println(F("  move <s0> <s1> ... <s6> [tempo] -> Move todos os servos (tempo opcional, auto-calculado)."));
        Serial.println(F("  set <idx> <ang> [tempo]         -> Move um servo específico."));
        Serial.println(F("  set ombro <ang> [tempo]         -> Move os servos 1 e 2 juntos."));
        Serial.println(F("-----------------------------------------Comandos de Poses (Pontos Fixos):-----------------------------------------"));
        Serial.println(F("  pose save <nome>                -> Salva a posição atual (ex: HOME)."));
        Serial.println(F("  pose load <nome> [tempo]        -> Carrega a pose e move (tempo opcional)."));
        Serial.println(F("  pose delete <nome> [ou all]     -> Apaga uma pose ou todas."));
        Serial.println(F("  pose list                       -> Lista todas as poses salvas."));
        Serial.println(F("-----------------------------------------Comandos de Macros (Rotinas):-----------------------------------------"));
        Serial.println(F("  macro create <nome>             -> Inicia a gravação de uma nova macro."));
        Serial.println(F("  macro add <pose> <delay>        -> Adiciona a pose e o tempo de espera (só durante a gravação)."));
        Serial.println(F("  macro save                      -> Salva a macro em gravação."));
        Serial.println(F("  macro list                      -> Lista todas as macros salvas."));
        Serial.println(F("  macro play <nome>               -> Executa a macro de forma não-bloqueante."));
        Serial.println(F("  macro stop                      -> Interrompe a execução atual da macro."));
        Serial.println(F("-----------------------------------------Comandos de Calibração/Sistema:-----------------------------------------"));
        Serial.println(F("  offset <idx> <valor>            -> Ajusta o offset de calibração do servo (+/-)."));
        Serial.println(F("  min <idx> <ang>                 -> Define o limite mínimo de software."));
        Serial.println(F("  max <idx> <ang>                 -> Define o limite máximo de software."));
        Serial.println(F("  align ombro [tempo]             -> Alinha servos 1 e 2 pela média."));
        Serial.println(F("  status                          -> Exibe posições, limites e offsets atuais."));
        Serial.println(F("  save                            -> Salva calibração e última posição na EEPROM."));
        Serial.println(F("  load                            -> Carrega calibração e move para a última posição."));
        Serial.println(F("  help                            -> Exibe este menu."));
        Serial.println(F("\n--- Modo Gravação ---"));
        if (isRecording) {
            Serial.printf(F("Gravação ATIVA: Macro '%s'. Próximo Passo: %d/%d\\n"), recordingMacro.name, recordingMacro.numSteps + 1, MAX_STEPS_PER_MACRO);
        } else {
            Serial.println(F("Gravação INATIVA. Use 'macro create <nome>' para começar."));
        }
    }

    void setup() {
        printHelp();
    }

    void handleSerialInput()
    {
        // 1. Verifica se há dados disponíveis na Serial
        if (Serial.available())
        {
            String input = Serial.readStringUntil('\n');
            input.trim(); // Remove espaços em branco antes e depois
            input.toLowerCase();

            if (input.length() == 0)
                return;

            // --- Lógica de Gravação de Macro (Precedência Alta) ---
            if (isRecording)
            {
                if (input.startsWith(F("macro add ")))
                {
                    char poseName[POSE_NAME_LEN] = {0};
                    unsigned long delay_ms = 0;
                    if (sscanf(input.c_str(), "macro add %s %lu", poseName, &delay_ms) == 2)
                    {
                        if (recordingMacro.numSteps < MAX_STEPS_PER_MACRO)
                        {
                            strncpy(recordingMacro.steps[recordingMacro.numSteps].poseName, poseName, POSE_NAME_LEN - 1);
                            recordingMacro.steps[recordingMacro.numSteps].delay_ms = delay_ms;
                            recordingMacro.numSteps++;
                            Serial.printf(F("Passo adicionado: Pose '%s', Delay %lu ms. Total: %d/%d.\\n"), poseName, delay_ms, recordingMacro.numSteps, MAX_STEPS_PER_MACRO);
                        }
                        else
                        {
                            Serial.println(F("AVISO: Limite de passos atingido. Salve a macro."));
                        }
                    }
                    else
                    {
                        Serial.println(F("Formato inválido. Use: macro add <pose> <delay_ms>"));
                    }
                }
                else if (input.equalsIgnoreCase(F("macro save")))
                {
                    if (MacroManager::saveMacro(recordingMacro))
                    {
                        isRecording = false; // Gravação concluída e salva
                    } // O MacroManager::saveMacro já imprime erro se não conseguir salvar
                }
                else
                {
                    Serial.println(F("AVISO: No modo gravação, os comandos são limitados a 'macro add' e 'macro save'."));
                }
                return; // Sai do loop principal de parsing
            }

            // --- Lógica de Comandos Normais ---

            // ** Movimento **
            if (input.startsWith(F("move ")))
            {
                handleMoveCommand(input);
            }
            // ** Macros **
            else if (input.startsWith(F("macro create ")))
            {
                char name[POSE_NAME_LEN];
                if (sscanf(input.c_str(), "macro create %s", name) == 1)
                {
                    if (!MacroManager::loadMacroByName(name, recordingMacro)) // Tenta carregar uma macro existente para sobrescrever
                    {
                        // Se não existe, inicia nova gravação
                        Macro emptyMacro = {0};
                        recordingMacro = emptyMacro;
                        strncpy(recordingMacro.name, name, POSE_NAME_LEN - 1);
                        recordingMacro.name[POSE_NAME_LEN - 1] = 0;
                    }
                    
                    isRecording = true;
                    Serial.printf(F("MODO GRAVAÇÃO ATIVADO para macro '%s'.\\n"), recordingMacro.name);
                    Serial.println(F("Use 'macro add <pose> <delay_ms>' e 'macro save' para finalizar."));
                    printHelp();
                }
                else
                {
                    Serial.println(F("Formato: macro create <nome>"));
                }
            }
            else if (input.startsWith(F("macro play ")))
            {
                char name[POSE_NAME_LEN];
                if (sscanf(input.c_str(), "macro play %s", name) == 1)
                {
                    Sequencer::startMacro(name);
                }
                else
                {
                    Serial.println(F("Formato: macro play <nome>"));
                }
            }
            else if (input.equalsIgnoreCase(F("macro stop")))
            {
                Sequencer::stopMacro();
            }
            else if (input.equalsIgnoreCase(F("macro list")))
            {
                MacroManager::listMacros();
            }
            else if (input.startsWith(F("macro delete ")))
            {
                char name[POSE_NAME_LEN];
                if (sscanf(input.c_str(), "macro delete %s", name) == 1)
                {
                    MacroManager::deleteMacro(name);
                }
            }
            // ** Poses **
            else if (input.startsWith(F("pose save ")))
            {
                char name[POSE_NAME_LEN];
                if (sscanf(input.c_str(), "pose save %s", name) == 1)
                {
                    PoseManager::savePose(name);
                    Storage::saveToEEPROM(); // Salva a posição atual também
                }
                else
                {
                    Serial.println(F("Formato: pose save <nome>"));
                }
            }
            else if (input.equalsIgnoreCase(F("pose list")))
            {
                PoseManager::listPoses();
            }
            else if (input.startsWith(F("pose delete ")))
            {
                char name[POSE_NAME_LEN];
                if (sscanf(input.c_str(), "pose delete %s", name) == 1)
                {
                    PoseManager::deletePose(name);
                }
            }
            else if (input.startsWith(F("pose load ")))
            {
                char name[POSE_NAME_LEN];
                unsigned long duration = 0;
                int params = sscanf(input.c_str(), "pose load %s %lu", name, &duration);

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
                    Serial.println(F("Formato: pose load <nome> [tempo]"));
                }
            }
            // ** Ajuste (Set) **
            else if (input.startsWith(F("set ")))
            {
                handleSetCommand(input);
            }
            // ** Calibração **
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
            // ** Sistema **
            else if (input.equalsIgnoreCase(F("save")))
            {
                Storage::saveToEEPROM(); 
            }
            else if (input.equalsIgnoreCase(F("load")))
            {
                Storage::loadFromEEPROM(true);
            }
            else if (input.equalsIgnoreCase(F("help")) || input.equalsIgnoreCase(F("h")))
            {
                printHelp();
            }
            else if (input.equalsIgnoreCase(F("status")))
            {
                Calibration::printStatus();
            }
            else
            {
                Serial.printf(F("Comando desconhecido: %s. Use 'help' para ver a lista de comandos.\\n"), input.c_str());
            }
        }
    }

} // namespace CommandParser
