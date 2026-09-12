# Experimento calendárico (Octave primero, Python como réplica)

Núcleo medido en **GNU Octave** (`*.m`): `octave --quiet run_all.m` debe imprimir
`TODO MEDIDO OK`. Réplica Python exacta (`model.py`/`data.py`) + `train.py` genera
`../results.json` y `evaluate.py` lo verifica. Alcance honesto: aritmética de ciclos,
conversiones y residuos astronómicos (Meeus, minutos); no alineaciones de sitios.
Ver `../data_contract.md` y `../experiment_design.md`.
