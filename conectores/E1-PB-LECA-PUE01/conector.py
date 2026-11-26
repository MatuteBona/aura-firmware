"""Conector de E1-PB-LECA-PUE01 — locker del pañol (LabECA).

Guarda el estado de la puerta (microswitch) y de la cerradura (energizada o no). Son estados, no
magnitudes: solo valen 0 y 1. Un valor intermedio (0.5) entra en el rango del manifiesto pero no
es un estado, así que se quita antes de guardar.

Las alertas (intrusion, puerta_abierta, puerta_cerrada, limite_pulsos) las emite la placa
(contrato v4.5, §3.5): el conector no las repite.
"""
from aura_sdk import Conector


class ConectorLocker(Conector):
    def al_recibir_datos(self, msg, ctx):
        estados = {campo: valor for campo, valor in msg.values.items()
                   if not isinstance(valor, bool) and valor in (0, 1)}
        if not estados:
            ctx.descartar("sin_estados_validos")
            return
        ctx.guardar_medicion(estados, msg.ts)
