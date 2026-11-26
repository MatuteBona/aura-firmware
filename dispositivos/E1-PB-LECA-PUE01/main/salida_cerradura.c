#include "salida_cerradura.h"
#include "cerradura_pulso.h"
#include "pines.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"

static const char *TAG = "CERRADURA";

/* Si la logica corta justo en el borde, el corte de hardware llega apenas despues del de ella */
#define MARGEN_CORTE_MS 150

static esp_timer_handle_t temporizador;
static volatile bool energizada = false;

void salida_cerradura_reposo_inicial(void)
{
    /* El nivel se escribe ANTES de pasar el pin a salida: sale directo en reposo */
    gpio_set_level(PIN_CERRADURA, 0);
    gpio_set_direction(PIN_CERRADURA, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_CERRADURA, 0);
    energizada = false;
}

static void al_vencer(void *arg)
{
    (void)arg;
    gpio_set_level(PIN_CERRADURA, 0);
    energizada = false;
    ESP_EARLY_LOGW(TAG, "Corte por temporizador: cerradura trabada");
}

void salida_cerradura_iniciar(void)
{
    const esp_timer_create_args_t args = {
        .callback = al_vencer,
        .name = "corte_cerradura",
        .dispatch_method = ESP_TIMER_TASK,
    };
    ESP_ERROR_CHECK(esp_timer_create(&args, &temporizador));
}

void salida_cerradura_energizar(uint32_t ms)
{
    if (ms > PULSO_TOPE_MS) ms = PULSO_TOPE_MS;       /* tope duro, pase lo que pase arriba */
    if (ms == 0) return;
    esp_timer_stop(temporizador);                      /* ESP_ERR_INVALID_STATE si no corria: no importa */
    /* Primero se arma el corte y despues se energiza: nunca hay un pulso sin temporizador */
    ESP_ERROR_CHECK(esp_timer_start_once(temporizador, (uint64_t)(ms + MARGEN_CORTE_MS) * 1000));
    gpio_set_level(PIN_CERRADURA, 1);
    energizada = true;
}

void salida_cerradura_soltar(void)
{
    gpio_set_level(PIN_CERRADURA, 0);
    energizada = false;
    esp_timer_stop(temporizador);
}

bool salida_cerradura_energizada(void)
{
    return energizada;
}
