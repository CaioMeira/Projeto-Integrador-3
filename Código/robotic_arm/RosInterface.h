/**
 * @file RosInterface.h
 * @brief Define a interface do módulo de comunicação micro-ROS.
 * Atua como um "tradutor" entre os tópicos ROS e os módulos
 * de controle do braço robótico (MotionController, Sequencer, etc.)
 *
 * Comunicação: Serial (USB) via micro-ROS Agent
 * Frequência: 10 Hz para publicação de estado
 *
 * Tópicos Subscribers (recebe comandos):
 *   - /joint_goals (sensor_msgs/JointState) - Ângulos alvo em radianos
 *   - /run_macro (std_msgs/String) - Nome da macro para executar
 *   - /run_pose (std_msgs/String) - Nome da pose para carregar
 *
 * Tópicos Publishers (envia feedback):
 *   - /joint_states (sensor_msgs/JointState) - Estado atual das juntas
 *   - /arm_status (std_msgs/String) - Status do braço (IDLE/MOVING/RUNNING_MACRO)
 */
#ifndef ROS_INTERFACE_H
#define ROS_INTERFACE_H

namespace RosInterface
{

    /**
     * @brief Configura a conexão Serial e inicializa o micro-ROS.
     * Tenta se conectar ao micro-ROS Agent no PC host via USB.
     */
    void setup();

    /**
     * @brief Processa mensagens ROS recebidas (spin) e publica o estado.
     * Esta função é não-bloqueante e deve ser chamada no loop() principal.
     */
    void update();

} // namespace RosInterface

#endif // ROS_INTERFACE_H