# E1-PB-LECA-LCK01 — conector del gabinete de lockers del pañol

Procesa lo que manda la hoja del gabinete de lockers del pañol del LabECA (ESP32-C3
SuperMini, hoja de la mesh): el estado de la puerta de cada locker y de su cerradura.

## Campos

Tienen que coincidir con lo que manda el firmware (`values`, contrato §3.1). Son estados: solo
se guardan 0 y 1.

| Campo | Unidad | Rango | |
|---|---|---|---|
| `puerta_1` | 0/1 | 0 o 1 | 1 = abierta (microswitch libre), 0 = cerrada |
| `solenoide_1` | 0/1 | 0 o 1 | 1 = cerradura energizada (la puerta se puede abrir), 0 = trabada |

Un valor intermedio (por ejemplo 0.5) está dentro del rango pero no es un estado: el conector lo
quita. Si en una muestra no queda ningún estado válido, la descarta (`sin_estados_validos`).

## Alertas

El conector no emite alertas propias: las de la puerta (`apertura_sin_orden`,
`abierta_demasiado`, `cerrada`) las emite la placa, con el tipo `puerta` (ver la ficha en
[`dispositivos/E1-PB-LECA-LCK01/`](../../dispositivos/E1-PB-LECA-LCK01/)).

## Probarlo

Desde la raíz del repo:

```
python -m unittest discover -s conectores\E1-PB-LECA-LCK01\tests
python herramientas\correr_conector.py conectores\E1-PB-LECA-LCK01 --archivo conectores\E1-PB-LECA-LCK01\tests\mensajes_ejemplo.jsonl --subproceso
```

`tests/mensajes_ejemplo.jsonl` es una apertura completa armada a mano con el formato de `data`
del gateway (cerrada, pulso, puerta abierta, fin del pulso, cerrada); se reemplaza por
mensajes reales del banco cuando estén las pruebas.
