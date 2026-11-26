# E1-PB-LECA-LCK01 — gabinete de lockers del pañol (LabECA)

| | |
|---|---|
| **Ubicación** | Edificio 1, planta baja, LabECA — gabinete de lockers del pañol |
| **Responsable** | Grupo de lockers del pañol, IC III 2026: MatuteBona |
| **Estado** | en desarrollo |
| **Instalado** | todavía no |
| **Transporte** | mesh ESP-WIFI-MESH (interior), en ESP-IDF 6.1, según [ADR-007](../../docs/adr/ADR-007-mesh-en-esp-idf-con-concentrador-por-aula.md); actualización por la malla según ADR-008 |
| **Contrato que implementa** | [`CONTRATO_MQTT.md`](../../docs/CONTRATO_MQTT.md) v4.3 |
| **Basado en** | [`Mesh_Red_Completa/Mesh3_Hoja`](../../Mesh_Red_Completa/Mesh3_Hoja/), sobre la biblioteca `nodo_mesh`, más el componente propio `components/puertas` |
| **Firmware** | tipo `aura-lockers`, versión `1.0.1` |
| **Códigos anteriores** | ninguno |

## Qué hace

Controla la cerradura de un locker del pañol automatizado: cuando AURA le manda `abrir`,
energiza el solenoide de la cerradura durante el tiempo pedido (15 s en el pañol), y en cada
cambio informa si la puerta está abierta o cerrada (microswitch) y si el solenoide está
energizado. Avisa con una alerta si la puerta se abre sin que se haya mandado la apertura
(forzada) o si queda abierta demasiado tiempo.

Las reglas del préstamo (reservas, código de apertura, ventana de retiro, mora) no viven en la
placa: las aplica el sistema de lockers del grupo, que pide la apertura a través de AURA. El
conector está en [`conectores/E1-PB-LECA-LCK01/`](../../conectores/E1-PB-LECA-LCK01/).

## Hardware y conexionado

| Componente | Modelo | Conexión |
|---|---|---|
| Placa | ESP32-C3 SuperMini (4 MB) | — |
| Llave de potencia | MOSFET IRLZ44N (canal N, nivel lógico, TO-220) | Gate ← GPIO0 por una resistencia de **470 Ω**; **10 kΩ entre gate y source**; source → −V de la fuente |
| Cerradura | Solenoide de 12 V, 5,4 Ω medidos (≈ 2,2 A, ≈ 27 W mientras está energizado) | +12 V → cerradura → drain del MOSFET |
| Diodo de rueda libre | 1N4007 | En paralelo con la cerradura: cátodo (franja) al +12 V, ánodo al drain |
| Sensor de puerta | Microswitch KW12-3 | COM → GND, NO → GPIO5 (pull-up interno). NC libre |
| Fuente de la cerradura | Switching 12 V / 5 A | +V → cerradura y cátodo del diodo; −V → source del MOSFET |
| Alimentación de la placa | 5 V por USB, o step-down 12 → 5 V al pin 5V | Nunca 12 V a la placa |
| LED de estado | El de la SuperMini (GPIO8, activo en bajo) | Fijo: en la malla con aula · 1 Hz: sin aula · 4 Hz: buscando la malla |

Tres cosas que salieron en el banco y no son opcionales:

- **Masa común:** el GND de la placa va unido al −V de la fuente de 12 V (al source del
  MOSFET) con un cable directo. Sin ella, la cerradura no abre y el microswitch lee la puerta
  abierta en el momento del pulso.
- **Pull-down de 10 kΩ en el gate:** sin él, mientras la placa arranca el gate queda flotando,
  el MOSFET conduce a medias (calienta) y la cerradura se destraba sola.
- **Resistencia en serie de 470 Ω (no de 10 kΩ):** con 10 kΩ en serie y 10 kΩ a GND, al gate
  llegan 1,6 V y el MOSFET no conduce. Con 470 Ω llegan 3,15 V.

El camino de potencia (2,2 A) va por cables o borneras, no por las filas de un protoboard.

## Lo que envía a AURA

Cada medición es un `values` (contrato §3.1). Sale una **en cada cambio** del microswitch o del
solenoide, y una completa cada `intervalo_s`:

```json
{"puerta_1":0,"solenoide_1":1}
```

| Campo | Unidad | Rango válido | Se envía |
|---|---|---|---|
| `puerta_1` | estado (1 = abierta, 0 = cerrada) | 0 o 1 | en cada cambio y cada `intervalo_s` |
| `solenoide_1` | estado (1 = energizado, 0 = trabado) | 0 o 1 | en cada cambio y cada `intervalo_s` |

Un microswitch desconectado se lee como abierto, que es el estado seguro.

**Alertas** (contrato §3.5), tipo `puerta`, una por cambio, con `details.puerta` y `details.motivo`:

| `details.motivo` | `severity` | Cuándo |
|---|---|---|
| `apertura_sin_orden` | `high` | la puerta se abrió sin que el solenoide estuviera energizado (forzada o traba floja); hay 3 s de gracia después de cada pulso |
| `abierta_demasiado` | `warning` | lleva más de `max_abierta_s` abierta (`details.segundos`) |
| `cerrada` | `info` | se cerró una puerta que tenía alguna de las dos alertas anteriores |

## Comandos

| Comando | `params` | Qué hace | Rechazos (`details.motivo`) |
|---|---|---|---|
| `abrir` | `puerta` (1, obligatorio), `ms` (100 a 20 000; por defecto 15 000) | Energiza el solenoide de esa puerta durante `ms`. Si ya está energizada, extiende el pulso | `puerta_fuera_de_rango`, `ms_fuera_de_rango`, `puertas_ocupadas`, `orden_local_no_permitida`, `comando_desconocido` |

Ejemplo: `{"command":"abrir","params":{"puerta":1,"ms":15000},"command_id":"..."}` (menos de
180 bytes).

- **El botón del concentrador no abre puertas:** una orden `local` de `abrir` se rechaza.
- **Protección de la bobina:** ningún pulso pasa de 20 s; el componente `puertas` lo corta al
  vencer aunque se pierda la comunicación.

## Configuración

Parámetros que acepta `set_config` (contrato §3.3). Se validan todos antes de aplicar alguno y se
guardan en NVS. `aula` la maneja la biblioteca. Un `intervalo_s` nuevo rige enseguida: la placa
mide en ese momento.

| Parámetro | Rango | Valor en esta placa |
|---|---|---|
| `intervalo_s` | 5 a 3600 (entero) | 30 |
| `max_abierta_s` | 30 a 3600 (entero) | 120 |

`describir_config` informa además `puertas: 1`. Un parámetro desconocido se rechaza con
`parametro_desconocido`; uno fuera de rango, con `<parámetro>_fuera_de_rango`, y la
configuración queda como estaba.

## Compilar, firmar y grabar (banco)

Desde la terminal de ESP-IDF 6.1, en `dispositivos\E1-PB-LECA-LCK01`, con `main\config_local.h`
copiado de `config_local.ejemplo.h` y completado con los datos de la mesh del banco:

```
idf.py set-target esp32c3
idf.py build
python ..\..\Mesh_Red_Completa\Broker_MQTT\firmar.py --clave C:\aura-claves\firma_prueba.pem build\aura-lockers.bin
idf.py -p COMx erase-flash      # solo en la primera grabación
idf.py -p COMx flash monitor
```

La imagen entra con un 39 % libre en los 1,75 MB de `ota_0`.

## Pruebas

Los informes están en [`pruebas/`](pruebas/). Ver `pruebas/2026-11-06-banco/` para lo probado
en el banco y lo que falta.
