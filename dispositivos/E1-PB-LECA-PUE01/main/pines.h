#pragma once
/*
 * Pines de la ESP32-C3 SuperMini. GPIO2, 8 y 9 son de arranque: no se usan para la potencia.
 * Conexionado completo en la ficha (README.md, "Hardware y conexionado").
 */
#include "driver/gpio.h"

#if !CONFIG_IDF_TARGET_ESP32C3
#error "Esta hoja es para la ESP32-C3 SuperMini: idf.py set-target esp32c3"
#endif

/* Gate del IRLZ44N por 470 ohm, con 10 kohm de gate a GND. 1 = cerradura energizada. */
#define PIN_CERRADURA    GPIO_NUM_0
/* Microswitch: COM a GND, NO al pin, con el pull-up interno. Puerta cerrada = switch apretado =
   pin en 0. Un cable cortado se lee como 1 = abierta, que es el estado seguro. */
#define PIN_MICROSWITCH  GPIO_NUM_5
/* LED azul de la SuperMini, activo en bajo */
#define PIN_LED          GPIO_NUM_8
