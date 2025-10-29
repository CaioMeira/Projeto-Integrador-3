# Braço Robótico ESP32

Este projeto implementa o controle e a automação de um braço robótico de **5 graus de liberdade**, utilizando um **ESP32**.  
O foco desta versão foi a **reestruturação modular** e a adoção de uma **arquitetura não-bloqueante baseada em Máquinas de Estado**, garantindo **movimentos suaves (smooth move)** e execução de **rotinas complexas (macros)**.

---

## Arquitetura do Firmware (Modularização)

O código foi dividido em **módulos (namespaces)** com responsabilidades bem definidas, facilitando a leitura, o teste e a expansão.

| **Módulo (Namespace)** | **Responsabilidade Principal** | **Descrição Detalhada** |
|-------------------------|-------------------------------|--------------------------|
| **Config** | Constantes e Estruturas | Define pinos, tamanhos de arrays, constantes de velocidade, endereços de EEPROM e structs de dados (`Pose`, `Macro`, `StoredData`). |
| **MotionController** | Movimento dos Servos (Físico) | Executa o movimento suave (interpolação) dos servos no tempo. Contém variáveis globais de posição, limites e offsets. |
| **Calibration** | Limites e Offsets | Gerencia comandos de `min`, `max`, `offset` e `align` para calibração de software e hardware. |
| **Storage** | Persistência (EEPROM) | Salva e carrega o estado de calibração (min/max/offsets) e a última posição. |
| **PoseManager** | Poses Estáticas | Gerencia criação, listagem, carregamento e exclusão de **Poses** na EEPROM. |
| **MacroManager** | Rotinas Sequenciais | Gerencia criação, listagem, carregamento e exclusão de **Macros** (sequências de poses e tempos). |
| **Sequencer** | Máquina de Estados (Automação) | Executa Macros de forma não-bloqueante, usando `MotionController::isMoving()` para avançar entre passos. |
| **CommandParser** | Interface Serial | Interpreta comandos de texto da Serial (`move`, `set`, `pose save`, `macro play`, etc.) e roteia ao módulo correto. |
| **robotic_arm.ino** | Ponto de Entrada | Inicializa Serial, EEPROM, chama `setup()` dos módulos e mantém o loop principal não-bloqueante. |

---

## Principais Funcionalidades

### 1. Movimento Suave e Não-Bloqueante  
**Módulos:** `MotionController` & `robotic_arm.ino`

- **Interpolação (Smooth Move):**  
  O movimento entre A → B é dividido em pequenos passos.  
  O `MotionController::update()` é chamado continuamente no `loop()` principal, calculando ângulos com a função de easing `EaseInOutQuad`.

- **Não-Bloqueante:**  
  O `loop()` principal continua executando outras tarefas (como ler a Serial e atualizar o Sequencer) pois o controle é baseado em `millis()` e não em `delay()`.

## 1.1 Easing (Interpolação Suave)

Para garantir que o braço **não comece nem pare de forma abrupta** (“engasgos”), utilizamos uma técnica chamada **Easing (abrandamento)**.

Em vez de um movimento **linear** (velocidade constante), o código implementa a equação **EaseInOutQuad**:

$$
\text{easeProgress} =
\begin{cases}
2 \times \text{progress}^2, & \text{se } \text{progress} < 0.5 \\
1 - \dfrac{(-2 \times \text{progress} + 2)^2}{2}, & \text{se } \text{progress} \geq 0.5
\end{cases}
$$

- `progress` → progresso linear (tempo) de `0.0` a `1.0`  
- `easeProgress` → progresso ajustado, também de `0.0` a `1.0`

## Efeito da equação

| Etapa | Descrição |
|-------|------------|
| **Início Suave** | O braço começa a se mover lentamente. |
| **Aceleração Central** | A velocidade aumenta gradualmente no meio do trajeto. |
| **Fim Suave** | A velocidade diminui gradualmente ao se aproximar da posição alvo. |

> Este método de interpolação não apenas torna os movimentos visualmente mais naturais e robóticos, mas também reduz o estresse mecânico e o ruído dos servos.

---

### 2. Automação de Rotinas  
**Módulos:** `Sequencer` & `MacroManager`

- **Máquina de Estados:**  
  Controla a execução de Macros nos estados: `IDLE`, `MOVING`, `WAITING`.

- **Sincronismo com Movimento:**  
  O Sequencer só avança quando `MotionController::isMoving()` retorna `false` (pose atingida).

- **Comandos:** `macro create <nome>`, `macro add <nome> <pose> <delay>`, `macro play <nome>` 

---

### 3. Persistência de Dados  
**Módulos:** `Storage`, `PoseManager`, `MacroManager`

- **EEPROM Estruturada:**  
Armazena calibração, poses e macros com estruturas organizadas (`Pose`, `Macro`, `StoredData`).

- **Comandos:** `save`, `load` (para calibração e última posição), `pose save <nome>`.

---

## Tabela de Melhorias (Código Legado vs. Modular)

| **Funcionalidade** | **Código Legado (Monolítico)** | **Versão Modular v5.0** |
|--------------------|---------------------------------|--------------------------|
| **Arquitetura** | Código único (legacy.ino). Configurações, lógica e parsing misturados. | Modular: Namespaces dedicados (MotionController, Storage, etc.) com alto reuso. |
| **Qualidade do Movimento** | Interpolação linear simples, movimentos bruscos. | Interpolação Suave com `EaseInOutQuad`. |
| **Execução de Macros** | Misturado ao parser, sem Máquina de Estados. | Máquina de Estados Não-Bloqueante (`Sequencer`). |
| **Calibração/Persistência** | Misturada à lógica de parsing. | Módulos dedicados `Calibration` e `Storage`. |
| **Comunicação (Parsing)** | `if/else` extensos no `loop()`. | Delegador de Comandos (`CommandParser`). |
| **Manutenção/Escalabilidade** | Baixa: alterações afetam todo o sistema. | Alta: novos recursos exigem apenas 1–2 módulos. |

---

## Comandos de Serial (Resumo)

| **Categoria** | **Comando** | **Exemplo** | **Descrição** |
|----------------|--------------|--------------|----------------|
| **Movimento** | `move <s0> ... <s6> [tempo]` | `move 90 90 90 90 90 90 90 1000` | Move todos os servos em tempo ms. |
| **Ajuste** | `set <idx> <ang> [tempo]` | `set 3 120 500` | Move o servo 3 para 120° em 500ms. |
| **Poses** | `pose save <nome>` | `pose save HOME` | Salva a posição atual. |
| | `pose load <nome> [tempo]` | `pose load HOME 2000` | Carrega uma pose. |
| **Macros** | `macro create <nome>` | `macro create ROTINA1` | Inicia a criação de uma macro. |
| | `macro add <nome> <pose> <delay>` | `macro add ROTINA1 P1 500` | Adiciona um passo à macro. |
| | `macro play <nome>` | `macro play ROTINA1` | Executa macro de forma não-bloqueante. |
| | `macro stop` | `macro stop` | Interrompe a execução atual. |
| **Calibração** | `offset <idx> <valor>` | `offset 1 -5` | Define offset de calibração. |
| | `min <idx> <ang>` | `min 3 20` | Define o limite mínimo. |
| **Sistema** | `status` | `status` | Exibe posição, limites e offsets. |
| | `save` | `save` | Salva calibração e última posição. |
| | `load` | `load` | Carrega calibração e última posição. |
| | `help` | `help` | Exibe o menu de comandos. |

---

**Resumo:**  
A **versão modular v5.0** traz uma arquitetura limpa, escalável e não-bloqueante, facilitando a adição de novas funcionalidades, a depuração e o controle preciso dos servos.

