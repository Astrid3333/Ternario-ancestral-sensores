"""Integraciones externas del Tritos Lab (Blender, Godot).

 Existe como paquete regular para que `from tritos_lab.tritos_blender import ...`
 resuelva al submodulo y no al modulo suelto `tritos_lab.py` de la raiz: con un
 paquete sin `__init__` (namespace package) el fichero `tritos_lab.py` tiene
 prioridad y el import falla con ImportError.
"""
