/**
 * @file Storage.cpp
 * @brief Implementação da lógica de persistência (EEPROM).
 */
#include "Storage.h"
#include "MotionController.h" // Para iniciar o movimento ao carregar

// As variáveis globais (currentAngles, minAngles, etc.) são definidas em MotionController.cpp
// e declaradas 'extern' em Config.h, portanto, estão disponíveis aqui.

namespace Storage {

/**
 * @brief Migra dados V1 para V2 (compatibilidade com versão anterior).
 */
bool migrateToV2() {
    StoredData oldData;
    EEPROM.get(0, oldData);
    
    if (oldData.magic != EEPROM_MAGIC) {
        return false;  // Sem dados para migrar
    }
    
    Serial.println(F("Migrando EEPROM V1 -> V2..."));
    
    // Converte para novo formato
    StoredDataV2 newData;
    newData.magic = EEPROM_MAGIC;
    newData.version = 2;
    
    for (int i = 0; i < NUM_SERVOS; i++) {
        newData.current[i] = (uint8_t)constrain(oldData.current[i], 0, 180);
        newData.minv[i] = (uint8_t)constrain(oldData.minv[i], 0, 180);
        newData.maxv[i] = (uint8_t)constrain(oldData.maxv[i], 0, 180);
        newData.offs[i] = (int8_t)constrain(oldData.offs[i], -127, 127);
    }
    
    // Calcula CRC
    newData.crc16 = calcCRC16((uint8_t*)&newData, sizeof(newData) - 2);
    
    // Salva novo formato
    EEPROM.put(0, newData);
    EEPROM.commit();
    
    Serial.println(F("Migracao completa!"));
    return true;
}

void saveToEEPROM() {
    StoredDataV2 sd;
    sd.magic = EEPROM_MAGIC;
    sd.version = 2;
    
    for (int i = 0; i < NUM_SERVOS; i++) {
        sd.current[i] = (uint8_t)constrain(currentAngles[i], 0, 180);
        sd.minv[i] = (uint8_t)constrain(minAngles[i], 0, 180);
        sd.maxv[i] = (uint8_t)constrain(maxAngles[i], 0, 180);
        sd.offs[i] = (int8_t)constrain(offsets[i], -127, 127);
    }
    
    // Calcula CRC (exclui o próprio campo CRC)
    sd.crc16 = calcCRC16((uint8_t*)&sd, sizeof(sd) - 2);
    
    EEPROM.put(0, sd);
    if (EEPROM.commit()) {
        Serial.println(F("EEPROM V2 salva com sucesso (35B)"));
    } else {
        Serial.println(F("ERRO ao salvar EEPROM!"));
    }
}

bool loadFromEEPROM(bool move) {
    StoredDataV2 sd;
    EEPROM.get(0, sd);
    
    // Validação 1: Magic number
    if (sd.magic != EEPROM_MAGIC) {
        Serial.println(F("EEPROM vazia ou corrompida"));
        return false;
    }
    
    // Validação 2: Versão
    if (sd.version == 1 || sd.version == 0) {
        // Dados V1 detectados, tentar migrar
        Serial.println(F("Versao V1 detectada"));
        if (migrateToV2()) {
            return loadFromEEPROM(move);  // Tenta novamente após migração
        }
        return false;
    }
    
    if (sd.version != 2) {
        Serial.print(F("Versao desconhecida: "));
        Serial.println(sd.version);
        return false;
    }
    
    // Validação 3: CRC
    uint16_t calculatedCRC = calcCRC16((uint8_t*)&sd, sizeof(sd) - 2);
    if (calculatedCRC != sd.crc16) {
        Serial.println(F("ERRO: CRC invalido! Dados corrompidos!"));
        Serial.print(F("Calculado: 0x"));
        Serial.print(calculatedCRC, HEX);
        Serial.print(F(" | Salvo: 0x"));
        Serial.println(sd.crc16, HEX);
        return false;
    }
    
    // Dados válidos, carrega
    int initialAngles[NUM_SERVOS];
    for (int i = 0; i < NUM_SERVOS; i++) {
        minAngles[i] = constrain(sd.minv[i], 0, 180);
        maxAngles[i] = constrain(sd.maxv[i], minAngles[i], 180);
        offsets[i] = constrain(sd.offs[i], -127, 127);
        initialAngles[i] = constrain(sd.current[i], minAngles[i], maxAngles[i]);
    }

    if (move) {
        Serial.println(F("EEPROM V2 carregada. Movendo..."));
        unsigned long autoDuration = MotionController::calculateDurationBySpeed(initialAngles);
        MotionController::startSmoothMove(initialAngles, autoDuration);
    } else {
        for (int i = 0; i < NUM_SERVOS; i++) {
            currentAngles[i] = initialAngles[i];
        }
        Serial.println(F("EEPROM V2 carregada (sem movimento)."));
    }
    return true;
}

} // namespace Storage
