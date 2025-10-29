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

## 2 Módulos de Persistência (EEPROM/Flash)

Para manter o estado do braço robótico, incluindo calibrações, a última posição e as rotinas programadas, o sistema utiliza a memória não-volátil EEPROM (ou a emulação de EEPROM em Flash no caso do ESP32). A memória é organizada em três seções lógicas:

---

### 2.1. Módulo Storage (Configurações do Sistema)

Este é o módulo base (`Storage.cpp`), responsável por salvar e carregar os dados essenciais para o funcionamento inicial do braço.

**O que Salva:**
- A última posição lógica conhecida de cada servo (`currentAngles`).
- Os limites de software (`minAngles`, `maxAngles`) definidos para segurança.
- Os offsets de calibração (`offsets`) para corrigir desvios mecânicos dos servos.

**Funcionalidade:**  
O módulo utiliza uma chave mágica (`EEPROM_MAGIC`) para garantir que os dados lidos sejam válidos.  
Ao carregar o estado, ele pode opcionalmente iniciar um movimento suave para a última posição salva (como é feito no `setup()` principal).

**Comandos:** `save` e `load`.

---

### 2.2. Módulo PoseManager (Poses Estáticas)

O `PoseManager` (`PoseManager.cpp`) permite que o usuário defina e armazene posições-chave (poses) na EEPROM para serem reutilizadas.

**Estrutura:**  
As poses são armazenadas em slots de memória fixos (definidos por `MAX_POSES`, atualmente 10), cada uma contendo um nome curto e os ângulos de todos os servos.

**Integração com Movimento:**  
O comando de carregamento de pose (`loadPoseByName`) é diretamente integrado ao `MotionController`, iniciando um movimento suave com a interpolação **EaseInOutQuad** (explicada acima) na duração especificada ou calculada.

**Comandos:**  
`savepose <nome>`, `loadpose <nome> [tempo]`, `listpose` e `delpose <nome>`.

---

### 2.3. Módulo MacroManager e Sequencer (Rotinas Programadas)

Estes módulos trabalham em conjunto para permitir a criação e execução de sequências complexas de movimento.

**MacroManager:**  
Responsável pela persistência e gestão de **Macros** (`MacroManager.cpp`), que são listas de passos.  
Cada passo (`MacroStep`) armazena o nome de uma Pose e um tempo de espera (`delay_ms`).

**Sequencer:**  
É a **Máquina de Estados (FSM)** que executa a macro de forma não-bloqueante (`Sequencer.cpp`).  
O seu `update()` alterna entre os estados:

- **MOVING:** Enquanto o `MotionController` interpola para a próxima pose.  
- **WAITING:** Esperando o `delay_ms` do passo atual antes de avançar.  
- **IDLE:** Quando não há macro em execução.

**Comandos:**  
`macro create <nome>`, `macro add <pose> <tempo>`, `macro save <nome>`, `macro play <nome>`, `macro stop`, `macro list` e `macro delete <nome>`.

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
