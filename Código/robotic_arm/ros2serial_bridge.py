#!/usr/bin/env python3
"""
ROS 2 ↔ Serial Bridge para Braço Robótico ESP32
================================================

Converte tópicos ROS 2 em comandos seriais e vice-versa.
Solução alternativa para ESP32 sem PSRAM.

Uso:
    python3 ros2serial_bridge.py /dev/ttyUSB0

Requisitos:
    pip3 install pyserial
    sudo apt install ros-humble-desktop
"""

import rclpy
from rclpy.node import Node
from std_msgs.msg import String
from sensor_msgs.msg import JointState
import serial
import threading
import sys
import time
import math


class RobotArmBridge(Node):
    def __init__(self, serial_port, baudrate=115200):
        super().__init__('robotic_arm_bridge')
        
        # Conexão serial
        self.get_logger().info(f'Conectando a {serial_port} @ {baudrate}...')
        try:
            self.serial = serial.Serial(serial_port, baudrate, timeout=1)
            time.sleep(2)  # Aguarda ESP32 resetar
            self.get_logger().info('Conexão serial estabelecida!')
        except serial.SerialException as e:
            self.get_logger().error(f'Erro ao abrir porta serial: {e}')
            sys.exit(1)
        
        # Publishers ROS (feedback do braço)
        self.pub_joint_states = self.create_publisher(JointState, '/joint_states', 10)
        self.pub_arm_status = self.create_publisher(String, '/arm_status', 10)
        
        # Subscribers ROS (comandos para o braço)
        self.sub_run_macro = self.create_subscription(
            String, '/run_macro', self.callback_run_macro, 10)
        self.sub_run_pose = self.create_subscription(
            String, '/run_pose', self.callback_run_pose, 10)
        self.sub_joint_goals = self.create_subscription(
            JointState, '/joint_goals', self.callback_joint_goals, 10)
        
        # Timer para publicar estado (10 Hz)
        self.timer = self.create_timer(0.1, self.publish_state)
        
        # Estado atual do braço
        self.current_angles = [90] * 7  # Valores padrão
        self.arm_status = "IDLE"
        
        # Thread para ler serial
        self.serial_thread = threading.Thread(target=self.serial_reader, daemon=True)
        self.serial_thread.start()
        
        self.get_logger().info('Bridge ROS 2 ↔ Serial inicializado!')
        self.get_logger().info('Tópicos ativos:')
        self.get_logger().info('  SUB: /run_macro, /run_pose, /joint_goals')
        self.get_logger().info('  PUB: /joint_states, /arm_status')
    
    def rad_to_deg(self, rad):
        """Converte radianos para graus"""
        return int(rad * 180.0 / math.pi)
    
    def deg_to_rad(self, deg):
        """Converte graus para radianos"""
        return deg * math.pi / 180.0
    
    def send_command(self, cmd):
        """Envia comando serial para o ESP32"""
        try:
            self.serial.write(f"{cmd}\n".encode())
            self.get_logger().info(f'Serial TX: {cmd}')
        except Exception as e:
            self.get_logger().error(f'Erro ao enviar comando: {e}')
    
    def serial_reader(self):
        """Thread que lê respostas do ESP32"""
        while rclpy.ok():
            try:
                if self.serial.in_waiting > 0:
                    line = self.serial.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        # Parse de status do braço
                        if "IDLE" in line or "MOVING" in line or "RUNNING_MACRO" in line:
                            if "IDLE" in line:
                                self.arm_status = "IDLE"
                            elif "MOVING" in line:
                                self.arm_status = "MOVING"
                            elif "RUNNING_MACRO" in line:
                                self.arm_status = "RUNNING_MACRO"

                        # Detecta mensagens textuais do firmware (sem palavras-chave diretas)
                        if "Macro '" in line and "concluida" in line:
                            self.arm_status = "IDLE"
                        if "Macro interrompida" in line:
                            self.arm_status = "IDLE"
                        
                        # Log de debug
                        if line and not line.startswith('---'):
                            self.get_logger().debug(f'Serial RX: {line}')
            except Exception as e:
                self.get_logger().error(f'Erro ao ler serial: {e}')
                time.sleep(0.1)
    
    def callback_run_macro(self, msg):
        """Callback para /run_macro"""
        macro_name = msg.data
        self.get_logger().info(f'ROS: Executando macro "{macro_name}"')
        self.send_command(f'macro play {macro_name}')
        self.arm_status = "RUNNING_MACRO"
    
    def callback_run_pose(self, msg):
        """Callback para /run_pose"""
        pose_name = msg.data
        self.get_logger().info(f'ROS: Carregando pose "{pose_name}"')
        self.send_command(f'pose load {pose_name}')
        self.arm_status = "MOVING"
    
    def callback_joint_goals(self, msg):
        """Callback para /joint_goals (ângulos em radianos)"""
        if len(msg.position) != 7:
            self.get_logger().warn(f'joint_goals deve ter 7 posições, recebeu {len(msg.position)}')
            return
        
        # Converte radianos para graus
        angles_deg = [self.rad_to_deg(rad) for rad in msg.position]
        cmd = f'move {" ".join(map(str, angles_deg))}'
        
        self.get_logger().info(f'ROS: Movendo juntas para {angles_deg}')
        self.send_command(cmd)
        self.arm_status = "MOVING"
    
    def publish_state(self):
        """Publica estado atual do braço (10 Hz)"""
        # Publicar joint_states
        joint_state = JointState()
        joint_state.header.stamp = self.get_clock().now().to_msg()
        joint_state.name = ['junta_base', 'junta_ombro1', 'junta_ombro2', 
                           'junta_cotovelo', 'junta_mao', 'junta_pulso', 'junta_garra']
        joint_state.position = [self.deg_to_rad(a) for a in self.current_angles]
        self.pub_joint_states.publish(joint_state)
        
        # Publicar arm_status
        status_msg = String()
        status_msg.data = self.arm_status
        self.pub_arm_status.publish(status_msg)
    
    def cleanup(self):
        """Fecha conexão serial"""
        if self.serial.is_open:
            self.serial.close()
        self.get_logger().info('Bridge encerrado.')


def main(args=None):
    if len(sys.argv) < 2:
        print("Uso: python3 ros2serial_bridge.py <porta_serial>")
        print("Exemplo: python3 ros2serial_bridge.py /dev/ttyUSB0")
        sys.exit(1)
    
    serial_port = sys.argv[1]
    
    rclpy.init(args=args)
    bridge = RobotArmBridge(serial_port)
    
    try:
        rclpy.spin(bridge)
    except KeyboardInterrupt:
        bridge.get_logger().info('Interrompido pelo usuário')
    finally:
        bridge.cleanup()
        bridge.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
