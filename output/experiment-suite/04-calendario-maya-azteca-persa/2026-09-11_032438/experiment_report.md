# Informe experimental: misma aritmética, distinto cielo (maya · mexica · persa)

**Divulgación:** todos los resultados son MEDIDOS (cómputo determinista en GNU Octave,
re-ejecutable con `experiment/run_all.m`; réplica Python en `experiment/train.py`).
Nada es simulado: `results.json → "simulated": false`.
**Run:** `output/experiment-suite/calendario-maya-azteca-persa-comparacion-45b0d609/2026-09-11_032438/`

## 1. Problema
Tres tradiciones resolvieron el calendario con filosofías opuestas: los mayas contaron días
en un sistema vigesimal roto con ciclos rituales fijos (Tzolk'in 260, Haab 365, Rueda 18 980,
Long Count); los mexicas operaron el mismo par 260/365 (tonalpohualli/xiuhpohualli, atado de
52 años); los persas ataron el año al equinoccio observado (Nowruz, intercalación de
Khayyam, ciclo-33 de Borkowski). Faltaba compararlos con el mismo motor y constantes.

## 2. Diseño (resumen; ver `experiment_design.md`)
H1 aritmética exacta → H2 anclaje GMT → H3 ranking de precisión → H4 deriva sin
intercalación → H5 regla del mediodía de Teherán. Métricas: pass/fail entero, error de año
medio (s/año), deriva acumulada (días), residuo equinoccio–Nowruz (horas). Ablaciones A1–A4.

## 3. Método y setup
Octave: `maya_lc_to_jdn.m`, `tonalpohualli.m`, `persa_bisiesto.m`, `run_all.m`
(GNU Octave, `octave-cli`, < 1 s). Astronomía: MCP `archaeoastronomy` (Meeus, juliano
proléptico pre-1582). Réplica: `experiment/model.py` + `data.py`; `train.py` genera
`results.json`; `evaluate.py` lo audita (todo OK). Constantes en `config.yaml`
(año trópico 365.24219, GMT 584 283, restos persas mod-33 {1,5,9,13,17,22,26,30}).

## 4. Resultados
- **M1 (Fig.3 `fig03_round_coincidence.pdf`):** mcm(260,365) = 18 980 = 52×365 = 73×260 ✓;
  13 baktunes = 1 872 000 días; ciclo persa-33 = 12 053 días; tonalpohualli cierra en 260/261 ✓.
- **M2:** 13.0.0.0.0 → JDN 2 456 283 = 2012-12-21 ✓ exacto; 0.0.0.0.0 → 584 283 ✓;
  1403 bisiesto / 1404–1405 comunes ✓ (coincide con jalcal/timeanddate).
- **Precisión (Fig.1 `fig01_mean_year_error.pdf`):** persa-33 +20.24, gregoriano +26.78,
  juliano +674.78, Haab −20 925.22, tun −452 925.22 s/año. El ciclo-33 persa supera al
  gregoriano por ~6.5 s/año con el trópico adoptado.
- **Deriva (Fig.2 `fig02_drift_accumulation.pdf`):** el Haab pierde 12.594 días por Rueda y
  ~3.4 años por Era de 13 baktunes (5125.37 años trópicos); la Era coincide con los 5125.366
  del Smithsonian a 0.004 años.
- **Ablación (Fig.4 `fig04_ablation_no_leap.pdf`):** sin bisiestos, persa y gregoriano
  colapsan a −12.594 d/52 a, idéntico al Haab: toda su precisión vive en la intercalación.
- **Nowruz:** equinoccios 2024/2025/2026 (20-mar UTC 03:07/08:51/14:37) + regla del mediodía
  de Teherán explican Nowruz 1404 y 1405 el 21-mar (residuos < 24 h con causa).

## 5. Análisis
La Rueda mesoamericana es un triunfo de teoría de números (260 = 2²·5·13 aporta el factor
13 ausente en 365 = 5·73), no de astronomía: ritual y Sol conviven sin corregirse, y el
"error" del tonalpohualli es de categoría (no pretende seguir al Sol). Persia invierte la
apuesta: el año lo define el cielo cada marzo y la aritmética (ciclo-33) solo aproxima; por
eso es auto-correctivo. El Long Count, con su base rota ×18, es el único conteo lineal largo
de los tres: historia fechable al día frente a tiempo cíclico.Sensibilidad (A3–A4): mover GMT
±2 días rompe el anclaje 2012 (el sistema es rígido, buena señal); cambiar el trópico a
365.2422 mueve persa/gregoriano < 7 s/año (el ranking H3 es robusto salvo ese margen).

## 6. Limitaciones
Meeus-baja-precisión basta para horas, no para alineaciones de sitios (fuera de alcance);
el ciclo-33 es aproximación con rupturas de 29 años (declarado en figuras); juliano vs
gregoriano proléptico exige declarar convención (la fecha cero es 6-sep juliano = 11-ago
gregoriano de 3114 a.C.); se excluyó por inconsistente la fecha secundaria
"9.6.0.0.0 = 9-may-751" de un resumen-AI (con GMT cae ~554 d.C.; ~751 corresponde a
9.16.0.0.0).

## 7. Conclusión y siguiente paso
Misma aritmética ritual (18 980), distinto pacto con el cielo: Mesoamérica fija el ciclo y
acepta la deriva; Persia fija el equinoccio y deriva la aritmética. Todo verificado en
Octave en < 1 s. Siguiente: T1 (quipu + data science, corpus 650) o T2 (yupana formal) con
el mismo protocolo; figuras reutilizables vía `figures/manifest.json` (solo basenames).
