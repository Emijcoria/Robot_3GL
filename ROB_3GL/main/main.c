#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"

#define stack_size        1024 * 1
#define DT                0.001f      // 1 ms
#define ACELERACION       1000.0f     // pasos/s²
#define VELOCIDAD_MAX     3000.0f     // pasos/s

const char *TAG = "main";
float velocidad = 0.0f;
uint32_t delay_us;


esp_err_t iniciar_todo_gpio(void);
esp_err_t crear_tareas(void);

void vTaskPulsos(void *pvParameters);
void vTaskSTP1(void *pvParameters);
void vTaskTeclado(void *pvParameters);


int boton_presionado = 0;
const int STEPPER1[3] = {GPIO_NUM_2, GPIO_NUM_4, GPIO_NUM_5};
            //ENABLE       ,  pasos    , dirección 
//motor2[3] = {GPIO_NUM_2, GPIO_NUM_6, GPIO_NUM_8};
            //ENABLE       ,  pasos    , dirección             
//motor3[3] = {GPIO_NUM_2, GPIO_NUM_9, GPIO_NUM_10};
            //ENABLE       ,  pasos    , dirección 

const int tecladopin[8] = {GPIO_NUM_12};




void app_main(void)
{
 
    iniciar_todo_gpio();
    crear_tareas();

}

void vTaskTeclado(void *pvParameters)
{

    while (1)
    {

    }
        
}

void vTaskPulsos(void *pvParameters){
    while(1)
    {
        if(velocidad > 0)
        {
            gpio_set_level(GPIO_NUM_4, 1);
            ets_delay_us(3);
            gpio_set_level(GPIO_NUM_4, 0);

            ets_delay_us(delay_us);
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
                            NULL,
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

    return ESP_OK;
}



esp_err_t iniciar_todo_gpio(void)
{
    
    //MOTOR 1
    gpio_reset_pin(GPIO_NUM_2); //EN
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    gpio_reset_pin(GPIO_NUM_4); //STP
    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
    gpio_reset_pin(GPIO_NUM_5); //DIR
    gpio_set_direction(GPIO_NUM_5, GPIO_MODE_OUTPUT);

    //TECLADO
    gpio_reset_pin(GPIO_NUM_12); //PULSADOR
    gpio_set_direction(GPIO_NUM_12, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_NUM_12, GPIO_PULLUP_ONLY);


    ESP_LOGE("main", "GPIOs inicializados correctamente");

    return ESP_OK;
}

void vTaskSTP1(void *pvParameters)
{
    while(1)
    {
        if(boton_presionado)
        {
            velocidad += ACELERACION * DT;

            if(velocidad > VELOCIDAD_MAX)
                velocidad = VELOCIDAD_MAX;
        }
        else
        {
            velocidad -= ACELERACION * DT;

            if(velocidad < 0)
                velocidad = 0;
        }

        if(velocidad > 0)
            delay_us = (uint32_t)(1000000.0f / velocidad);

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}




