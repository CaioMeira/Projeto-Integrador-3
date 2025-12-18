# Guia de Integração micro-ROS com ESP32

**Versão:** 1.0  
**ROS 2:** Humble  
**Plataforma:** ESP32 + Ubuntu 22.04  
**Protocolo:** Serial (USB)

---

## Índice

1. [Visão Geral](#visão-geral)
2. [Arquitetura](#arquitetura)
3. [Configuração do ESP32](#passo-1-configuração-do-esp32)
4. [Agente micro-ROS no PC](#passo-2-agente-micro-ros-no-pc)
5. [Testando a Comunicação](#passo-3-testando-a-comunicação)
6. [Comandos Disponíveis](#comandos-ros-disponíveis)
7. [Troubleshooting](#troubleshooting)

---

## Visão Geral

Seu braço robótico agora é um **nó ROS 2** (`robotic_arm_node`) que:

- **Publica** estado das juntas e status do braço (10 Hz)
- **Recebe** comandos de movimento, poses e macros via ROS
- Comunica via **Serial (USB)** com micro-ROS Agent no PC
- Compatível com **ROS 2 Humble** (Ubuntu 22.04)

### Por que usar micro-ROS?

- Integração nativa com ecossistema ROS 2
- Visualização no RViz2 (com URDF do braço)
- Compatibilidade com MoveIt2 para planejamento de trajetórias
- Scripts Python/C++ para automação complexa

---

## Arquitetura

```
┌─────────────────────────────────────────────────────────────┐
│                       PC Linux (Ubuntu 22.04)               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  micro-ROS Agent (Docker)                            │   │
│  │  - Porta: /dev/ttyUSB0 (Serial)                      │   │
│  │  - Baudrate: 115200                                  │   │
│  └─────────────────┬────────────────────────────────────┘   │
│                    │ USB                                    │
│  ┌─────────────────┴────────────────────────────────────┐   │
│  │  ROS 2 Humble                                        │   │
│  │  - ros2 topic list/echo/pub                          │   │
│  │  - RViz2, MoveIt2, etc                               │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                          ▲
                          │ Serial (115200 baud)
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                       ESP32 (Braço Robótico)                │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  robotic_arm_node (micro-ROS)                        │   │
│  │                                                      │   │
│  │  Publishers (10 Hz):                                 │   │
│  │    - /joint_states → Ângulos atuais (radianos)       │   │
│  │    - /arm_status → IDLE / MOVING / RUNNING_MACRO     │   │
│  │                                                      │   │
│  │  Subscribers:                                        │   │
│  │    - /joint_goals ← Comandar ângulos (radianos)      │   │
│  │    - /run_pose ← Executar pose salva                 │   │
│  │    - /run_macro ← Executar macro (sequência)         │   │
│  └──────────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  MotionController + PoseManager + Sequencer          │   │
│  │  (Código otimizado Fases 1 e 2)                      │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### Fluxo de Comunicação

1. **ESP32** aguarda conexão do Agent via Serial
2. **PC** executa micro-ROS Agent (Docker) conectado à porta USB
3. **Agent** estabelece comunicação bidirecional
4. **ROS 2** vê o ESP32 como um nó normal (`robotic_arm_node`)
5. Você pode usar `ros2 topic pub/echo` normalmente!

---

## Passo 1: Configuração do ESP32

### 1.1 Instalar Biblioteca micro-ROS

**Arduino IDE:**

```
Sketch → Include Library → Manage Libraries
Buscar: "micro_ros_arduino"
Instalar versão compatível com ROS 2 Humble
```

**PlatformIO:**

```ini
[env:esp32]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps =
    https://github.com/micro-ROS/micro_ros_arduino/releases/download/v2.0.7/micro_ros_arduino-2.0.7.zip
```

### 1.2 Compilar e Enviar

1. Abra o projeto em `Código/robotic_arm/`
2. Compile e envie para o ESP32
3. Abra o **Monitor Serial** (115200 baud)

### 1.3 Verificar Inicialização

Você deve ver:

```
Iniciando Sistema do Braco Robotico v5.0...
Watchdog Timer configurado (15s).
...
Iniciando RosInterface (modo SERIAL)...
Transporte micro-ROS: Serial (USB)
micro-ROS (Serial) configurado e pronto.
```

> **Atenção:** O ESP32 ficará **esperando** o Agent se conectar. Isso é normal!

---

## Passo 2: Agente micro-ROS no PC

### 2.1 Identificar a Porta Serial

Conecte o ESP32 ao PC via USB e execute:

```bash
dmesg | grep tty
```

**Saída esperada:**

```
[12345.678] usb 1-1: FTDI USB Serial Device converter now attached to ttyUSB0
```

A porta é `/dev/ttyUSB0` (ou `ttyACM0`, depende do ESP32).

### 2.2 Rodar o Agent (Docker)

**Método 1: Acesso privilegiado (mais simples)**

```bash
docker run -it --rm --privileged \
    -v /dev:/dev \
    microros/micro-ros-agent:humble \
    serial --dev /dev/ttyUSB0 -b 115200
```

**Método 2: Mapear dispositivo específico (mais seguro)**

```bash
docker run -it --rm \
    --device=/dev/ttyUSB0:/dev/ttyUSB0 \
    microros/micro-ros-agent:humble \
    serial --dev /dev/ttyUSB0 -b 115200
```

### 2.3 Verificar Conexão

**Saída esperada no Agent:**

```
[1669123456.789] micro-ROS Agent
[1669123456.790] Version: 2.0.0
[1669123456.791] Serial device: /dev/ttyUSB0, baudrate: 115200
[1669123457.123] session established
[1669123457.124] Root.cpp init_session
```

> **Sucesso!** O ESP32 agora está conectado ao ROS 2.

---

## Passo 3: Testando a Comunicação

** IMPORTANTE:** Mantenha o terminal do Agent **rodando** durante todos os testes!

### 3.1 Listar Tópicos Disponíveis

Abra um **novo terminal** e execute:

```bash
ros2 topic list
```

**Saída esperada:**

```
/arm_status
/joint_goals
/joint_states
/parameter_events
/rosout
/run_macro
/run_pose
```

> Se você vê `/joint_states` e `/arm_status`, está funcionando!

---

### 3.2 Monitorar o Estado do Braço (Publishers)

#### Ver Estado das Juntas (10 Hz)

```bash
ros2 topic echo /joint_states
```

**Saída (exemplo):**

```yaml
header:
  stamp:
    sec: 1234567890
    nanosec: 123456789
  frame_id: ""
name:
  - junta_base
  - junta_ombro1
  - junta_ombro2
  - junta_cotovelo
  - junta_mao
  - junta_pulso
  - junta_garra
position:
  - 1.5707963 # 90° em radianos
  - 2.2689280 # 130° em radianos
  - 2.2689280 # 130° em radianos
  - 1.7453292 # 100° em radianos
  - 1.2217304 # 70° em radianos
  - 2.0943951 # 120° em radianos
  - 1.7453292 # 100° em radianos
velocity: []
effort: []
```

> **Dica:** Para converter radianos → graus: `graus = radianos × 180 / π`

#### Ver Status do Braço

```bash
ros2 topic echo /arm_status
```

**Saída:**

```yaml
data: "IDLE"
```

Possíveis valores:

- `IDLE` - Braço parado
- `MOVING` - Executando movimento suave
- `RUNNING_MACRO` - Executando sequência de macro

---

### 3.3 Comandar o Braço (Subscribers)

#### Comando 1: Executar Pose Salva

> **Pré-requisito:** Você deve ter uma pose salva (via Serial: `SAVE home`).

```bash
ros2 topic pub /run_pose std_msgs/msg/String "data: 'home'" --once
```

O braço deve se mover para a pose `home`.

---

#### Comando 2: Comandar Ângulos Específicos

Mover todos os servos para **90 graus** (1.5708 radianos):

```bash
ros2 topic pub /joint_goals sensor_msgs/msg/JointState "{
  name: ['junta_base', 'junta_ombro1', 'junta_ombro2', 'junta_cotovelo', 'junta_mao', 'junta_pulso', 'junta_garra'],
  position: [1.57, 1.57, 1.57, 1.57, 1.57, 1.57, 1.57]
}" --once
```

**Conversão rápida:**

- 0° = 0 rad
- 45° = 0.785 rad
- 90° = 1.57 rad
- 135° = 2.356 rad
- 180° = 3.14 rad

---

#### Comando 3: Executar Macro

**Pré-requisito:** Você deve ter uma macro salva (via Serial).

**Exemplo de criação de macro via Serial:**

```
RECORD pegar
MOVE 0 45
WAIT 1000
MOVE 1 90
END
```

**Executar via ROS:**

```bash
ros2 topic pub /run_macro std_msgs/msg/String "data: 'pegar'" --once
```

O braço executa toda a sequência automaticamente.

---

## Comandos ROS Disponíveis

### Subscribers (Enviar Comandos)

| Tópico         | Tipo                     | Descrição                        | Exemplo                                                  |
| -------------- | ------------------------ | -------------------------------- | -------------------------------------------------------- |
| `/joint_goals` | `sensor_msgs/JointState` | Comandar ângulos em **radianos** | Ver [Comando 2](#comando-2-comandar-ângulos-específicos) |
| `/run_pose`    | `std_msgs/String`        | Executar pose salva pelo nome    | `data: 'home'`                                           |
| `/run_macro`   | `std_msgs/String`        | Executar macro pelo nome         | `data: 'pegar'`                                          |

### Publishers (Receber Feedback)

| Tópico          | Tipo                     | Frequência | Descrição                                 |
| --------------- | ------------------------ | ---------- | ----------------------------------------- |
| `/joint_states` | `sensor_msgs/JointState` | 10 Hz      | Posição atual de todas as juntas          |
| `/arm_status`   | `std_msgs/String`        | 10 Hz      | Status: `IDLE`, `MOVING`, `RUNNING_MACRO` |

---

## Troubleshooting

### Problema 1: Agent não conecta

**Sintomas:**

```
[ERROR] Cannot open serial device /dev/ttyUSB0
```

**Solução:**

```bash
# Verificar permissões
ls -l /dev/ttyUSB0

# Adicionar seu usuário ao grupo dialout
sudo usermod -aG dialout $USER

# Fazer logout e login novamente
```

---

### Problema 2: ESP32 não aparece no `ros2 topic list`

**Verificar:**

1. Agent está rodando? (Docker container ativo?)
2. ESP32 mostra "session established" no Serial Monitor?
3. Baudrate correto? (115200 em ambos os lados)

**Teste:**

```bash
# Reiniciar ESP32
# Pressione o botão RST no ESP32

# Verificar Agent
docker ps  # Container deve estar UP
```

---

### Problema 3: "ERRO ROS: /joint_goals recebido com X posições"

**Causa:** Você enviou número incorreto de ângulos.

**Solução:** Sempre envie **7 posições** (uma para cada servo):

```bash
ros2 topic pub /joint_goals sensor_msgs/msg/JointState "{
  name: ['j0','j1','j2','j3','j4','j5','j6'],
  position: [1.57, 1.57, 1.57, 1.57, 1.57, 1.57, 1.57]
}" --once
```

---

### Problema 4: Watchdog reseta o ESP32

**Causa:** Loop principal travou por >15 segundos.

**Verificar:**

- Nenhum `delay()` longo no código
- `RosInterface::update()` está sendo chamado no `loop()`
- Nenhuma operação bloqueante

---

## FAQ: Preciso de 2 Terminais como na demonstração?

### **NÃO!** Seu código **JÁ FAZ PUB+SUB no mesmo nó**

**Na demo básica do ROS:**

- Terminal 1: `ros2 run demo_nodes_cpp talker` (só publica)
- Terminal 2: `ros2 run demo_nodes_cpp listener` (só escuta)

**No seu projeto:**

- Terminal 1: Agent (obrigatório)
- Terminal 2: Seus comandos `ros2 topic pub/echo` (quantos quiser!)

**O ESP32 é um único nó que:**

- Publica `/joint_states` e `/arm_status` (como o `talker`)
- Escuta `/joint_goals`, `/run_pose`, `/run_macro` (como o `listener`)

**Você pode abrir N terminais** para testar, mas o Agent deve estar sempre rodando.

---
