# Prueba en placa: el locker en el banco de la mesh (2026-11-06)

Grupo de lockers del pañol, IC III 2026 (@MatuteBona, @agusferrero05, @bertolaco).

Primera vez que la hoja del locker entró a una malla: el banco de la materia en el LabECA
(gateway y concentrador de la cátedra, y `broker.py` con su tablero en una notebook del
laboratorio). **En el banco no hay AURA:** lo que se describe abajo como `persistido` y el comando
`abrir` son de `broker.py` y de su tablero.

La hoja corrió la **versión anterior** de este firmware (`aura-lockers` 1.0.1, cuando el
dispositivo se llamaba `E1-PB-LECA-LCK01`), firmada con la clave de prueba del grupo, sobre una
ESP32-C3 SuperMini conectada al hardware de un locker (cerradura de 12 V con IRLZ44N y
microswitch, ver la ficha). Esa versión mandaba `{"puerta_1":…,"solenoide_1":…}` y aceptaba un
`abrir` con `puerta` y `ms`; la 1.1.0 los reemplazó por lo que define el contrato v4.5. **Por eso
la prueba 1 se repite con la 1.1.0** (ver [Lo que falta](#lo-que-falta)).

## El banco

| Placa | Hardware | Firmware |
|---|---|---|
| Gateway y concentrador | los del banco de la cátedra | los de `main` (con la seguridad de la etapa 1, #36) |
| Hoja | ESP32-C3 SuperMini + IRLZ44N + cerradura de 12 V + microswitch; la placa por USB a la notebook, la cerradura con su fuente de 12 V / 5 A | `aura-lockers` 1.0.1, firmado con la clave de prueba del grupo |

- **Grabación:** `erase-flash` y después la imagen firmada, por USB. Una imagen sin firmar no
  arranca con esta configuración (aborta en la verificación de la firma); eso se vio antes de
  firmar y confirma que la configuración de imágenes firmadas está activa.
- **Datos de la mesh:** los del banco, en `main/config_local.h` (no se sube).

## Resultados

| Prueba | Resultado |
|---|---|
| Alta y aula (prueba 1) | ✅ La hoja entró en modo aula por el concentrador (capa 3, RSSI ≈ −82 dBm), y `broker.py` respondió las mediciones con `persistido`. Al apretar y soltar el microswitch, cada cambio salió enseguida como una medición nueva (`puerta_1` 0 ↔ 1) |
| `abrir` desde el tablero | ⚠️ Desde el tablero de `broker.py` se mandó `{"command":"abrir","params":{"puerta":1,"ms":15000},"command_id":"tablero-2"}`; la hoja lo aceptó y energizó la cerradura (lo muestra el monitor de la hoja). La cerradura no se movió: el montaje del banco no tenía la masa común entre la placa y la fuente de 12 V |

Con la masa común, en el montaje de casa, el pulso abre la cerradura. Ese pulso era de 15 s: tres
veces lo que admite la FIT0620 (5 s). La 1.1.0 tiene un tope de 5 s.

## Lo que falta

Con la 1.1.0 en el banco (la cátedra pidió estas; OTA, pruebas 4 a 8, y la 9 quedan para después
de este PR):

1. **Alta y aula de nuevo** (prueba 1): mediciones `{"puerta":…,"cerradura":…}` con `persistido`,
   y `abierta` en el `config` del reporte.
2. **`abrir` con la opción apagada** (la imagen normal): `rechazado`, `abrir_deshabilitado`.
3. **Con la opción encendida** (la imagen de `sdkconfig.banco`):
   - `abrir` con un `vence` válido: `aplicado`, la cerradura abre, `cerradura` 1 y vuelve a 0;
     con cortar al abrir, se corta apenas se abre la puerta;
   - rechazos `sin_vencimiento` (sin `vence`), `orden_vencida` (un `vence` pasado),
     `vencimiento_muy_lejano` (más de 300 s) y `ocupada` (otro `abrir` durante el pulso).
4. **Límite por minuto:** varios `abrir` seguidos hasta que salte `limite_pulsos`.
5. **Alertas:** `intrusion` (abrir la puerta con la mano sin pulso), `puerta_abierta` (con
   `aviso_s` en 5) y `puerta_cerrada` al cerrarla.
6. **`set_config` inválido** (`{"pulso_ms": 150.7}`, `{"pulso_ms": 8000}`, `{"aviso_s": 3}`):
   `rechazado` y la configuración como estaba. **Una orden local** (botón del concentrador):
   `orden_local_no_admitida`.
