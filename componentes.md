# Componentes e Justificativas Técnicas

## 1. Microcontrolador ESP32 e Mapeamento dos Servomotores

### Por que escolher o ESP32 para controle de braço robótico

O ESP32 é uma excelente escolha para sistemas embarcados em robótica, especialmente para braços robóticos com múltiplos graus de liberdade. Ele oferece:

- **Conectividade integrada (Wi-Fi + Bluetooth)**  
  Ideal para controle remoto, integração com ROS via rede local, atualizações OTA e comunicação com interfaces web ou móveis.

- **Alto desempenho computacional**  
  Processador dual-core com clock de até 240 MHz, suporte a multitarefa via FreeRTOS, e memória suficiente para controle de servos, leitura de sensores e comunicação com ROS.

- **Recursos periféricos robustos**  
  GPIOs abundantes, suporte a PWM, ADCs, SPI, I²C, UARTs, permitindo controle direto de servos, leitura de sensores analógicos/digitais e comunicação com periféricos externos.

- **Ecossistema maduro e flexível**  
  Compatível com Arduino IDE, PlatformIO e ESP-IDF. Possui ampla documentação, bibliotecas e comunidade ativa — facilitando prototipagem, debugging e integração com ROS/micro-ROS.

### Recomendações de integração com servomotores

- Utilizar o ESP32 para controle lógico, leitura de sensores e comunicação com ROS.
- Para controle de múltiplos servos, delegue PWM a um driver dedicado como o PCA9685 (via I²C), que oferece até 16 canais PWM com resolução de 12 bits e frequência ajustável.
- Alimentar os servos separadamente do ESP32, conectando os GNDs em comum. Use fontes com corrente suficiente para picos de consumo (stall current).
- Verificar se os sinais de controle de 3.3V do ESP32 são compatíveis com os servos. Caso contrário, utilizar level shifters para garantir integridade de sinal.

---

### Mapeamento proposto dos servos

Com base nos modelos disponíveis (2× SG90, 3× HXT/HX5010, 2× HK15328D), segue uma sugestão de alocação:

| Servo (modelo)                  | Articulação sugerida        | Justificativa técnica                                                                                         |
|--------------------------------|------------------------------|---------------------------------------------------------------------------------------------------------------|
| SG90 (micro servo)             | Garra (abertura/fechamento)  | Leve, barato, torque suficiente para movimentos de baixa carga. Ideal para garras pequenas.                   |
| SG90 (ou similar)              | Rotação da “cabeça” (leve)   | Adequado para movimentos estéticos ou de sensores leves. Se houver carga maior, prefira servo padrão.         |
| HX5010 / HXT5010               | Cotovelo / rotação da base   | Servo padrão com torque médio-alto. Ideal para articulações que sustentam peso ou movimentam o braço.         |
| HK15328D (digital hi-torque)   | Ombro (2 unidades)           | Servo robusto com engrenagens metálicas e controle digital. Ideal para articulações com alta inércia.         |

---

## 2. ROS no Projeto — Integração com ESP32

### Visão geral

O ROS (Robot Operating System) é uma plataforma amplamente utilizada para desenvolvimento de sistemas robóticos modulares e escaláveis. Ele oferece uma arquitetura baseada em nós, tópicos e serviços, permitindo que diferentes componentes de um robô se comuniquem de forma eficiente. Para integrar microcontroladores como o ESP32 ao ROS, existem duas abordagens principais, cada uma com suas vantagens e limitações.

---

### Opções de integração com ROS

Durante as pesquisas realizadas, identificaram-se duas formas principais de conectar o ESP32 ao ROS:

---

#### 1. micro-ROS (recomendado para ROS 2)

micro-ROS é uma extensão do ROS 2 voltada para microcontroladores com recursos limitados. Ele permite que o ESP32 funcione como um nó ROS 2 completo, publicando e assinando tópicos, utilizando serviços e participando da rede ROS 2.

**Principais características:**

- Compatível com FreeRTOS, que é amplamente suportado pelo ESP32.
- Suporte a transporte via UDP, Wi-Fi ou serial, permitindo flexibilidade na comunicação.
- Utiliza o protocolo DDS-XRCE, que é otimizado para dispositivos embarcados.
- Comunicação baseada em arquitetura Client-Agent, onde o ESP32 (Client) se comunica com um agente ROS 2 rodando no PC ou Raspberry Pi.

**Vantagens:**

- Integração nativa com ROS 2, sem necessidade de conversões ou adaptações.
- Suporte contínuo da comunidade ROS 2 e atualizações regulares.
- Permite escalabilidade e modularidade para sistemas mais complexos.
- Suporte a múltiplos tipos de transporte (Wi-Fi, serial, Ethernet).

**Desvantagens:**

- Curva de aprendizado um pouco maior para iniciantes.
- Requer configuração do ambiente ROS 2 e compilação cruzada do firmware.

**Exemplo de fluxo de trabalho:**

1. Instalar ROS 2 (ex.: Humble) no PC ou Raspberry Pi.
2. Criar e compilar o workspace do micro-ROS Agent.
3. Configurar e compilar o firmware micro-ROS para ESP32 (via Arduino IDE, PlatformIO ou ESP-IDF).
4. Estabelecer comunicação entre ESP32 e Agent via Wi-Fi ou USB.
5. Publicar/assinar tópicos ROS 2 diretamente do ESP32.

Referência prática: [Tutorial micro-ROS com ESP32](https://www.hackster.io/514301/micro-ros-on-esp32-using-arduino-ide-1360ca)

---

#### 2. rosserial (alternativa para ROS 1)

rosserial é uma biblioteca que permite que microcontroladores comuniquem-se com ROS 1 via uma conexão serial (USB, UART, etc.). O microcontrolador atua como um nó ROS simplificado, e o PC executa o rosserial_server que traduz os dados para a rede ROS.

**Principais características:**

- Comunicação baseada em protocolo serial com encapsulamento de mensagens ROS.
- Suporte a microcontroladores Arduino-like, incluindo ESP32 (com adaptações).
- Utiliza arquitetura cliente-servidor, semelhante ao micro-ROS.

**Vantagens:**

- Simples de implementar, ideal para protótipos rápidos.
- Boa documentação para ROS 1 e Arduino IDE.
- Menor complexidade de configuração inicial.

**Desvantagens:**

- Suporte limitado para ROS 2.
- Instabilidade em algumas placas ESP32 (reboots, perda de pacotes).
- Menor eficiência e escalabilidade comparado ao micro-ROS.
- Dependência de conexão serial, o que limita mobilidade e flexibilidade.

**Quando usar rosserial:**

- Projetos simples ou educacionais baseados em ROS 1.
- Protótipos com poucos nós e baixa demanda de comunicação.
- Situações onde o tempo de desenvolvimento é curto e a confiabilidade não é crítica.

Referência técnica: [Comparação micro-ROS vs rosserial](https://micro.ros.org/docs/concepts/middleware/rosserial/)

---

### Comparativo resumido

| Critério                  | micro-ROS (ROS 2)                     | rosserial (ROS 1)                  |
|--------------------------|---------------------------------------|------------------------------------|
| Compatibilidade          | ROS 2                                 | ROS 1                              |
| Transporte               | UDP, Wi-Fi, Serial, Ethernet          | Serial (USB/UART)                  |
| Arquitetura              | Client-Agent (DDS-XRCE)               | Cliente-Servidor (ROS messages)    |
| Suporte a ESP32          | Sim (via FreeRTOS)                    | Sim (com adaptações)               |
| Escalabilidade           | Alta                                   | Baixa                              |
| Estabilidade             | Alta                                   | Média (instável em ESP32)          |
| Complexidade de setup    | Moderada                               | Baixa                              |
| Comunidade e futuro      | Ativo e em expansão                   | Legado, com suporte limitado       |

---

### Recomendação para o projeto

- Se o projeto utilizar ROS 2, a escolha ideal é o micro-ROS, pois oferece integração nativa, maior estabilidade e suporte futuro.
- Se o projeto for baseado em ROS 1 e tiver foco em prototipagem rápida, o rosserial pode ser utilizado, com atenção às limitações de estabilidade no ESP32.


### Arquitetura sugerida

```plaintext
[ PC / Raspberry Pi ]
        ↑
  micro-ROS Agent
        ↑
     [ ESP32 ]
        ↓
     PCA9685 → Servos
```

- **PC/RPi**: Planejamento, visualização (RViz), simulação (Gazebo), interface de usuário.
- **ESP32**: Controle embarcado, leitura de sensores, geração de setpoints, controle local (PID).
- **Micro-ROS Agent**: Ponte entre ROS2 e ESP32 via Wi-Fi ou USB.

### Simulação com Gazebo

- Modelar o braço em URDF e simule no Gazebo para validar movimentos, colisões e controladores.
- A integração ROS ↔ Gazebo é padrão e permite testes antes da montagem física.

---

## Checklist de Implementação

- [ ] Criar `componentes.md` com este conteúdo.
- [ ] Verificar torque e peso das partes mecânicas (consultar datasheets).
- [ ] Escolher entre ROS1 (rosserial) ou ROS2 (micro-ROS).
- [ ] Planejar fonte de alimentação com margem para picos.
- [ ] Integrar driver PWM (PCA9685) para controle dos servos.
- [ ] Modelar URDF e testar no Gazebo.

---

## Referências Técnicas (colocar links depois)

- ESP32 datasheet e overview  
- TowerPro SG90 (micro-servo) — specs / datasheet  
- HXT/HX5010 (servo standard de torque médio-alto) — exemplos de specs  
- HK15328D (hi-torque digital) — página do produto  
- PCA9685 16-channel PWM/servo driver (Adafruit guide)  
- micro-ROS (ESP32 port + tutoriais)  
- rosserial (wiki/tutorial para microcontroladores)

