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
    
    // --- Buffer Estático para Comandos (Evita fragmentação de heap) ---
    static char cmdBuffer[64];
    static uint8_t bufIdx = 0;

    /**
     * @brief Função auxiliar interna para tratar comandos 'set'.
     */
    void handleSetCommand(const char* input)
    {
        int tempTarget[NUM_SERVOS];
        for (int i = 0; i < NUM_SERVOS; i++)
            tempTarget[i] = currentAngles[i];

        unsigned long duration = 0; // 0 = gatilho para cálculo automático
        int angle = 0;
        int servo_idx = -1;

        if (strncmp(input, "set ombro ", 10) == 0)
        {
            int params = sscanf(input, "set ombro %d %lu", &angle, &duration);
            if (params >= 1)
            {
                tempTarget[1] = angle;
                tempTarget[2] = angle;
                Serial.print(F("Ajustando ombros para "));
                Serial.print(angle);
                Serial.print(F("° (duracao: "));
                Serial.print(duration);
                Serial.println(F(" ms)..."));
            }
            else
            {
                Serial.println(F("Formato inválido. Use: set ombro <angulo> [tempo]"));
                return;
            }
        }
        else if (strncmp(input, "set ", 4) == 0)
        {
            int params = sscanf(input, "set %d %d %lu", &servo_idx, &angle, &duration);
            if (params >= 2 && servo_idx >= 0 && servo_idx < NUM_SERVOS)
            {
                tempTarget[servo_idx] = angle;
                Serial.print(F("Ajustando servo "));
                Serial.print(servo_idx);
                Serial.print(F(" para "));
                Serial.print(angle);
                Serial.print(F("° (duracao: "));
                Serial.print(duration);
                Serial.println(F(" ms)..."));
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
            Serial.print(F("Duracao nao fornecida. Calculando duracao automatica: "));
            Serial.print(duration);
            Serial.println(F(" ms."));
        }

        // Inicia o movimento suave
        MotionController::startSmoothMove(tempTarget, duration);
    }
    
    /**
     * @brief Função auxiliar interna para tratar comandos 'move'.
     * Move todos os 7 servos de uma só vez.
     */
    void handleMoveCommand(const char* input)
    {
        int targetAngles[NUM_SERVOS];
        unsigned long duration = 0;
        int paramsFound = 0;

        // Formato: move S0 S1 S2 S3 S4 S5 S6 [tempo]
        paramsFound = sscanf(
            input, 
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
                Serial.print(F("Duracao nao fornecida. Calculando duracao automatica: "));
                Serial.print(duration);
                Serial.println(F(" ms."));
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
        Serial.println(F("  macro delete <nome> [ou all]    -> Apaga uma macro ou todas."));
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
            Serial.print(F("Gravacao ATIVA: Macro '"));
        Serial.print(recordingMacro.name);
        Serial.print(F("'. Proximo Passo: "));
        Serial.print(recordingMacro.numSteps + 1);
        Serial.print(F("/"));
        Serial.println(MAX_STEPS_PER_MACRO);
        } else {
            Serial.println(F("Gravação INATIVA. Use 'macro create <nome>' para começar."));
        }
    }

    void setup() {
        printHelp();
    }

    void handleSerialInput()
    {
        // Lê caracteres da Serial de forma não-bloqueante
        while (Serial.available())
        {
            char c = Serial.read();
            
            if (c == '\n' || c == '\r') {
                if (bufIdx > 0) {
                    cmdBuffer[bufIdx] = '\0';
                    processCommand(cmdBuffer);
                    bufIdx = 0;
                }
            }
            else if (c == ' ' && bufIdx == 0) {
                // Ignora espaços no início
            }
            else if (bufIdx < sizeof(cmdBuffer) - 1) {
                // Converte para lowercase inline
                cmdBuffer[bufIdx++] = (c >= 'A' && c <= 'Z') ? c + 32 : c;
            }
            else {
                // Buffer cheio, descarta comando
                Serial.println(F("ERR: Comando muito longo"));
                bufIdx = 0;
            }
        }
    }
    
    void processCommand(const char* cmd)
    {
        if (cmd[0] == '\0') return;
        
        // --- Lógica de Gravação de Macro (Precedência Alta) ---
        if (isRecording)
        {
            if (strncmp(cmd, "macro add ", 10) == 0)
            {
                char poseName[POSE_NAME_LEN] = {0};
                unsigned long delay_ms = 0;
                if (sscanf(cmd, "macro add %s %lu", poseName, &delay_ms) == 2)
                {
                    if (recordingMacro.numSteps < MAX_STEPS_PER_MACRO)
                    {
                        strncpy(recordingMacro.steps[recordingMacro.numSteps].poseName, poseName, POSE_NAME_LEN - 1);
                        recordingMacro.steps[recordingMacro.numSteps].delay_ms = delay_ms;
                        recordingMacro.numSteps++;
                        Serial.print(F("Passo adicionado: Pose '"));
                        Serial.print(poseName);
                        Serial.print(F("', Delay "));
                        Serial.print(delay_ms);
                        Serial.print(F(" ms. Total: "));
                        Serial.print(recordingMacro.numSteps);
                        Serial.print(F("/"));
                        Serial.println(MAX_STEPS_PER_MACRO);
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
            else if (strcmp(cmd, "macro save") == 0)
            {
                if (MacroManager::saveMacro(recordingMacro))
                {
                    isRecording = false; // Gravação concluída e salva
                }
            }
            else
            {
                Serial.println(F("AVISO: No modo gravacao, os comandos sao limitados a 'macro add' e 'macro save'."));
            }
            return;
        }

        // --- Lógica de Comandos Normais ---

        // ** Movimento **
        if (strncmp(cmd, "move ", 5) == 0)
        {
            handleMoveCommand(cmd);
        }
        // ** Macros **
        else if (strncmp(cmd, "macro create ", 13) == 0)
        {
            char name[POSE_NAME_LEN];
            if (sscanf(cmd, "macro create %s", name) == 1)
            {
                if (!MacroManager::loadMacroByName(name, recordingMacro))
                {
                    Macro emptyMacro = {0};
                    recordingMacro = emptyMacro;
                    strncpy(recordingMacro.name, name, POSE_NAME_LEN - 1);
                    recordingMacro.name[POSE_NAME_LEN - 1] = 0;
                }
                
                isRecording = true;
                Serial.print(F("MODO GRAVACAO ATIVADO para macro '"));
                Serial.print(recordingMacro.name);
                Serial.println(F("'."));
                Serial.println(F("Use 'macro add <pose> <delay_ms>' e 'macro save' para finalizar."));
                printHelp();
            }
            else
            {
                Serial.println(F("Formato: macro create <nome>"));
            }
        }
        else if (strncmp(cmd, "macro play ", 11) == 0)
        {
            char name[POSE_NAME_LEN];
            if (sscanf(cmd, "macro play %s", name) == 1)
            {
                Sequencer::startMacro(name);
            }
            else
            {
                Serial.println(F("Formato: macro play <nome>"));
            }
        }
        else if (strcmp(cmd, "macro stop") == 0)
        {
            Sequencer::stopMacro();
        }
        else if (strcmp(cmd, "macro list") == 0)
        {
            MacroManager::listMacros();
        }
        else if (strncmp(cmd, "macro delete ", 13) == 0)
        {
            char name[POSE_NAME_LEN];
            if (sscanf(cmd, "macro delete %s", name) == 1)
            {
                MacroManager::deleteMacro(name);
            }
        }
        // ** Poses **
        else if (strncmp(cmd, "pose save ", 10) == 0)
        {
            char name[POSE_NAME_LEN];
            if (sscanf(cmd, "pose save %s", name) == 1)
            {
                PoseManager::savePose(name);
                Storage::saveToEEPROM();
            }
            else
            {
                Serial.println(F("Formato: pose save <nome>"));
            }
        }
        else if (strcmp(cmd, "pose list") == 0)
        {
            PoseManager::listPoses();
        }
        else if (strncmp(cmd, "pose delete ", 12) == 0)
        {
            char name[POSE_NAME_LEN];
            if (sscanf(cmd, "pose delete %s", name) == 1)
            {
                PoseManager::deletePose(name);
            }
        }
        else if (strncmp(cmd, "pose load ", 10) == 0)
        {
            char name[POSE_NAME_LEN];
            unsigned long duration = 0;
            int params = sscanf(cmd, "pose load %s %lu", name, &duration);

            if (params == 1)
            {
                PoseManager::loadPoseByName(name);
            }
            else if (params == 2)
            {
                PoseManager::loadPoseByName(name, duration);
            }
            else
            {
                Serial.println(F("Formato: pose load <nome> [tempo]"));
            }
        }
        // ** Ajuste (Set) **
        else if (strncmp(cmd, "set ", 4) == 0)
        {
            handleSetCommand(cmd);
        }
        // ** Calibração **
        else if (strncmp(cmd, "min ", 4) == 0)
        {
            Calibration::setMin(cmd); 
        }
        else if (strncmp(cmd, "max ", 4) == 0)
        {
            Calibration::setMax(cmd); 
        }
        else if (strncmp(cmd, "offset ", 7) == 0)
        {
            Calibration::setOffset(cmd); 
        }
        else if (strncmp(cmd, "align ombro", 11) == 0)
        {
            Calibration::alignShoulders(cmd); 
        }
        // ** Sistema **
        else if (strcmp(cmd, "save") == 0)
        {
            Storage::saveToEEPROM(); 
        }
        else if (strcmp(cmd, "load") == 0)
        {
            Storage::loadFromEEPROM(true);
        }
        else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "h") == 0)
        {
            printHelp();
        }
        else if (strcmp(cmd, "status") == 0)
        {
            Calibration::printStatus();
        }
        else
        {
            Serial.print(F("Comando desconhecido: "));
            Serial.print(cmd);
            Serial.println(F(". Use 'help' para ver a lista de comandos."));
        }
    }

} // namespace CommandParser

