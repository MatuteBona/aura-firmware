// Logica del locker: pulso sin extension, limite por minuto, cortar al abrir, alertas del
// contrato v4.5 (intrusion, puerta_abierta, puerta_cerrada, limite_pulsos), antirrebote y
// vigencia de la orden de abrir.
#include "../main/locker_logica.h"
#include "../../../comun/tests/aserciones.h"
#include <vector>
#include <string>

// Saca toda la cola y la deja como texto: "PULSO 3000", "CORTAR", "ALERTA intrusion critical", "MEDIR"
static std::vector<std::string> acciones(locker_t *l) {
  std::vector<std::string> v;
  accion_t a;
  char buf[64];
  while (locker_sacar(l, &a)) {
    switch (a.tipo) {
    case ACC_PULSO: snprintf(buf, sizeof(buf), "PULSO %d", (int)a.valor); break;
    case ACC_CORTAR: snprintf(buf, sizeof(buf), "CORTAR"); break;
    case ACC_ALERTA: snprintf(buf, sizeof(buf), "ALERTA %s %s", a.alerta, severidad_texto(a.severidad)); break;
    case ACC_MEDIR: snprintf(buf, sizeof(buf), "MEDIR"); break;
    }
    v.push_back(buf);
  }
  return v;
}

static bool es(const std::vector<std::string> &v, std::initializer_list<const char *> e) {
  if (v.size() != e.size()) return false;
  size_t i = 0;
  for (const char *s : e) if (v[i++] != s) return false;
  return true;
}

int main() {
  locker_t l;

  // Apertura normal sin cortar al abrir: pulso, se abre durante el pulso, vence, se cierra
  locker_iniciar(&l, 0, false, 3000, 120, false);
  VERIFICAR(locker_abrir(&l, 1000) == ABRIR_OK);
  VERIFICAR(es(acciones(&l), {"PULSO 3000", "MEDIR"}));
  locker_sensor(&l, 2000, true);
  VERIFICAR(es(acciones(&l), {"MEDIR"}));                      // abierta con orden: sin alerta
  locker_tick(&l, 3999);
  VERIFICAR(acciones(&l).empty());
  locker_tick(&l, 4000);
  VERIFICAR(es(acciones(&l), {"CORTAR", "MEDIR"}));
  locker_sensor(&l, 10000, false);
  VERIFICAR(es(acciones(&l), {"MEDIR"}));                      // cerrada sin alerta previa

  // Con un pulso en curso, otro abrir NO lo extiende: ocupada
  locker_iniciar(&l, 0, false, 3000, 120, false);
  VERIFICAR(locker_abrir(&l, 0) == ABRIR_OK);
  acciones(&l);
  VERIFICAR(locker_abrir(&l, 2000) == ABRIR_OCUPADA);
  VERIFICAR(acciones(&l).empty());
  VERIFICAR(l.cerradura.vence_ms == 3000);                    // sigue venciendo cuando debia

  // Cortar al abrir: el pulso se corta apenas el sensor marca abierta
  locker_iniciar(&l, 0, false, 3000, 120, true);
  locker_abrir(&l, 0);
  acciones(&l);
  locker_sensor(&l, 1200, true);
  VERIFICAR(es(acciones(&l), {"CORTAR", "MEDIR"}));
  VERIFICAR(!locker_energizada(&l));
  locker_tick(&l, 3000);
  VERIFICAR(acciones(&l).empty());                            // ya no hay nada que vencer

  // Intrusion: se abre sin pulso. La alerta sale ANTES que la medicion
  locker_iniciar(&l, 0, false, 3000, 120, true);
  locker_sensor(&l, 5000, true);
  VERIFICAR(es(acciones(&l), {"ALERTA intrusion critical", "MEDIR"}));
  locker_sensor(&l, 8000, false);
  VERIFICAR(es(acciones(&l), {"ALERTA puerta_cerrada info", "MEDIR"}));

  // Gracia: abrir hasta 3 s despues del fin del pulso no es intrusion; despues si
  locker_iniciar(&l, 0, false, 3000, 120, false);
  locker_abrir(&l, 0);
  locker_tick(&l, 3000);
  acciones(&l);
  locker_sensor(&l, 5999, true);
  VERIFICAR(es(acciones(&l), {"MEDIR"}));
  locker_sensor(&l, 7000, false);
  acciones(&l);
  locker_sensor(&l, 9000, true);                              // 6 s despues del fin del pulso
  VERIFICAR(es(acciones(&l), {"ALERTA intrusion critical", "MEDIR"}));

  // Arrancar con la puerta abierta no es intrusion (no se sabe que paso antes)
  locker_iniciar(&l, 0, true, 3000, 120, false);
  locker_tick(&l, 1000);
  VERIFICAR(acciones(&l).empty());

  // puerta_abierta a los aviso_s, una sola vez; puerta_cerrada al cerrar
  locker_iniciar(&l, 0, false, 3000, 10, true);
  locker_abrir(&l, 0);
  locker_sensor(&l, 500, true);
  acciones(&l);
  locker_tick(&l, 10499);
  VERIFICAR(acciones(&l).empty());
  locker_tick(&l, 10500);
  VERIFICAR(es(acciones(&l), {"ALERTA puerta_abierta warning"}));
  locker_tick(&l, 20000);
  VERIFICAR(acciones(&l).empty());
  locker_sensor(&l, 30000, false);
  VERIFICAR(es(acciones(&l), {"ALERTA puerta_cerrada info", "MEDIR"}));

  // aviso_s = 0: sin aviso
  locker_iniciar(&l, 0, false, 3000, 0, true);
  locker_abrir(&l, 0);
  locker_sensor(&l, 500, true);
  acciones(&l);
  locker_tick(&l, 3600000);
  VERIFICAR(acciones(&l).empty());

  // Limite por minuto: con pulsos de 5 s, el quinto abrir dentro del minuto da limite_pulsos
  locker_iniciar(&l, 0, false, 5000, 120, false);
  int64_t t = 0;
  for (int i = 0; i < 4; i++) {
    VERIFICAR(locker_abrir(&l, t) == ABRIR_OK);
    t += 5000;
    locker_tick(&l, t);
  }
  acciones(&l);
  VERIFICAR(locker_abrir(&l, t) == ABRIR_LIMITE);
  VERIFICAR(es(acciones(&l), {"ALERTA limite_pulsos warning"}));
  VERIFICAR(!locker_energizada(&l));

  // Antirrebote: un rebote de 20 ms no cambia nada; 50 ms estables si
  antirrebote_t a;
  antirrebote_iniciar(&a, false, 0);
  VERIFICAR(!antirrebote_paso(&a, true, 10));
  VERIFICAR(!antirrebote_paso(&a, false, 30));
  VERIFICAR(!antirrebote_paso(&a, false, 200));
  VERIFICAR(!a.estable);
  VERIFICAR(!antirrebote_paso(&a, true, 300));
  VERIFICAR(!antirrebote_paso(&a, true, 349));
  VERIFICAR(antirrebote_paso(&a, true, 350));
  VERIFICAR(a.estable);

  // Vigencia de la orden de abrir
  const int64_t ahora = 1764000000;                           // nov 2025
  VERIFICAR(orden_vigente(false, 0, ahora) == ORDEN_SIN_VENCIMIENTO);
  VERIFICAR(orden_vigente(true, ahora + 60, 1000) == ORDEN_SIN_HORA);
  VERIFICAR(orden_vigente(true, ahora, ahora) == ORDEN_VENCIDA);
  VERIFICAR(orden_vigente(true, ahora + 301, ahora) == ORDEN_VENCIMIENTO_LEJANO);
  VERIFICAR(orden_vigente(true, ahora + 300, ahora) == ORDEN_OK);
  VERIFICAR(strcmp(orden_motivo(ORDEN_VENCIDA), "orden_vencida") == 0);

  // aviso_s: 0 o de 5 a 600
  VERIFICAR(aviso_valido(0) && aviso_valido(5) && aviso_valido(600));
  VERIFICAR(!aviso_valido(4) && !aviso_valido(601) && !aviso_valido(-1));

  RESUMEN();
}
