# Diseño experimental: ¿misma aritmética, distinto cielo? Maya, mexica y persa bajo cómputo reproducible

**Pregunta:** ¿Qué estructura matemática comparten y en qué difieren —en aritmética, anclaje astronómico y precisión— los calendarios maya (Long Count + Tzolk'in/Haab), mexica (tonalpohualli/xiuhpohualli) y persa (Solar Hijri / Jalali), cuando se verifican con el mismo motor de cómputo (GNU Octave) y las mismas constantes?
**Tipo de tarea:** cómputo verificatorio / análisis comparativo determinista (no hay aprendizaje automático: no hay entrenamiento, no hay semillas estocásticas; la "réplica" es re-ejecución exacta).
**Modo:** measured. Todos los números de `results.json` provienen de aritmética exacta o del motor `archaeoastronomy` (Meeus) ejecutado en sesión; cada cifra cita su verificación en `data_contract.md §3`. Nada es simulado: si un valor dependiera de una hipótesis no verificada, se marcaría `simulated:true` (no ocurre en este run).
**Framework:** GNU Octave (motor principal, exigido por el usuario) + réplica Python pura (`experiment/`) para portabilidad; figuras en matplotlib. Sin PyTorch/JAX: sería deshonesto invocar deep learning donde hay teoría de números.
**Presupuesto:** < 1 CPU-hora total; 0 GPU; memoria < 100 MB. Todo corre en laptop.

## 1. Motivación (por qué este experimento ahora)
La exploración previa identificó tres frentes calientes: (a) el giro computacional del quipu (Medrano & Khosla 2024), (b) el calendario maya como modelo generativo (Chanier + Xultun), (c) la yupana como computador paralelo. El usuario eligió calendario maya + azteca + "información persa" con experimento Octave. La comparación es novedosa en un punto preciso: mesoamericanos y persas resolvieron el mismo problema (atar el año civil al año trópico y ritualizar el conteo) con filosofías opuestas —ciclos aritméticos fijos sin intercalación (Haab 365 exacto; tonalpohualli 260 autónomo) frente a intercalación astronómica viva (Nowruz = equinoccio observado, bisiestos por regla de Khayyam/Borkowski)—. Ninguna fuente encontrada los compara cuantitativamente en un mismo marco reproducible; este run lo hace con ~30 líneas de aritmética por sistema.

## 2. Hipótesis (falsables)
- **H1 (aritmética):** 18 980 = mcm(260,365) = 52×365 = 73×260 es exacto y genera la Rueda de 52 años; el Long Count es un conteo mixto 20/18 con 13 baktunes = 1 872 000 días; el ciclo persa-33 idealizado dura 12 053 días (año medio 365.242424…).
- **H2 (anclaje):** con correlación GMT (JDN 584 283), 13.0.0.0.0 cae exactamente en 2012-12-21 (JD 2456283.0); la fecha cero en juliano proléptico es 6-sep-3114 a.C. (no 11-ago, que es gregoriano proléptico).
- **H3 (precisión):** ordenados por error de año medio vs 365.24219 d: Haab (365 exacto, −20 925 s/año) ≪ juliano (+675 s/año) < gregoriano (+26.8 s/año) ≈ persa-33 (+20.2 s/año); el persa real (astronómico) es además auto-correctivo por construcción.
- **H4 (deriva):** sin intercalación, el Haab deriva 12.59 días por Rueda de 52 años y ~3.4 años por Era de 13 baktunes; el tonalpohualli, al no pretender seguir al Sol, no "deriva": su error es de categoría, no de magnitud.
- **H5 (Nowruz):** los equinoccios 2024–2026 (Meeus) caen 20-marzo UTC pero Nowruz 1404/1405 cae 21-marzo por la regla del mediodía de Teherán (verificado por consistencia hora UTC + 3:30).

## 3. Datasets y baselines
Dataset = constantes publicadas D1–D4 (ver `data_contract.md`); no hay split train/test porque no hay aprendizaje. Baselines de comparación: año juliano (365.25), gregoriano (365.2425), tun maya (360, control negativo: año ritual-administrativo), Haab (365). El "modelo propuesto" es cada calendario ancestral formalizado como función (día→posición en ciclo; fecha↔JDN).

## 4. Método (operadores verificables)
- **M1 Ciclos:** mcm y factorización (260=2²·5·13; 365=5·73) → 18 980; conteo mixto LC (×20,×18,×20,×20); ciclo persa (25 comunes + 8 bisiestos; restos mod-33 {1,5,9,13,17,22,26,30}).
- **M2 Conversiones:** LC→JDN = 584 283 + Σ dígitos×pesos; JDN→gregoriano vía motor; tonalpohualli: número = ((n−1) mod 13)+1, signo = ((n−1) mod 20)+1; persa: año bisiesto por resto mod-33 (aprox. documentada).
- **M3 Astronomía:** equinoccio de marzo y JD vía `archaeoastronomy`; hora de Teherán = UTC+3:30; regla Nowruz (equinoccio antes del mediodía → mismo día, si no → día siguiente).
- **M4 Derivas:** error lineal acumulado; ablación "sin intercalación" (persa-365-exacto, gregoriano-sin-bisiestos) para aislar el valor de la regla de intercalación.

## 5. Métricas
Exactitud entera (pass/fail) para M1–M2; error de año medio (días/año y s/año) para M3–H3; deriva acumulada (días por Rueda / por Era / por siglo) para H4; residuo equinoccio–Nowruz (horas) para H5. Criterio de éxito del run: 100% pass en M1–M2 y residuos H5 < 24 h con explicación por regla del mediodía.

## 6. Ablaciones
A1: persa sin bisiestos (365 exacto) → debe converger a la deriva Haab. A2: gregoriano sin bisiestos → idem. A3: correlación GMT ± 2 días → el anclaje 2012-12-21 falla (sensibilidad). A4: año trópico alternativo (365.2422 Newcomb-1900) → reordena gregoriano vs persa-33 por < 7 s/año (fragilidad declarada, no colapso).

## 7. Presupuesto y riesgos
Cómputo despreciable; el riesgo es historiográfico, no técnico: (i) confundir juliano/gregoriano proléptico (mitigado: §H2 y test 584283); (ii) presentar el ciclo-33 como ley persa exacta (mitigado: rupturas de 29 años y carácter astronómico declarados); (iii) la fecha "9.6.0.0.0 = 9-may-751" de un resumen-AI secundario es inconsistente con GMT (9.6.0.0.0→~554 d.C.; 9.16.0.0.0→~751): se excluye de resultados y se documenta como hallazgo de control de fuentes. Límite: Meeus-baja-precisión (±minutos) basta para residuos de horas, no para arqueoastronomía de alineaciones (fuera de alcance).
