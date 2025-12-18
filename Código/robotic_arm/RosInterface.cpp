/**
 * @file RosInterface.cpp
 * @brief Implementação do cliente micro-ROS (v2 - Serial).
 * Se conecta via Serial (USB) a um Agente micro-ROS e expõe tópicos para
 * controle (Subscribers) e feedback (Publishers).
 */

#include "RosInterface.h"

// --- Dependências do micro-ROS ---
#include <micro_ros_arduino.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

// --- Mensagens ROS ---
#include <std_msgs/msg/string.h>
#include <sensor_msgs/msg/joint_state.h>

// --- Módulos do Braço Robótico ---
#include "Config.h"
#include "MotionController.h"
#include "Sequencer.h"
#include "PoseManager.h"

// =================================================================
// 1. Variáveis Globais do micro-ROS
// =================================================================

// Converte Graus (0-180) para Radianos (0-PI)
#define DEG_TO_RAD(g) ((g) * PI / 180.0f)
// Converte Radianos para Graus
#define RAD_TO_DEG(r) ((r) * 180.0f / PI)

rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;
rcl_timer_t timer;

// --- Publishers ---
rcl_publisher_t pub_joint_states;
rcl_publisher_t pub_arm_status;
sensor_msgs__msg__JointState joint_state_msg; // Mensagem de estado das juntas
std_msgs__msg__String arm_status_msg;         // Mensagem de status (IDLE, MOVING)

// --- Subscribers ---
rcl_subscription_t sub_joint_goals;
rcl_subscription_t sub_run_macro;
rcl_subscription_t sub_run_pose;
sensor_msgs__msg__JointState joint_goals_msg; // Mensagem de ângulos alvo
std_msgs__msg__String run_macro_msg;          // Mensagem para rodar macro
std_msgs__msg__String run_pose_msg;           // Mensagem para rodar pose

// Nomes das juntas (deve corresponder ao seu URDF no ROS)
const char *joint_names[NUM_SERVOS] = {"junta_base", "junta_ombro1", "junta_ombro2", "junta_cotovelo", "junta_mao", "junta_pulso", "junta_garra"};

// =================================================================
// 2. Callbacks (Funções chamadas quando o ROS envia um comando)
// =================================================================

/**
 * @brief Callback para o tópico /joint_goals.
 * Recebe ângulos em RADIANOS e os converte para graus.
 */
void jointGoalsCallback(const void *msgin)
{
    const sensor_msgs__msg__JointState *msg = (const sensor_msgs__msg__JointState *)msgin;

    // Verificação de segurança
    if (msg->position.size != NUM_SERVOS)
    {
        Serial.print(F("ERRO ROS: /joint_goals recebido com "));
        Serial.print(msg->position.size);
        Serial.print(F(" posicoes, esperado "));
        Serial.println(NUM_SERVOS);
        return;
    }

    int targetAngles[NUM_SERVOS];
    for (int i = 0; i < NUM_SERVOS; i++)
    {
        // Converte de Radianos (ROS) para Graus (Braço)
        targetAngles[i] = (int)RAD_TO_DEG(msg->position.data[i]);
    }

    // Usa a velocidade padrão do MotionController
    unsigned long duration = MotionController::calculateDurationBySpeed(targetAngles);

    Serial.println("ROS: Recebido comando /joint_goals.");
    MotionController::startSmoothMove(targetAngles, duration);
}

/**
 * @brief Callback para o tópico /run_macro
 */
void runMacroCallback(const void *msgin)
{
    const std_msgs__msg__String *msg = (const std_msgs__msg__String *)msgin;
    Serial.print(F("ROS: Recebido comando /run_macro: '"));
    Serial.print(msg->data.data);
    Serial.println(F("'"));
    Sequencer::startMacro(msg->data.data);
}

/**
 * @brief Callback para o tópico /run_pose
 */
void runPoseCallback(const void *msgin)
{
    const std_msgs__msg__String *msg = (const std_msgs__msg__String *)msgin;
    Serial.print(F("ROS: Recebido comando /run_pose: '"));
    Serial.print(msg->data.data);
    Serial.println(F("'"));
    PoseManager::loadPoseByName(msg->data.data);
}

// =================================================================
// 3. Timer Callback (Função para publicar o estado)
// =================================================================

/**
 * @brief Chamado pelo timer (a 10 Hz) para publicar o estado do braço.
 */
void timerCallback(rcl_timer_t *timer, int64_t last_call_time)
{
    (void)last_call_time;
    if (timer == NULL)
        return;

    // --- Publicar /joint_states ---
    // Atualiza a mensagem com os ângulos LÓGICOS atuais
    for (int i = 0; i < NUM_SERVOS; i++)
    {
        // Converte de Graus (Braço) para Radianos (ROS)
        joint_state_msg.position.data[i] = DEG_TO_RAD(currentAngles[i]);
    }
    // Define o timestamp
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    joint_state_msg.header.stamp.sec = ts.tv_sec;
    joint_state_msg.header.stamp.nanosec = ts.tv_nsec;

    rcl_publish(&pub_joint_states, &joint_state_msg, NULL);

    // --- Publicar /arm_status ---
    const char *status = "IDLE";
    if (Sequencer::isRunning())
    {
        status = "RUNNING_MACRO";
    }
    else if (MotionController::isMoving())
    {
        status = "MOVING";
    }
    // Prepara a string para publicação (seguro contra overflow)
    size_t len = strlen(status);
    if (len >= arm_status_msg.data.capacity)
    {
        len = arm_status_msg.data.capacity - 1;
    }
    strncpy(arm_status_msg.data.data, status, len);
    arm_status_msg.data.data[len] = '\0';
    arm_status_msg.data.size = len;
    rcl_publish(&pub_arm_status, &arm_status_msg, NULL);
}

// =================================================================
// 4. Funções de Setup e Update (Públicas)
// =================================================================

namespace RosInterface
{

    // Função auxiliar para inicializar a msg /joint_states (CRÍTICO)
    void initJointStateMsg()
    {
        joint_state_msg.name.size = NUM_SERVOS;
        joint_state_msg.name.data = (rosidl_runtime_c__String *)malloc(sizeof(rosidl_runtime_c__String) * NUM_SERVOS);
        // Aloca strings para os nomes
        for (int i = 0; i < NUM_SERVOS; i++)
        {
            joint_state_msg.name.data[i].data = (char *)malloc(POSE_NAME_LEN * sizeof(char));
            size_t len = strlen(joint_names[i]);
            if (len >= POSE_NAME_LEN)
                len = POSE_NAME_LEN - 1;
            strncpy(joint_state_msg.name.data[i].data, joint_names[i], len);
            joint_state_msg.name.data[i].data[len] = '\0';
            joint_state_msg.name.data[i].size = len;
        }
        // Aloca floats para as posições
        joint_state_msg.position.size = NUM_SERVOS;
        joint_state_msg.position.data = (double *)malloc(sizeof(double) * NUM_SERVOS);
        for (int i = 0; i < NUM_SERVOS; i++)
        {
            joint_state_msg.position.data[i] = 0.0;
        }
        // (Opcional) Aloca para velocity e effort
        joint_state_msg.velocity.size = 0;
        joint_state_msg.velocity.data = NULL;
        joint_state_msg.effort.size = 0;
        joint_state_msg.effort.data = NULL;
    }

    // Função auxiliar para inicializar a msg /arm_status
    void initArmStatusMsg()
    {
        const int max_status_len = 30; // "RUNNING_MACRO" é o maior
        arm_status_msg.data.data = (char *)malloc(max_status_len * sizeof(char));
        arm_status_msg.data.size = 0;
        arm_status_msg.data.capacity = max_status_len;
    }

    void setup()
    {
        Serial.println("Iniciando RosInterface (modo SERIAL)...");

        // 1. Configurar transporte micro-ROS (Serial)
        //    A Serial já foi iniciada em robotic_arm.ino
        set_microros_transports();
        Serial.println("Transporte micro-ROS: Serial (USB)");
        delay(2000); // Espera para garantir que o Agente possa detectar

        // 2. Inicializar alocador
        allocator = rcl_get_default_allocator();

        // 3. Inicializar Suporte e Nó
        rclc_support_init(&support, 0, NULL, &allocator);
        rclc_node_init_default(&node, "robotic_arm_node", "", &support);

        // 4. Criar Publishers
        rclc_publisher_init_default(
            &pub_joint_states, &node,
            ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, JointState),
            "/joint_states");

        rclc_publisher_init_default(
            &pub_arm_status, &node,
            ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String),
            "/arm_status");

        // 5. Criar Subscribers e associar Callbacks
        // NOTA: joint_goals e run_pose comentados para economizar memória (ESP32 limitado)
        // Apenas run_macro está ativo para automação via ROS 2
        /*
        rclc_subscription_init_default(
            &sub_joint_goals, &node,
            ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, JointState),
            "/joint_goals");
        */

        rclc_subscription_init_default(
            &sub_run_macro, &node,
            ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String),
            "/run_macro");

        /*
        rclc_subscription_init_default(
            &sub_run_pose, &node,
            ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String),
            "/run_pose");
        */

        // 6. Criar Timer (para publicar a 10Hz)
        const unsigned long timer_period = 100; // 100 ms = 10 Hz
        rclc_timer_init_default(&timer, &support, RCL_MS_TO_NS(timer_period), timerCallback);

        // 7. Inicializar o Executor
        rclc_executor_init(&executor, &support.context, 2, &allocator); // 2 = 1 sub + 1 timer (otimizado)
        rclc_executor_add_timer(&executor, &timer);
        // rclc_executor_add_subscription(&executor, &sub_joint_goals, &joint_goals_msg, &jointGoalsCallback, ON_NEW_DATA);
        rclc_executor_add_subscription(&executor, &sub_run_macro, &run_macro_msg, &runMacroCallback, ON_NEW_DATA);
        // rclc_executor_add_subscription(&executor, &sub_run_pose, &run_pose_msg, &runPoseCallback, ON_NEW_DATA);

        // 8. Inicializar as mensagens (Alocação Dinâmica)
        initJointStateMsg();
        initArmStatusMsg();

        Serial.println("micro-ROS (Serial) configurado e pronto.");
    }

    void update()
    {
        // Processa todas as tarefas pendentes do micro-ROS (callbacks, timers)
        // O timeout 0 torna o spin não-bloqueante
        rclc_executor_spin_some(&executor, 0);
    }

} // namespace
