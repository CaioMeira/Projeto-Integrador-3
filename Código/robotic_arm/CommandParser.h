/**
 * CommandParser.h
 * Responsável por ler a Serial, interpretar os comandos
 * e chamar as funções apropriadas nos outros módulos.
 */
#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

namespace CommandParser
{

    /**
     * @brief Inicializa o parser (mostra a ajuda).
     */
    void setup();

    /**
     * @brief Verifica a Serial por novos comandos e os processa.
     * Deve ser chamado a cada iteração do loop() principal.
     */
    void handleSerialInput();

} // namespace CommandParser

#endif // COMMAND_PARSER_H
