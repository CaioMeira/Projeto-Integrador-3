# Tecnologias e justificativas

## 1) Microcontrolador (ESP32) e escolha/mapeamento dos servomotores

### Por que escolher o ESP32

O **ESP32** é uma ótima escolha para o seu braço robótico por vários motivos práticos:

- **Conectividade integrada** (Wi-Fi + Bluetooth) — facilita teleop remoto, integração com PC/ROS via rede e OTA. :contentReference[oaicite:0]{index=0}
- **CPU e recursos** — dual-core, clock até 240 MHz, memória suficiente para tarefas de controle, filtros, e uma pilha de comunicação leve; bom para rodar controladores simples e gerir comunicação com ROS (via micro-ROS/rosserial) ou com um nó intermediário. :contentReference[oaicite:1]{index=1}
- **Periféricos e PWMs** — várias GPIOs, timers e suporte a PWM; mas como servos consomem PWM contínuo, é comum delegar PWM a um driver dedicado (ex.: PCA9685) quando há muitos servos. :contentReference[oaicite:2]{index=2}
- **Ecossistema** — muitas libs, exemplos (Arduino/PlatformIO/ESP-IDF) e comunidade (útil para prototipagem e debugging).

**Recomendações de integração do ESP32**

- Use o ESP32 para **lógica de controle**, leitura de sensores (se houver), sequenciamento e comunicação com ROS.
- Para **gerar PWM de muitos canais** com estabilidade e evitar interferência nas tarefas do micro, use um driver I²C PWM (ex.: PCA9685) ou um controlador PWM externo — assim você libera timers do ESP32. :contentReference[oaicite:3]{index=3}
- **Alimentação**: mantenha a alimentação dos servos separada da alimentação do ESP32; conecte os GNDs em comum. Use um PSU/pack com corrente adequada (verifique as correntes de pico nos datasheets).
- Verifique se o sinal de controle de 3.3V do ESP32 é reconhecido pelo servo (na maioria dos servos hobby é OK, mas confirme; caso contrário use um level shifter).

---

### Mapeamento proposto dos servos (com justificativa)

> Com base no que você descreveu (2× SG90, 3× HXT/HX5010, 2× HK15328D), aqui vai uma sugestão de onde cada um faz mais sentido e por quê:

| Atuador (modelo)                                      |                                                  Articulação sugerida | Por que aqui (resumo)                                                                                                                                                                                                                                        | Fonte / nota                               |
| ----------------------------------------------------- | --------------------------------------------------------------------: | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ------------------------------------------ |
| **SG90 (micro servo)**                                |                                       **Garra (abertura/fechamento)** | Micro-servos SG90 são leves, baratos e suficientes para abrir/fechar uma garra pequena com baixa carga. Bom para movimentos de baixa carga/pequena inércia. :contentReference[oaicite:4]{index=4}                                                            | SG90: torque ~1.8 kg·cm (verificar versão) |
| **SG90 (ou similar)**                                 |             **Pequena rotação da "cabeça"** (se o movimento for leve) | Se a rotação do “head” for apenas estética ou com baixa carga, outro SG90 pode servir; caso a cabeça sustente sensores/peso, prefira um servo maior. :contentReference[oaicite:5]{index=5}                                                                   |
| **HX5010 / HXT5010 (servo standard de maior torque)** | **Cotovelo / rotação da base (ou cabeça se precisar de mais torque)** | Servos como HXT/HX5010 oferecem torque e velocidade superiores aos micro-servos; bons para articulações que suportam braço e massa. Use um para o cotovelo e outro para girar a base (dependendo do torque requerido). :contentReference[oaicite:6]{index=6} |
| **HK15328D (hi-torque digital)**                      |                                                **Ombro (2 unidades)** | Articulação do ombro precisa mover parte do braço carregando inércia — por isso servos hi-torque digitais (mais robustos, com engrenagens metálicas e melhor comportamento sob carga) são indicados. :contentReference[oaicite:7]{index=7}                   |

**Notas práticas**

- Os modelos citados acima têm **torques e dimensões diferentes** — sempre confirme a capacidade (kg·cm) no datasheet e dimensione o braço mecanicamente para não exceder. :contentReference[oaicite:8]{index=8}
- **Se houver dúvida com relação ao torque** (por exemplo, se os braços ou garra forem maiores/pesados), prefira servos com margem de torque (×1.5 a ×2) para evitar stall.
- **Driver PWM**: com 7 servos totais você já pode usar um PCA9685 (16 canais) para simplificar o controle via I²C e evitar load no ESP32. O PCA9685 permite ajustar frequência (~50 Hz para servos tradicionais) e tem resolução de 12 bits, muito útil para suavidade. :contentReference[oaicite:9]{index=9}

**Sugestões de implementação imediata**

- Comece conectando 2–3 servos direto no ESP32 para testar lógica e timings; depois adicione o PCA9685 e migre a geração do PWM.
- Teste e registre o **consumo de corrente em pico** de cada servo (stall current) e dimensione a fonte (e fusíveis) conforme o pior cenário (vários servos travados).
- Adicione **capacitores de desacoplamento** perto da linha de alimentação dos servos e evite alimentar servos pela porta USB do ESP32.

---

## 2) ROS no projeto — opções e como aplicar

### Opções principais para conectar ESP32 ao ROS

Há duas abordagens comuns, com tradeoffs:

1. **micro-ROS (recomendado se você escolher ROS 2)**

   - **O que é:** uma implementação do ROS 2 para microcontroladores que permite que MCUs publiquem/assinem tópicos ROS2 e participem da arquitetura ROS de forma “nativa”. :contentReference[oaicite:10]{index=10}
   - **Por que usar:** projetado para sistemas embarcados (inclui suporte a FreeRTOS, transporte UDP/Wi-Fi, etc.), é mais robusto e moderno para ROS2. Existem ports/guia para ESP32 e tutoriais (ex.: build com micro_ros_setup e firmware para ESP32). :contentReference[oaicite:11]{index=11}
   - **Como aplicar no seu projeto:** rodar um **micro-ROS agent** no PC (ou Raspberry Pi) e o firmware micro-ROS no ESP32; o ESP32 publica estados dos servos/sensores e recebe comandos de posição/velocidade. Há exemplos para transporte por Wi-Fi/UDP e também por serial (USB). :contentReference[oaicite:12]{index=12}

2. **rosserial (alternativa simples, tipicamente ROS 1)**
   - **O que é:** pilha que permite que microcontroladores atuem como nós ROS via serial (ou TCP). É simples para protótipos com ROS1 e microcontroladores Arduino-like. :contentReference[oaicite:13]{index=13}
   - **Contra:** pode haver instabilidade/limitações em placas ESP32 (há relatos de reboots e adaptações necessárias); além disso, rosserial é pensado para ROS1 — se seu grupo migrar para ROS2, micro-ROS é mais apropriado. :contentReference[oaicite:14]{index=14}

### Arquitetura sugerida usando ROS

- **Nó no PC / Raspberry Pi (ROS)**
  - Responsável por planejamento de alto nível (path planning, buscas, UI, algoritmos pesados), visualização (RViz) e simulação (Gazebo).
- **micro-ROS agent**
  - Ponte entre ROS2 e o firmware do ESP32 (mantém tópicos/serviços entre o PC e o MCU). :contentReference[oaicite:15]{index=15}
- **ESP32 (firmware com micro-ROS)**
  - Nó embarcado responsável por: receber comandos de posição/trajectória, gerar setpoints para os servos (ou enviar comandos via PCA9685), ler telemetria (encoder / IMU se houver), aplicar controle em malha fechada (se fizer PID local). :contentReference[oaicite:16]{index=16}

### Integração com Gazebo

- Para **simulação** é recomendável modelar o braço no URDF e simular no **Gazebo** integrando com ROS (controladores e tópicos sim <-> real). Isso permite testar trajetórias, colisões e tune dos controladores antes do hardware. A integração ROS↔Gazebo é padrão nas stacks ROS.

_(Nota: documentação e tutoriais do ROS/Gazebo variam conforme ROS1/ROS2 — adapte ao ambiente que você escolher.)_

---

## Checklist prático / próximos passos

- [ ] Criar `tecnologias.md` com este conteúdo no repo.
- [ ] Confirmar o torque/peso das partes mecânicas e dimensionar servos com margem. (checagem: datasheets HXT/HK/SG90) :contentReference[oaicite:18]{index=18}
- [ ] Decidir ROS1 vs ROS2 — se ROS2, priorizar **micro-ROS**; se ROS1 e for só protótipo, rosserial pode servir. :contentReference[oaicite:19]{index=19}
- [ ] Planejar fonte de alimentação (corrente de pico) e driver PWM (PCA9685 recomendado para muitos servos). :contentReference[oaicite:20]{index=20}
- [ ] Criar URDF do braço e testar em Gazebo antes de montar hardware (valida colisões e movimentos).

---

## Referências rápidas

- ESP32 datasheet / overview. :contentReference[oaicite:21]{index=21}
- TowerPro SG90 (micro-servo) — specs / datasheet. :contentReference[oaicite:22]{index=22}
- HXT/HX5010 (servo standard de torque médio-alto) — exemplos de specs. :contentReference[oaicite:23]{index=23}
- HK15328D (hi-torque digital) — página do produto. :contentReference[oaicite:24]{index=24}
- PCA9685 16-channel PWM/servo driver (Adafruit guide). :contentReference[oaicite:25]{index=25}
- micro-ROS (ESP32 port + tutoriais). :contentReference[oaicite:26]{index=26}
- rosserial (wiki/tutorial para microcontroladores). :contentReference[oaicite:27]{index=27}
