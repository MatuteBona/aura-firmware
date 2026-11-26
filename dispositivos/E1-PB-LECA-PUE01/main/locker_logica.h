#pragma once
/*
 * Logica del locker (C puro, se prueba en la compu: tests/test_locker.cpp).
 *
 * Un locker es una puerta con cerradura por pulso y sensor de apertura (ADR-009). Esta logica
 * no toca pines ni la red: recibe lo que pasa (el sensor cambio, llego una orden de abrir,
 * paso el tiempo) y deja en una cola lo que hay que hacer (energizar, cortar, mandar una alerta
 * o una medicion). main.c ejecuta esa cola en orden.
 *
 *  - Abrir: un pulso de pulso_ms (con el tope y el limite por minuto de cerradura_pulso.h). Con
 *    un pulso en curso NO se extiende: se contesta "ocupada". Si el limite por minuto no deja,
 *    tambien "ocupada", y sale la alerta limite_pulsos.
 *  - Cortar al abrir (opcional, Kconfig): apenas el sensor marca abierta, se corta el pulso.
 *    La traba de la FIT0620 es biselada: la puerta cierra sola con la traba afuera, asi que no
 *    hace falta seguir energizando.
 *  - intrusion (critical): la puerta se abrio sin pulso en curso y fuera de la gracia que sigue
 *    a cada pulso (la traba tarda en volver y la persona puede estar tirando justo al final).
 *    Sale ANTES que cualquier otro mensaje del cambio (contrato v4.5, 3.5).
 *  - puerta_abierta (warning): abierta mas de aviso_s (0 = sin aviso).
 *  - puerta_cerrada (info): se cerro despues de una intrusion o de estar abierta de mas.
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "cerradura_pulso.h"

#define GRACIA_APERTURA_MS 3000
#define ANTIRREBOTE_MS     50
#define ACCIONES_MAX       8

/* pulso_ms y aviso_s de set_config (contrato v4.5, 3.3) */
#define PULSO_DEFECTO_MS   3000
#define AVISO_DEFECTO_S    120
#define AVISO_MIN_S        5               /* 0 = sin aviso; si no, de 5 a 600 */
#define AVISO_MAX_S        600

typedef enum { SEV_INFO, SEV_WARNING, SEV_HIGH, SEV_CRITICAL } severidad_t;

static inline const char *severidad_texto(severidad_t s)
{
    static const char *const t[] = {"info", "warning", "high", "critical"};
    return t[s];
}

typedef enum {
    ACC_PULSO,      /* valor = ms concedidos */
    ACC_CORTAR,
    ACC_ALERTA,     /* tipo, severidad y mensaje */
    ACC_MEDIR,      /* cambio la puerta o la cerradura: medicion en el acto */
} accion_tipo_t;

typedef struct {
    accion_tipo_t tipo;
    int32_t valor;
    const char *alerta;
    severidad_t severidad;
    const char *mensaje;
} accion_t;

typedef enum { ABRIR_OK, ABRIR_OCUPADA, ABRIR_LIMITE } abrir_resultado_t;

typedef struct {
    cerradura_t cerradura;
    bool abierta;
    int64_t abierta_desde_ms;
    bool hubo_pulso;
    int64_t fin_pulso_ms;
    bool intrusion;              /* hubo intrusion y la puerta todavia no se cerro */
    bool aviso_alertado;         /* ya salio puerta_abierta en esta apertura */
    int32_t pulso_ms;
    int32_t aviso_s;
    bool cortar_al_abrir;
    accion_t cola[ACCIONES_MAX];
    uint8_t cabeza, cantidad;
} locker_t;

/* ---------- cola de acciones ---------- */
static inline void l_push(locker_t *l, accion_t a)
{
    if (l->cantidad == ACCIONES_MAX) {
        /* No deberia pasar: main la vacia en cada vuelta. Se pierde la mas vieja. */
        l->cabeza = (uint8_t)((l->cabeza + 1) % ACCIONES_MAX);
        l->cantidad--;
    }
    l->cola[(l->cabeza + l->cantidad) % ACCIONES_MAX] = a;
    l->cantidad++;
}

static inline void l_simple(locker_t *l, accion_tipo_t tipo, int32_t valor)
{
    accion_t a = {tipo, valor, NULL, SEV_INFO, NULL};
    l_push(l, a);
}

static inline void l_alerta(locker_t *l, const char *tipo, severidad_t sev, const char *mensaje)
{
    accion_t a = {ACC_ALERTA, 0, tipo, sev, mensaje};
    l_push(l, a);
}

static inline bool locker_sacar(locker_t *l, accion_t *a)
{
    if (l->cantidad == 0) return false;
    *a = l->cola[l->cabeza];
    l->cabeza = (uint8_t)((l->cabeza + 1) % ACCIONES_MAX);
    l->cantidad--;
    return true;
}

/* ---------- estado ---------- */
static inline void locker_iniciar(locker_t *l, int64_t ahora_ms, bool abierta, int32_t pulso_ms,
                                  int32_t aviso_s, bool cortar_al_abrir)
{
    memset(l, 0, sizeof(*l));
    cerradura_iniciar(&l->cerradura);
    l->abierta = abierta;                /* lo de antes del arranque no se sabe: sin intrusion */
    l->abierta_desde_ms = ahora_ms;
    l->pulso_ms = pulso_ms;
    l->aviso_s = aviso_s;
    l->cortar_al_abrir = cortar_al_abrir;
}

static inline bool locker_energizada(const locker_t *l)
{
    return l->cerradura.energizada;
}

static inline void l_cortar(locker_t *l, int64_t ahora_ms)
{
    if (!l->cerradura.energizada) return;
    cerradura_cortar(&l->cerradura, ahora_ms);
    l->hubo_pulso = true;
    l->fin_pulso_ms = ahora_ms;
    l_simple(l, ACC_CORTAR, 0);
}

/* Orden de abrir ya validada (vencimiento y hora): decide si hay pulso. */
static inline abrir_resultado_t locker_abrir(locker_t *l, int64_t ahora_ms)
{
    if (l->cerradura.energizada) return ABRIR_OCUPADA;          /* no se extiende el pulso */
    int64_t concedido = cerradura_pedir(&l->cerradura, ahora_ms, l->pulso_ms, l->pulso_ms);
    if (concedido <= 0) {
        l_alerta(l, "limite_pulsos", SEV_WARNING, "Se llego al limite de tiempo energizada por minuto");
        return ABRIR_LIMITE;
    }
    l_simple(l, ACC_PULSO, (int32_t)concedido);
    l_simple(l, ACC_MEDIR, 0);
    return ABRIR_OK;
}

/* El sensor ya filtrado (antirrebote) cambio, o no. */
static inline void locker_sensor(locker_t *l, int64_t ahora_ms, bool abierta)
{
    if (abierta == l->abierta) return;
    l->abierta = abierta;

    if (abierta) {
        l->abierta_desde_ms = ahora_ms;
        l->aviso_alertado = false;
        bool con_orden = l->cerradura.energizada ||
                         (l->hubo_pulso && (ahora_ms - l->fin_pulso_ms) < GRACIA_APERTURA_MS);
        if (!con_orden) {
            l->intrusion = true;
            l_alerta(l, "intrusion", SEV_CRITICAL, "Locker abierto sin orden de apertura");
        } else if (l->cortar_al_abrir) {
            l_cortar(l, ahora_ms);
        }
    } else if (l->intrusion || l->aviso_alertado) {
        l->intrusion = false;
        l->aviso_alertado = false;
        l_alerta(l, "puerta_cerrada", SEV_INFO, "Locker cerrado");
    }
    l_simple(l, ACC_MEDIR, 0);
}

/* El paso del tiempo: vence el pulso y el aviso de puerta abierta. */
static inline void locker_tick(locker_t *l, int64_t ahora_ms)
{
    if (cerradura_vencio(&l->cerradura, ahora_ms)) {
        l_cortar(l, ahora_ms);
        l_simple(l, ACC_MEDIR, 0);
    }
    if (l->abierta && l->aviso_s > 0 && !l->aviso_alertado &&
        (ahora_ms - l->abierta_desde_ms) >= (int64_t)l->aviso_s * 1000) {
        l->aviso_alertado = true;
        l_alerta(l, "puerta_abierta", SEV_WARNING, "Locker abierto demasiado tiempo");
    }
}

/* ---------- antirrebote del microswitch ---------- */
typedef struct {
    bool estable;
    bool crudo;
    int64_t desde_ms;
} antirrebote_t;

static inline void antirrebote_iniciar(antirrebote_t *a, bool lectura, int64_t ahora_ms)
{
    a->estable = a->crudo = lectura;
    a->desde_ms = ahora_ms;
}

/* true si el valor estable cambio (quedo ANTIRREBOTE_MS sin moverse). */
static inline bool antirrebote_paso(antirrebote_t *a, bool lectura, int64_t ahora_ms)
{
    if (lectura != a->crudo) {
        a->crudo = lectura;
        a->desde_ms = ahora_ms;
        return false;
    }
    if (a->crudo != a->estable && (ahora_ms - a->desde_ms) >= ANTIRREBOTE_MS) {
        a->estable = a->crudo;
        return true;
    }
    return false;
}

/* ---------- vigencia de la orden de abrir (igual que _modelos/PUE-RFID) ----------
 * `abrir` llega con un vencimiento (epoch en segundos). Se rechaza sin hora valida (no se puede
 * saber si vencio), vencida, o con un vencimiento a mas de 300 s: una orden que quedo encolada
 * en el broker no se tiene que poder ejecutar horas despues. */
#define HORA_VALIDA_DESDE_S 1735689600LL    /* 2025-01-01 */
#define ORDEN_VIGENCIA_MAX_S 300

typedef enum {
    ORDEN_OK = 0,
    ORDEN_SIN_VENCIMIENTO,
    ORDEN_SIN_HORA,
    ORDEN_VENCIDA,
    ORDEN_VENCIMIENTO_LEJANO
} orden_estado_t;

static inline orden_estado_t orden_vigente(bool tiene_vence, int64_t vence_s, int64_t ahora_s)
{
    if (!tiene_vence) return ORDEN_SIN_VENCIMIENTO;
    if (ahora_s < HORA_VALIDA_DESDE_S) return ORDEN_SIN_HORA;
    if (vence_s <= ahora_s) return ORDEN_VENCIDA;
    if (vence_s - ahora_s > ORDEN_VIGENCIA_MAX_S) return ORDEN_VENCIMIENTO_LEJANO;
    return ORDEN_OK;
}

static inline const char *orden_motivo(orden_estado_t o)
{
    static const char *const m[] = {"", "sin_vencimiento", "sin_hora_valida", "orden_vencida",
                                    "vencimiento_muy_lejano"};
    return m[o];
}

/* aviso_s: 0 (sin aviso) o de AVISO_MIN_S a AVISO_MAX_S */
static inline bool aviso_valido(int32_t v)
{
    return v == 0 || (v >= AVISO_MIN_S && v <= AVISO_MAX_S);
}
