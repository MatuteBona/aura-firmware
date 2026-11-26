#pragma once
/*
 * Salida de la cerradura: lo unico que toca el pin del gate del MOSFET. La cerradura esta TRABADA
 * sin corriente y se destraba solo mientras el MOSFET conduce (ADR-009, decision 1).
 *
 * Igual que rele.c del modelo _modelos/PUE-RFID: el corte de cada pulso lo hace un esp_timer
 * propio, que NO depende de la tarea de la logica. Si la logica se cuelga, el temporizador igual
 * corta; si se cuelga todo, el watchdog reinicia la placa y el pin vuelve a 0 (con el pull-down de
 * 10 kohm del gate, el MOSFET queda apagado tambien durante el arranque). Ademas impone por su
 * cuenta el tope duro de cada pulso.
 */
#include <stdbool.h>
#include <stdint.h>

/* LO PRIMERO que se hace al arrancar: el pin en 0 antes de configurarlo como salida. */
void salida_cerradura_reposo_inicial(void);

/* Crea el temporizador de corte. Despues de salida_cerradura_reposo_inicial(). */
void salida_cerradura_iniciar(void);

/* Destraba por `ms` (como mucho PULSO_TOPE_MS). El temporizador la vuelve a trabar solo. */
void salida_cerradura_energizar(uint32_t ms);

/* Traba ya. */
void salida_cerradura_soltar(void);

bool salida_cerradura_energizada(void);
