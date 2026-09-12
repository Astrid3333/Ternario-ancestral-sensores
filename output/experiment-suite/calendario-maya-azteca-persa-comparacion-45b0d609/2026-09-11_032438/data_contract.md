# Contrato de datos — comparativa calendárica maya / mexica / persa (cómputo Octave)

**Run:** `output/experiment-suite/calendario-maya-azteca-persa-comparacion-45b0d609/2026-09-11_032438/`
**Modo:** measured (cómputo determinista verificado; sin sujetos humanos, sin datos personales).
**Fecha:** 2026-09-11.

## 1. Fuentes (binding)
| # | Dato / constante | Fuente | Ruta de acceso | Versión / fecha |
|---|------------------|--------|----------------|-----------------|
| D1 | Estructura Long Count, Tzolk'in 260, Haab 365, Rueda 18 980, creación 13.0.0.0.0 = 11-ago-3114 a.C. (GMT), fin ciclo 21-dic-2012 | Smithsonian NMAI (fetcheado 2026-09-11) + Wikipedia/FAMSI (búsqueda en sesión) | https://maya.nmai.si.edu/calendar/calendar-system | consulta 2026-09-11 |
| D2 | Xiuhpohualli 365 (18×20+5), tonalpohualli 260 (20×13), xiuhmolpilli 52 años | Biémont 2025 (Springer, cap.5) + Wikipedia/azteccalendar (búsqueda) | https://link.springer.com/chapter/10.1007/978-3-031-95967-7_5 | 2025 |
| D3 | Persa Solar Hijri: 12 meses (6×31+5×30+29/30), Nowruz=equinoccio vernal, ciclo 33 años (8 bisiestos, restos 1,5,9,13,17,22,26,30 mod 33), Khayyam 1079, modelo Borkowski 1996 (equinoccios 550–3800 d.C.) | jalcal (Jalilian, CRAN/GitHub) + timeanddate + Univ. Teherán (búsqueda 2026-09-11) | https://github.com/jalilian/jalcal | jalcal 0.3.0 (2025-03-20) |
| D4 | Año trópico de referencia 365.24219 días | Valor estándar adoptado en el run (constante física, no dataset) | `experiment/config.yaml: tropical_year` | fijo en run |
| D5 | Equinoccios/solsticios y días julianos de anclaje | Calculados en sesión con MCP `archaeoastronomy` (Meeus, baja precisión) | `search` no aplica; ver §3 | 2026-09-11 |

## 2. Qué NO hay
- Sin datos de campo, sin entrevistas, sin imágenes con derechos: nada que anonimizar.
- Sin corpus quipu: este run es solo calendarios (T1 queda para otro run).
- Reutilización permitida: constantes D1–D4 y funciones de conversión son reutilizables por `paper-writer` vía `results.json`.

## 3. Verificaciones measured ya ejecutadas (MCP Octave, 2026-09-11)
- `lcm(260,365)=18980`; `52*365=18980`; `73*260=18980` ✓
- `13*144000=1872000`; `1872000/365.24219≈5125.36` años ✓ (Smithsonian: 5125.366)
- Ciclo persa 33 a: `25*365+8*366=12053` días; año medio `12053/33≈365.242424`; error `+20.24 s/año` vs D4 ✓
- `julian_day(2012-12-21)=2456283.0` y `584283+1872000=2456283` ✓ (cierre exacto GMT)
- `julian_day(-3113-09-06)=584283.0` ✓ (fecha cero GMT en juliano proléptico = 6-sep-3114 a.C.; el 11-ago es en gregoriano proléptico — documentado en reporte)
- Equinoccios marzo: 2024 JD 2460389.62797 (03:07 UTC), 2025 JD 2460754.86874 (08:51 UTC), 2026 JD 2461120.10954 (14:37 UTC); Nowruz 1404=2025-03-21 y 1405=2026-03-21 (regla mediodía Teherán) ✓ consistentes
- Deriva Haab en una Rueda: `52*(365.24219-365)=12.594` días ✓

## 4. Límite de validez
- `archaeoastronomy` usa juliano proléptico antes del 1582-10-15: las comparaciones con fechas gregorianas prolépticas declaran la convención.
- El año trópico varía lentamente (Newcomb −0.00000614 d/siglo): D4 fijo es convención del run; el reporte muestra sensibilidad.
- La regla bisiesta persa real es astronómica (Teherán), el ciclo-33 es aproximación documentada con rupturas de 29 años: se reporta como tal, no como ley exacta.
