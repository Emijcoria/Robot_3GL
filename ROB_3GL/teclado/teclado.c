#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"

esp_err_t iniciar_teclado(void)
{
    // 4, 5, 16, 17, 18, 19, 21, 22

    const int teclaspin[8] = {
        GPIO_NUM_4,
        GPIO_NUM_5,
        GPIO_NUM_16,
        GPIO_NUM_17,
        GPIO_NUM_18,
        GPIO_NUM_19,
        GPIO_NUM_21,
        GPIO_NUM_22};

    for (int i = 0; i < 8; i++)
    {

        gpio_reset_pin(teclaspin[i]);
        gpio_set_direction(teclaspin[i], GPIO_MODE_INPUT);
        gpio_set_pull_mode(teclaspin[i], GPIO_PULLUP_ONLY);
    }

    ESP_LOGE("main", "Teclado inicializado correctamente");

    return ESP_OK;
}

esp_err_t Crear_tareas(void)
{

    vTaskCreate(
        vTaskTeclado,
        "vTaskTeclado",
        2048,
        NULL,
        5,
        NULL);

    return ESP_OK;
}

void vTaskTeclado(void *pvParameters){

    while (1){

        int32_t select = -1;

        for (int i = 0; i < 8; i++){

            if (digitalRead(teclaspin[i]) == 0){

                select = teclaspin[i];
                break; // toma el primero que encuentra
            }
        }

      if (select != -1) {
           
            xTaskNotify(xTaskMostrarMensajeHandle, (uint32_t)select, eSetValueWithOverwrite);

        }

        

    }
    vTaskDelay(pdMS_TO_TICKS(50));
}


void app_main(void){


Crear_tareas();
iniciar_teclado();}

