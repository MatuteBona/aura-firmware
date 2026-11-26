"""Conector de E1-PB-LECA-LCK01 — gabinete de lockers del pañol (LabECA).

Guarda el estado de cada puerta (microswitch) y de su cerradura (solenoide energizado o no).
Son estados, no magnitudes: solo valen 0 y 1. Un valor intermedio (0.5) entra en el rango
del manifiesto pero no es un estado, así que se quita antes de guardar.

Las alertas de la puerta (apertura_sin_orden, abierta_demasiado, cerrada) las emite la placa
(contrato §3.5): el conector no las repite.
"""
from aura_sdk import Conector


class ConectorLockers(Conector):
    def al_recibir_datos(self, msg, ctx):
        estados = {campo: valor for campo, valor in msg.values.items()
                   if not isinstance(valor, bool) and valor in (0, 1)}
        if not estados:
            ctx.descartar("sin_estados_validos")
            return
        ctx.guardar_medicion(estados, msg.ts)
