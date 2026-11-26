# E1-PB-LECA-PUE01 — locker del pañol (LabECA)

| | |
|---|---|
| **Ubicación** | Edificio 1, planta baja, LabECA — gabinete de lockers del pañol, locker 1 |
| **Responsable** | Grupo de lockers del pañol, IC III 2026: MatuteBona, agusferrero05, bertolaco |
| **Estado** | en desarrollo |
| **Instalado** | todavía no |
| **Transporte** | mesh ESP-WIFI-MESH (interior), en ESP-IDF 6.1, según [ADR-007](../../docs/adr/ADR-007-mesh-en-esp-idf-con-concentrador-por-aula.md); actualización por la malla según ADR-008 |
| **Contrato que implementa** | [`CONTRATO_MQTT.md`](../../docs/CONTRATO_MQTT.md) v4.5: `abrir`, `estado` y las alertas de la puerta (§3.3 y §3.5) |
| **Decisiones** | [ADR-009](../../docs/adr/ADR-009-puertas-con-llavero-la-puerta-decide-y-aura-supervisa.md): un locker es una puerta con cerradura por pulso y sensor de apertura |
| **Basado en** | [`Mesh_Red_Completa/Mesh3_Hoja`](../../Mesh_Red_Completa/Mesh3_Hoja/), sobre la biblioteca `nodo_mesh`. El pulso (`cerradura_pulso.h`), la salida con corte por temporizador y la validación de `set_config` vienen del modelo [`_modelos/PUE-RFID`](../_modelos/PUE-RFID/) |
| **Firmware** | tipo `aura-lockers`, versión `1.1.0` |
| **Códigos anteriores** | `E1-PB-LECA-LCK01` (solo en el PR, nunca instalado) |

## Qué hace

Supervisa un locker del pañol automatizado: informa en cada cambio si la puerta está abierta o
cerrada (microswitch) y si la cerradura está energizada, y manda alertas si el locker se abre sin
orden (`intrusion`), si queda abierto demasiado tiempo (`puerta_abierta`) y cuando se cierra
después de alguna de las dos (`puerta_cerrada`).

**La apertura remota está deshabilitada** (ADR-009, decisión 8). Las reglas del préstamo
(reservas, código de apertura, ventana de retiro, mora) viven en el sistema de lockers del grupo,
que pediría la apertura a través de AURA. Hoy AURA manda comandos solo desde la sesión de una
persona y no hay una cuenta para que otro sistema los mande, y la mesh todavía no autentica los
comandos (etapa 2 de seguridad). Mientras tanto, el firmware se compila con `abrir` apagado: se
rechaza con `abrir_deshabilitado`, y el dispositivo solo informa el estado y manda las alertas.
La lógica de `abrir` (con vencimiento, tope y límite por minuto) está escrita y probada, y se
enciende solo en el banco (ver [Opciones al compilar](#opciones-al-compilar)).

### Cuántos lockers y cuántas placas

El gabinete del pañol tiene **4 lockers**, y va **una placa por locker**: cada uno es una puerta,
con su código (`E1-PB-LECA-PUE01` a `PUE04`), su cerradura y su microswitch. Todas llevan este mismo binario. Por ahora está armado y
probado el locker 1; los otros tres se dan de alta con su carpeta cuando se monten.

## Hardware y conexionado

| Componente | Modelo | Conexión |
|---|---|---|
| Placa | ESP32-C3 SuperMini (4 MB) | — |
| Llave de potencia | MOSFET IRLZ44N (canal N, nivel lógico, TO-220) | Gate ← GPIO0 por una resistencia de **470 Ω**; **10 kΩ entre gate y source**; source → −V de la fuente |
| Cerradura | [DFRobot FIT0620](https://wiki.dfrobot.com/Electric%20Solenoid%20Lock%20SKU:%20FIT0620), 9 a 12 V, 1,5 A nominales (en el banco se midieron 5,4 Ω: unos 2,2 A y 27 W a 12 V). Traba biselada | +12 V → cerradura → drain del MOSFET |
| Diodo de rueda libre | 1N4007 | En paralelo con la cerradura: cátodo (franja) al +12 V, ánodo al drain |
| Sensor de puerta | Microswitch KW12-3 | COM → GND, NO → GPIO5 (pull-up interno). NC libre. Puerta cerrada = switch apretado |
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

### Protección de la bobina

El fabricante de la FIT0620 pide **no energizarla más de 5 s seguidos** (la hoja de datos no da un
ciclo de trabajo). Por eso:

| | Valor | Dónde |
|---|---|---|
| Tope de un pulso | **5 s**, ni por `set_config` ni por una orden | `cerradura_pulso.h` (`PULSO_TOPE_MS`) y, aparte, `salida_cerradura.c` |
| Pulso por defecto | 3 s (`pulso_ms`, de 0,5 a 5 s) | `set_config` |
| Límite por minuto | 20 s energizada en cualquier ventana de 60 s (ADR-009, decisión 1); al llegar, alerta `limite_pulsos` | `cerradura_pulso.h` |
| Orden con un pulso en curso | **no lo extiende**: se contesta `ocupada` | `locker_logica.h` |
| Corte del pulso | un `esp_timer` propio, que no depende de la tarea de la lógica; la tarea está suscrita al watchdog (5 s, con reinicio) | `salida_cerradura.c`, `sdkconfig.defaults` |
| Cortar al abrir | apenas el microswitch marca abierta, se corta el pulso (la traba es biselada: la puerta cierra sola con la traba afuera) | Kconfig, encendido por defecto |

## Lo que envía a AURA

Cada medición es un `values` (contrato §3.1). Sale una **en cada cambio** de la puerta o de la
cerradura, y una completa cada `intervalo_s`:

```json
{"puerta":0,"cerradura":0}
```

| Campo | Unidad | Rango válido | Se envía |
|---|---|---|---|
| `puerta` | estado (1 = abierta, 0 = cerrada) | 0 o 1 | en cada cambio y cada `intervalo_s` |
| `cerradura` | estado (1 = energizada, 0 = trabada) | 0 o 1 | en cada cambio y cada `intervalo_s` |

Un microswitch desconectado se lee como abierto, que es el estado seguro. Son los mismos nombres
que usa el latido del modelo `PUE-RFID`.

En el reporte (`status`, §3.2), `config` trae además `abierta` (0 o 1), de solo lectura.

**Alertas** (contrato v4.5, §3.5, "Alertas de la puerta con llavero"), una por cambio:

| Tipo | `severity` | Cuándo |
|---|---|---|
| `intrusion` | `critical` | la puerta se abrió sin un pulso en curso y fuera de los 3 s de gracia que siguen a cada pulso (la traba tarda en volver). Sale antes que cualquier otro mensaje del mismo cambio |
| `puerta_abierta` | `warning` | quedó abierta más de `aviso_s` |
| `puerta_cerrada` | `info` | se cerró después de una `intrusion` o de un `puerta_abierta` |
| `limite_pulsos` | `warning` | una orden de abrir no se cumplió porque se llegó a los 20 s energizada en el minuto |

**Por qué `intrusion` es `critical`:** con la apertura remota deshabilitada, ninguna apertura del
locker es legítima desde el punto de vista de la placa, y un locker abierto sin orden es una
herramienta del pañol que se puede llevar cualquiera. Es el mismo caso que una puerta armada que se
fuerza, y AURA lo tiene que tratar igual. Arrancar con la puerta abierta no es intrusión: no se sabe
qué pasó antes del arranque.

## Comandos

Los de la puerta del contrato v4.5 (§3.3, "Comandos de la puerta con llavero"), con los mismos
nombres de `params` y de motivos. Los de llaveros (`tarjeta_*`) no aplican: el locker no tiene lector.

| Comando | `params` | Qué hace | Rechazos (`details.motivo`) |
|---|---|---|---|
| `abrir` | `{"vence": <segundos Unix>}` | **Deshabilitado al compilar:** siempre `abrir_deshabilitado`. Con `CONFIG_LOCKER_ABRIR_REMOTO` (solo banco): un pulso de `pulso_ms` | `abrir_deshabilitado`; habilitado: `sin_vencimiento`, `sin_hora_valida`, `orden_vencida`, `vencimiento_muy_lejano` (más de 300 s), `ocupada` (pulso en curso o límite por minuto) |
| `estado` | `{}` | De solo lectura: manda el reporte y una medición en el momento | — |

- **El botón del concentrador no toca el locker:** una orden `local` que no sea `estado` se
  rechaza con `orden_local_no_admitida`.
- Un comando que no es ninguno de estos: `comando_desconocido`.

Ejemplo, en el banco con la opción encendida:
`{"command":"abrir","params":{"vence":1764163200},"command_id":"..."}`.

## Configuración

Parámetros que acepta `set_config` (contrato §3.3). Tienen que ser enteros, se validan todos
antes de aplicar alguno y se guardan en NVS en un solo bloque. `aula` la maneja la biblioteca.

| Parámetro | Rango | Valor por defecto |
|---|---|---|
| `intervalo_s` | 5 a 3600 | 30 |
| `pulso_ms` | 500 a 5000 (el tope de la FIT0620) | 3000 |
| `aviso_s` | `0` (sin aviso) o 5 a 600 | 120 |

Un parámetro desconocido se rechaza con `parametro_desconocido`; uno fuera de rango o no entero
(`150.7`), con `<parámetro>_fuera_de_rango`; un `params` que no es un objeto, con
`params_invalido`. En todos los casos la configuración queda como estaba. Un `intervalo_s` nuevo
rige enseguida.

## Opciones al compilar

En `idf.py menuconfig` → *Locker del panol (E1-PB-LECA-PUE01)*:

| Opción | Por defecto | |
|---|---|---|
| `CONFIG_LOCKER_ABRIR_REMOTO` | **apagada** | Con la opción apagada, `abrir` se rechaza con `abrir_deshabilitado` (ADR-009, decisión 8). Se enciende **solo en el banco** |
| `CONFIG_LOCKER_CORTAR_AL_ABRIR` | encendida | Corta el pulso apenas el microswitch marca abierta |

Para el banco hay un archivo aparte, `sdkconfig.banco`, que enciende la apertura remota en una
carpeta de compilación separada, sin tocar el `sdkconfig` normal:

```
idf.py -B build-banco -D SDKCONFIG=build-banco/sdkconfig -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.banco" build
```

## Compilar, firmar y grabar (banco)

Desde la terminal de ESP-IDF 6.1, en `dispositivos\E1-PB-LECA-PUE01`, con `main\config_local.h`
copiado de `config_local.ejemplo.h` y completado con los datos de la mesh del banco:

```
idf.py set-target esp32c3
idf.py build
python ..\..\Mesh_Red_Completa\Broker_MQTT\firmar.py --clave C:\aura-claves\firma_prueba.pem build\aura-lockers.bin
idf.py -p COMx erase-flash      # solo en la primera grabación
idf.py -p COMx flash monitor
```

La imagen entra con un 39 % libre en los 1,75 MB de `ota_0`.

## Tests

La lógica del locker está en headers de C puro y se prueba en la compu (el CI los corre):

```
make -C dispositivos/E1-PB-LECA-PUE01/tests test
```

| Test | Qué prueba |
|---|---|
| `test_locker` | pulso sin extensión (`ocupada`), cortar al abrir, `intrusion` antes que la medición, gracia de 3 s, `puerta_abierta` una sola vez y `puerta_cerrada`, `aviso_s: 0`, `limite_pulsos`, antirrebote, vigencia de `abrir` |
| `test_cerradura` | tope de 5 s y límite de 20 s por minuto con pulsos de la FIT0620 |
| `test_config` | enteros dentro de rango (`150.7` no pasa) |

## Pruebas

Los informes están en [`pruebas/`](pruebas/).
