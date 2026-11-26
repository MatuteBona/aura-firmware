# E1-PB-LECA-PUE01 — conector del locker del pañol

Procesa lo que manda la hoja de un locker del pañol del LabECA (ESP32-C3 SuperMini, hoja de la
mesh, una placa por locker): el estado de la puerta y de la cerradura.

## Campos

Tienen que coincidir con lo que manda el firmware (`values`, contrato §3.1). Son estados: solo
se guardan 0 y 1.

| Campo | Unidad | Rango | |
|---|---|---|---|
| `puerta` | 0/1 | 0 o 1 | 1 = abierta (microswitch libre), 0 = cerrada |
| `cerradura` | 0/1 | 0 o 1 | 1 = energizada (la puerta se puede abrir), 0 = trabada |

Un valor intermedio (por ejemplo 0.5) está dentro del rango pero no es un estado: el conector lo
quita. Si en una muestra no queda ningún estado válido, la descarta (`sin_estados_validos`).

## Alertas

El conector no emite alertas propias: `intrusion`, `puerta_abierta`, `puerta_cerrada` y
`limite_pulsos` las emite la placa, con los tipos del contrato v4.5 (§3.5). Ver la ficha en
[`dispositivos/E1-PB-LECA-PUE01/`](../../dispositivos/E1-PB-LECA-PUE01/).

## Probarlo

Desde la raíz del repo:

```
python -m unittest discover -s conectores\E1-PB-LECA-PUE01\tests
python herramientas\correr_conector.py conectores\E1-PB-LECA-PUE01 --archivo conectores\E1-PB-LECA-PUE01\tests\mensajes_ejemplo.jsonl --subproceso
```

`tests/mensajes_ejemplo.jsonl` es una apertura completa armada a mano con el formato de `data`
del gateway (cerrada, pulso, puerta abierta, fin del pulso, cerrada); se reemplaza por
mensajes reales del banco cuando estén las pruebas.
