/**
 * @file Calibration.cpp
 * @brief Implementação do módulo de calibração.
 * Contém a lógica para fazer o parsing dos comandos de calibração.
 */

#include "Calibration.h"
#include "MotionController.h"
#include <Arduino.h>

namespace Calibration
{

    void setMin(const char *input)
    {
        int idx, val;
        if (sscanf(input, "min %d %d", &idx, &val) == 2 && idx >= 0 && idx < NUM_SERVOS)
        {
            minAngles[idx] = constrain(val, 0, 180);
            Serial.print(F("Minimo do servo "));
            Serial.print(idx);
            Serial.print(F(" definido para "));
            Serial.print(val);
            Serial.println(F("°"));
        }
        else
        {
            Serial.println(F("Formato inválido. Use: min <servo> <angulo>"));
        }
    }

    void setMax(const char *input)
    {
        int idx, val;
        if (sscanf(input, "max %d %d", &idx, &val) == 2 && idx >= 0 && idx < NUM_SERVOS)
        {
            maxAngles[idx] = constrain(val, 0, 180);
            Serial.print(F("Maximo do servo "));
            Serial.print(idx);
            Serial.print(F(" definido para "));
            Serial.print(val);
            Serial.println(F("°"));
        }
        else
        {
            Serial.println(F("Formato inválido. Use: max <servo> <angulo>"));
        }
    }

    void setOffset(const char *input)
    {
        int idx, val;
        if (sscanf(input, "offset %d %d", &idx, &val) == 2 && idx >= 0 && idx < NUM_SERVOS)
        {
            offsets[idx] = constrain(val, -90, 90);

            int correctedAngle = constrain(currentAngles[idx] + offsets[idx], 0, 180);
            servos[idx].write(correctedAngle);
            Serial.print(F("Offset do servo "));
            Serial.print(idx);
            Serial.print(F(" ajustado para "));
            if (val >= 0)
                Serial.print(F("+"));
            Serial.print(val);
            Serial.println(F("°"));
        }
        else
        {
            Serial.println(F("Formato inválido. Use: offset <servo> <valor>"));
        }
    }

    void alignShoulders(const char *input)
    {
        unsigned long duration = 0;
        sscanf(input, "align ombro %lu", &duration);

        int media = (currentAngles[1] + currentAngles[2]) / 2;

        int tempTarget[NUM_SERVOS];
        for (int i = 0; i < NUM_SERVOS; i++)
            tempTarget[i] = currentAngles[i];
        tempTarget[1] = media;
        tempTarget[2] = media;

        if (duration == 0)
        {
            duration = MotionController::calculateDurationBySpeed(tempTarget);
            Serial.print(F("Alinhando ombros para "));
            Serial.print(media);
            Serial.print(F("° (duracao calc: "));
            Serial.print(duration);
            Serial.println(F(" ms)..."));
        }
        else
        {
            Serial.print(F("Alinhando ombros para "));
            Serial.print(media);
            Serial.print(F("° (duracao: "));
            Serial.print(duration);
            Serial.println(F(" ms)..."));
        }

        MotionController::startSmoothMove(tempTarget, duration);
    }

    void printStatus()
    {
        Serial.println(F("\n--- Status Atual dos Servos ---"));
        for (int i = 0; i < NUM_SERVOS; i++)
        {
            Serial.print(F("Servo "));
            Serial.print(i);
            Serial.print(F(" | Logico:"));
            Serial.print(currentAngles[i]);
            Serial.print(F("° | Min:"));
            Serial.print(minAngles[i]);
            Serial.print(F("° | Max:"));
            Serial.print(maxAngles[i]);
            Serial.print(F("° | Offset:"));
            if (offsets[i] >= 0)
                Serial.print(F("+"));
            Serial.print(offsets[i]);
            Serial.print(F("° | Fisico(out):"));
            Serial.print(constrain(currentAngles[i] + offsets[i], 0, 180));
            Serial.println(F("°"));
        }
    }

} // namespace Calibration
