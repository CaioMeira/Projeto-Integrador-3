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

            Serial.printf(F("Iniciando Macro '%s' (%d passos)...\n"), runningMacro.name, runningMacro.numSteps);
            currentStep = 0;

            // Inicia o primeiro passo
            Serial.printf(F("  Passo 1: Carregando pose '%s'...\n"), runningMacro.steps[0].poseName);
            if (PoseManager::loadPoseByName(runningMacro.steps[0].poseName))
            {
                currentState = MOVING;
            }
            else
            {
                Serial.printf(F("ERRO: Pose '%s' não encontrada. Abortando macro.\n"), runningMacro.steps[0].poseName);
                currentState = IDLE;
            }
        }
        else
        {
            Serial.printf(F("ERRO: Macro '%s' não encontrada.\n"), name);
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
                Serial.printf(F("  Passo %d concluído. Aguardando %lu ms...\n"), currentStep + 1, delay);

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
                    Serial.printf(F("Macro '%s' concluída.\n"), runningMacro.name);
                    currentState = IDLE;
                }
                else
                {
                    // Inicia o próximo passo
                    Serial.printf(F("  Passo %d: Carregando pose '%s'...\n"), currentStep + 1, runningMacro.steps[currentStep].poseName);
                    if (PoseManager::loadPoseByName(runningMacro.steps[currentStep].poseName))
                    {
                        currentState = MOVING;
                    }
                    else
                    {
                        Serial.printf(F("ERRO: Pose '%s' não encontrada. Abortando macro.\n"), runningMacro.steps[currentStep].poseName);
                        currentState = IDLE;
                    }
                }
            }
            break;
        }
    }

} // namespace Sequencer
