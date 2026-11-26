# Prueba en placa: E1-PB-LECA-LCK01 en el banco de la mesh (2026-11-06)

Grupo de lockers del pañol, IC III 2026 (@MatuteBona, @agusferrero05, @bertolaco).

Primera vez que la hoja del gabinete de lockers entró a la malla de AURA: el banco de la
materia en el LabECA (gateway y concentrador de la cátedra, `broker.py` en una notebook del
laboratorio). La hoja corrió el firmware `aura-lockers` 1.0.1 de esta carpeta, firmado con la
clave de prueba del grupo, sobre una ESP32-C3 SuperMini conectada al hardware de un locker
(cerradura de 12 V con IRLZ44N y microswitch, ver la ficha).

**Resultado: la prueba 1 salió bien y el comando `abrir` llegó a la hoja. Las pruebas 2 a 9
todavía no están hechas** (ver [Lo que falta](#lo-que-falta)). Este informe queda abierto y se
completa en el mismo PR.

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

| N.º | Prueba | Resultado |
|---|---|---|
| 1 | Alta y aula | ✅ La hoja entró en modo aula por el concentrador (capa 3, RSSI ≈ −82 dBm), y AURA confirmó las mediciones (`persistido`). Al apretar y soltar el microswitch, cada cambio salió enseguida como una medición nueva (`puerta_1` 0 ↔ 1) |
| — | Comando `abrir` (propio del dispositivo) | ⚠️ Llegó desde AURA `{"command":"abrir","params":{"puerta":1,"ms":15000},"command_id":"tablero-2"}`, la hoja lo aceptó y energizó el solenoide (lo muestra el monitor de la hoja). La cerradura no se movió: el montaje del banco no tenía la masa común entre la placa y la fuente de 12 V. Con esa unión, en el montaje de casa, el pulso de 15 s abre la cerradura. Falta repetirlo en el banco |
| 2 | `set_config` inválido | pendiente |
| 3 | Plan B | pendiente |
| 4 | OTA normal | pendiente (1.0.1 → 1.0.2) |
| 5 | Misma versión | pendiente |
| 6 | Imagen sin firma | pendiente |
| 7 | Otra clave | pendiente |
| 8 | Vuelta atrás | pendiente |
| 9 | Ningún dato perdido | pendiente |

## Lo que falta

1. Las pruebas 2 a 9 de `GUIA_HOJAS_NUEVAS.md` (paso 5), con `broker.py`, el tablero y `ota.py`,
   en la próxima sesión de banco.
2. Repetir `abrir` en el banco con la masa común y ver que la cerradura abre y que
   `puerta_1` pasa a 1 al abrir la puerta.
3. Las alertas de tipo `puerta`: `apertura_sin_orden` (abrir la puerta con la mano sin
   pulso), `abierta_demasiado` (con `max_abierta_s` en 30) y `cerrada`.
4. Los rechazos de `abrir`: `puerta` 2, `ms` 50 y 30000, y una orden `local`.
