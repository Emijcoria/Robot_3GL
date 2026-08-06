#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"

#define stack_size 1024 * 2
#define DT 0.001f             // 1 ms
#define ACELERACION 1000.0f   // pasos/s²
#define VELOCIDAD_MAX 3000.0f // pasos/s

const char *TAG = "main";
float velocidad_actual = 0.0f;
uint32_t delay_us;

//TaskHandle_t vTaskSTP1Handle = NULL;

esp_err_t crear_tareas(void);

void vTaskInicializar(void *pvParameters);
void vTaskPulsos(void *pvParameters);
void vTaskSTP1(void *pvParameters);
void vTaskTeclado(void *pvParameters);


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
        .motor_handle = NULL};



int boton_presionado = 0;
//const int STEPPER1[3] = {GPIO_NUM_2, GPIO_NUM_4, GPIO_NUM_5};
// ENABLE       ,  pasos    , dirección
// STEPPER2[3] = {GPIO_NUM_2, GPIO_NUM_6, GPIO_NUM_8};
// ENABLE       ,  pasos    , dirección
// STEPPER3[3] = {GPIO_NUM_2, GPIO_NUM_9, GPIO_NUM_10};
// ENABLE       ,  pasos    , dirección
const int teclaspin[8] = {
    GPIO_NUM_4,
    GPIO_NUM_5,
    GPIO_NUM_16,
    GPIO_NUM_17,
    GPIO_NUM_18,
    GPIO_NUM_19,
    GPIO_NUM_21,
    GPIO_NUM_22};

void app_main(void)
{
    crear_tareas();

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
            if (motor1.motor_handle != NULL)
            {

                xTaskNotify(motor1.motor_handle, (uint32_t)select, eSetValueWithOverwrite); // Enviar señal al motor X
            }
        }
    }
    
}

void vTaskPulsos(void *pvParameters)
{
    Config_motor *motor1 = (Config_motor *)pvParameters;

    while (1)
    {



        if (motor1->velocidad > 0)
        {
            gpio_set_level(motor1->pines[0], 0); // EN

            gpio_set_level(motor1->pines[1], 1); // STP
            vTaskDelay(pdMS_TO_TICKS(3));
            gpio_set_level(motor1->pines[1], 0);

            vTaskDelay(pdMS_TO_TICKS(delay_us / 1000));

            gpio_set_level(motor1->pines[0], 1); // EN
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}

esp_err_t crear_tareas(void)
{
    xTaskCreatePinnedToCore(vTaskSTP1,
                            "vTaskSTP1",
                            stack_size,
                            NULL,
                            1,
                            &motor1.motor_handle,
                            tskNO_AFFINITY);

    xTaskCreatePinnedToCore(vTaskPulsos,
                            "vTaskPulsos",
                            stack_size,
                            NULL,
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

    xTaskCreatePinnedToCore(vTaskInicializar,
                            "vTaskInicializar",
                            stack_size,
                            NULL,
                            5,
                            NULL,
                            tskNO_AFFINITY);

    return ESP_OK;
}

void vTaskInicializar(void *pvParameters){

    Config_motor *motor1 = (Config_motor *)pvParameters;

    // MOTOR 1
    gpio_reset_pin(motor1->pines[0]); // EN
    gpio_set_direction(motor1->pines[0], GPIO_MODE_OUTPUT);
    gpio_reset_pin(motor1->pines[1]); // STP
    gpio_set_direction(motor1->pines[1], GPIO_MODE_OUTPUT);
    gpio_reset_pin(motor1->pines[2]); // DIR
    gpio_set_direction(motor1->pines[2], GPIO_MODE_OUTPUT);

    // 4, 5, 16, 17, 18, 19, 21, 22 pines para el teclado

    for (int i = 0; i < 8; i++)
    {

        gpio_reset_pin(teclaspin[i]);
        gpio_set_direction(teclaspin[i], GPIO_MODE_INPUT);
        gpio_set_pull_mode(teclaspin[i], GPIO_PULLUP_ONLY);
    }

    ESP_LOGE("main", "Teclado inicializado correctamente");

    ESP_LOGE("main", "GPIOs inicializados correctamente");

}

void vTaskSTP1(void *pvParameters)
{


    while (1)
    {

        if(boton_presionado)
             {
                 motor1.velocidad += ACELERACION * DT;

                 if(motor1.velocidad > VELOCIDAD_MAX)
                     motor1.velocidad = VELOCIDAD_MAX;
             }
             else
             {
                 motor1.velocidad -= ACELERACION * DT;

                 if(motor1.velocidad < 0)
                     motor1.velocidad = 0;
             }

             if(motor1.velocidad > 0)
                 delay_us = (uint32_t)(1000000.0f / motor1.velocidad);

             vTaskDelay(pdMS_TO_TICKS(1));
         
        
    }
}
