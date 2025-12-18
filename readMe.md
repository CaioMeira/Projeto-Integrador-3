<img src="Imagens/ifsc-logo.png"
     width="30%"
     style="padding: 10px">

# Projeto Integrador III - Braço Robótico

## Descrição do Projeto

Repositório destinado ao desenvolvimento do **Projeto Integrador III** do curso de Engenharia Eletrônica.  
O projeto consiste na construção de um **braço robótico controlado por ESP32** integrando o protocolo de comunicação **ROS 2**

---

## Requisitos para Executar o Projeto

### 1. Firmware ESP32 (Obrigatório)

**Arduino IDE** com as seguintes bibliotecas instaladas:

- `ESP32Servo` - Controle de servos
- `EEPROM` (incluída no core ESP32)
- `micro_ros_arduino` v2.0.7-humble - Comunicação ROS 2 (opcional)

**Como instalar:**

```
Arduino IDE → Sketch → Include Library → Manage Libraries
Buscar e instalar cada biblioteca acima
```

### 2. Integração ROS 2 (Opcional - 2 métodos)

#### **Método A: Script Python (Recomendado - Funciona com ESP32 padrão)**

- **Python 3.8+**
- **ROS 2 Humble** (Ubuntu 22.04 ou WSL2)
- **pyserial**: `pip3 install pyserial`

**Vantagens:** Não requer modificação no ESP32, funciona com qualquer modelo

#### **Método B: ROS Nativo via micro-ROS Agent (Docker)**

- **Docker** instalado
- **ROS 2 Humble** (Ubuntu 22.04 ou WSL2)
- **ESP32 com biblioteca micro-ROS** compilada no firmware

**Limitação:** ESP32 padrão (520KB RAM) suporta apenas `/run_macro`. Para todos os tópicos, use ESP32-WROVER ou **Método A (Script Python)**.

### 3. Hardware

- ESP32 DevKit (qualquer modelo)
- PCA9685 (driver PWM para servos)
- 7× Servos MG996R ou similar
- Fonte 5V/6V adequada para os servos

---

> ## Equipe
>
> - Caio Neves Meira
> - Guilherme da Costa Franco
> - Marcelo Zampieri Pereira da Silva
>
> Professores: Matheus Leitzke Pinto e Renan Augusto Starke

---

<p align=center><strong>SUMÁRIO</strong></p>

[**1. INTRODUÇÃO**](./introdução.md)<br>
[**2. COMPONENTES**](./componentes.md)<br>
[**3. LISTA DE COMPONENTES**](./lista-de-componentes.md)<br>
[**4. IMAGENS**](./Imagens/)<br>

---
