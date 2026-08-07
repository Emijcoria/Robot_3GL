#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h" // Necesario para tiempos precisos en microsegundos (esp_rom_delay_us)

#define stack_size 1024 * 2
#define DT 0.001f             // 1 ms
#define ACELERACION 1000.0f   // pasos/s²
#define VELOCIDAD_MAX 3000.0f // pasos/s

const char *TAG = "main";
uint32_t delay_us = 0;
volatile int boton_presionado = 0;

typedef struct {
    int32_t pines[3]; // enable, step, dir
    char nombre_motor[10];
    float velocidad;
    float aceleracion;
    TaskHandle_t motor_handle;
} Config_motor;

static Config_motor motor1 = {
    .pines = {GPIO_NUM_2, GPIO_NUM_4, GPIO_NUM_5},
    .nombre_motor = "Motor1",
    .velocidad = 0.0f,
    .aceleracion = ACELERACION,
    .motor_handle = NULL
};

const int teclaspin[8] = {
    GPIO_NUM_4,
    GPIO_NUM_5,
    GPIO_NUM_16,
    GPIO_NUM_17,
    GPIO_NUM_18,
    GPIO_NUM_19,
    GPIO_NUM_21,
    GPIO_NUM_22
};

// Declaración de funciones
esp_err_t inicializar_gpios(void);
esp_err_t crear_tareas(void);
void vTaskPulsos(void *pvParameters);
void vTaskSTP1(void *pvParameters);
void vTaskTeclado(void *pvParameters);

void app_main(void)
{
    // 1. Inicializar Hardware con verificación esp_err_t
    if (inicializar_gpios() == ESP_OK) {
        ESP_LOGI(TAG, "Hardware e Entradas/Salidas inicializados correctamente");
    }

    // 2. Crear Tareas
    crear_tareas();
}

// Función de inicialización retornando esp_err_t
esp_err_t inicializar_gpios(void)
{
    // MOTOR 1 (EN, STP, DIR)
    for (int i = 0; i < 3; i++) {
        gpio_reset_pin(motor1.pines[i]);
        gpio_set_direction(motor1.pines[i], GPIO_MODE_OUTPUT);
    }
    gpio_set_level(motor1.pines[0], 1); // Deshabilitar motor por defecto (EN = HIGH)

    // PINES TECLADO
    for (int i = 0; i < 8; i++) {
        gpio_reset_pin(teclaspin[i]);
        gpio_set_direction(teclaspin[i], GPIO_MODE_INPUT);
        gpio_set_pull_mode(teclaspin[i], GPIO_PULLUP_ONLY);
    }

    return ESP_OK;
}

void vTaskTeclado(void *pvParameters)
{
    while (1)
    {
        int32_t select = -1;

        for (int i = 0; i < 8; i++)
        {
            if (gpio_get_level(teclaspin[i]) == 0)
            {
                select = teclaspin[i];
                break; // toma el primero que encuentra
            }
        }

        if (select != -1)
        {
            boton_presionado = 1; // Se activa la bandera para la rampa
            if (motor1.motor_handle != NULL)
            {
                xTaskNotify(motor1.motor_handle, (uint32_t)select, eSetValueWithOverwrite);
            }
        }
        else
        {
            boton_presionado = 0; // Se apaga cuando no hay teclas presionadas
        }

        // CORREGIDO: Retardo colocado DENTRO del while(1) para no colapsar la CPU
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void vTaskSTP1(void *pvParameters)
{
    Config_motor *motor = (Config_motor *)pvParameters;
    uint32_t tecla_notificada = 0;

    while (1)
    {
        // Consultar notificaciones sin bloquear la tarea
        if (xTaskNotifyWait(0, 0, &tecla_notificada, 0) == pdTRUE) {
            // Recibe la tecla si fuera necesario
        }

        if (boton_presionado)
        {
            motor->velocidad += motor->aceleracion * DT;
            if (motor->velocidad > VELOCIDAD_MAX)
                motor->velocidad = VELOCIDAD_MAX;
        }
        else
        {
            motor->velocidad -= motor->aceleracion * DT;
            if (motor->velocidad < 0.0f)
                motor->velocidad = 0.0f;
        }

        if (motor->velocidad > 0.0f)
            delay_us = (uint32_t)(1000000.0f / motor->velocidad);

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void vTaskPulsos(void *pvParameters)
{
    Config_motor *motor = (Config_motor *)pvParameters;

    while (1)
    {
        if (motor != NULL && motor->velocidad > 0.0f)
        {
            gpio_set_level(motor->pines[0], 0); // Habilitar EN

            // Generar pulso STEP preciso de 5 us
            gpio_set_level(motor->pines[1], 1);
            esp_rom_delay_us(5);
            gpio_set_level(motor->pines[1], 0);

            // Tiempo restante del periodo del paso en microsegundos
            if (delay_us > 5) {
                esp_rom_delay_us(delay_us - 5);
            }
        }
        else
        {
            if (motor != NULL) {
                gpio_set_level(motor->pines[0], 1); // Deshabilitar EN en reposo
            }
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}

esp_err_t crear_tareas(void)
{
    xTaskCreatePinnedToCore(vTaskSTP1,
                            "vTaskSTP1",
                            stack_size,
                            (void *)&motor1, // CORREGIDO: Pasamos la estructura del motor1
                            1,
                            &motor1.motor_handle,
                            tskNO_AFFINITY);

    xTaskCreatePinnedToCore(vTaskPulsos,
                            "vTaskPulsos",
                            stack_size,
                            (void *)&motor1, // CORREGIDO: Pasamos la estructura del motor1
                            1,
                            NULL,
                            tskNO_AFFINITY);

    xTaskCreatePinnedToCore(vTaskTeclado,
                            "vTaskTeclado",
                            stack_size,
                            NULL,
                            5,
                            NULL,
                            tskNO_AFFINITY);

    return ESP_OK;
}