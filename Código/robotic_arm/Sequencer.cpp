/**
 * Sequencer.cpp
 * Implementação da máquina de estados (FSM) do sequenciador.
 */
#include "Sequencer.h"
#include "Config.h"
#include "MacroManager.h"
#include "PoseManager.h"
#include "MotionController.h"

namespace Sequencer
{

    // --- Estados Internos da Máquina ---
    enum State
    {
        IDLE,
        MOVING,
        WAITING
    };

    static State currentState = IDLE;
    static Macro runningMacro;
    static int currentStep = 0;
    static unsigned long waitStartTime = 0;

    bool isRunning()
    {
        return currentState != IDLE;
    }

    void startMacro(const char *name)
    {
        if (isRunning())
        {
            Serial.println(F("ERRO: Sequenciador já está em execução."));
            return;
        }

        if (MacroManager::loadMacroByName(name, runningMacro))
        {
            if (runningMacro.numSteps == 0)
            {
                Serial.println(F("ERRO: Macro está vazia."));
                return;
            }

            Serial.print(F("Iniciando Macro '"));
            Serial.print(runningMacro.name);
            Serial.print(F("' ("));
            Serial.print(runningMacro.numSteps);
            Serial.println(F(" passos)..."));
            currentStep = 0;

            // Inicia o primeiro passo
            Serial.print(F("  Passo 1: Carregando pose '"));
            Serial.print(runningMacro.steps[0].poseName);
            Serial.println(F("'..."));
            if (PoseManager::loadPoseByName(runningMacro.steps[0].poseName))
            {
                currentState = MOVING;
            }
            else
            {
                Serial.print(F("ERRO: Pose '"));
                Serial.print(runningMacro.steps[0].poseName);
                Serial.println(F("' nao encontrada. Abortando macro."));
                currentState = IDLE;
            }
        }
        else
        {
            Serial.print(F("ERRO: Macro '"));
            Serial.print(name);
            Serial.println(F("' nao encontrada."));
        }
    }

    void stopMacro()
    {
        if (isRunning())
        {
            currentState = IDLE;
            // O MotionController continua o movimento atual, mas a sequência para.
            // Para parar o movimento físico, teríamos que chamar MotionController::stopMove() (não implementado)
            Serial.println(F("Macro interrompida."));
        }
    }

    void update()
    {
        if (!isRunning())
        {
            return;
        }

        unsigned long now = millis();

        switch (currentState)
        {
        case IDLE:
            // Não faz nada se estiver ocioso
            break;

        case MOVING:
            // Espera o MotionController terminar o movimento atual
            if (!MotionController::isMoving())
            {
                unsigned long delay = runningMacro.steps[currentStep].delay_ms;
                Serial.print(F("  Passo "));
                Serial.print(currentStep + 1);
                Serial.print(F(" concluido. Aguardando "));
                Serial.print(delay);
                Serial.println(F(" ms..."));

                if (delay > 0)
                {
                    waitStartTime = now;
                    currentState = WAITING;
                }
                else
                {
                    // Se não há espera, avança para o próximo passo imediatamente
                    currentState = WAITING; // Reutiliza a lógica do WAITING com delay 0
                    waitStartTime = now;
                }
            }
            break;

        case WAITING:
            // Espera o tempo de delay_ms do passo atual
            unsigned long delayNeeded = runningMacro.steps[currentStep].delay_ms;
            if (now - waitStartTime >= delayNeeded)
            {
                // Tempo de espera concluído, avança para o próximo passo
                currentStep++;

                if (currentStep >= runningMacro.numSteps)
                {
                    // Macro concluída
                    Serial.print(F("Macro '"));
                    Serial.print(runningMacro.name);
                    Serial.println(F("' concluida."));
                    currentState = IDLE;
                }
                else
                {
                    // Inicia o próximo passo
                    Serial.print(F("  Passo "));
                    Serial.print(currentStep + 1);
                    Serial.print(F(": Carregando pose '"));
                    Serial.print(runningMacro.steps[currentStep].poseName);
                    Serial.println(F("'..."));
                    if (PoseManager::loadPoseByName(runningMacro.steps[currentStep].poseName))
                    {
                        currentState = MOVING;
                    }
                    else
                    {
                        Serial.print(F("ERRO: Pose '"));
                        Serial.print(runningMacro.steps[currentStep].poseName);
                        Serial.println(F("' nao encontrada. Abortando macro."));
                        currentState = IDLE;
                    }
                }
            }
            break;
        }
    }

} // namespace Sequencer
