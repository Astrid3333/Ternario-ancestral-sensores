# Exploración de investigación: modelos matemáticos ancestrales — saberes indígenas (América)

**Dirección:** formalizar, modelar y reactivar sistemas matemáticos indígenas americanos (andino-amazónicos y mesoamericanos) como objetos matemáticos vivos, no como curiosidades históricas.
**Restricciones del usuario:** enfoque en saberes indígenas (no historia clásica del Viejo Mundo ni genética de poblaciones); primer producto = exploración inicial; idioma = español; cómputo disponible = MCP Octave (verificado en sesión: `ethnomath`, `ancestral_octave`, `archaeoastronomy`).
**Fecha:** 2026-09-11. **Run:** `output/research-explorer/modelos-matematicos-ancestrales-saberes-2e5429e9/2026-09-11_031434/`

## 1. Mapa del paisaje

```
Saberes matemáticos indígenas (América)
├── A. Registro y cálculo andino
│   ├── Quipu: escritura numérica decimal posicional por nudos (S/L/E, cero por ausencia)
│   ├── Yupana: ábaco/tablero de cálculo (Guamán Poma, arqueológicas, Tawa Pukllay)
│   └── Código binario 7-bit (Urton) vs relaciones aditivas (Ascher) vs data science (Medrano/Khosla)
├── B. Tiempo y astronomía mesoamericana
│   ├── Maya: vigesimal roto (kin/winal/tun/katun/baktun), cero, Tzolk'in 260, Haab 365, Rueda 18 980, Long Count
│   ├── Mexica: xiuhpohualli 365 + tonalpohualli 260, xiuhmolpilli 52 años, Piedra del Sol
│   └── Modelos aritméticos de astronomía a ojo desnudo (Xultun, Palenque/Copán, pasos cenitales)
├── C. Número, lengua y cognición
│   ├── Quechua, mapuche (rakin), lenguas Sierra Nevada (kogui/arhuaco/wiwa/kankuamo), tsimane'
│   └── Conteo, medición no convencional, clasificación, inferencia (D'Ambrosio/Bishop)
├── D. Geometría del hacer
│   ├── Textiles (simetrías, transformaciones), cerámica, arquitectura, caminos (Qhapaq Ñan), ceques
│   └── Etnomodelación: puente emic → etic (Rosa & Orey)
└── E. Educación y política del conocimiento
    ├── EIB / etnoeducación / educación propia, currículos regionalizados, IESII (UNESCO)
    └── Tensión decolonial: matemática monocultural vs pluralidad de sistemas
```

Relaciones clave: A calcula y registra lo que B predice (calendarios tributarios/agrícolas); C provee la lengua del número que A y B codifican; D es la geometría implícita en A–C; E decide qué sobrevive en la escuela.

## 2. Estado del arte (síntesis honesta)

- **Quipu:** lo numérico está resuelto hace un siglo (Locke: decimal posicional; Ascher: 8s/X/4L para 804). Lo narrativo sigue abierto: Urton propone binario 7-bit (2003) pero Brokaw (2005) lo impugna por falta de evidencia e inconsistencias. El giro 2024–2025 es computacional: Medrano & Khosla generalizan las "relaciones Ascher" a 650 quipus (≥74% con sumas internas), identifican quipus "de trabajo" por cuerdas superiores y reensamblan fragmentos por aritmética. Datos abiertos (Open Khipu Repository) pero con problemas de calidad (discrepancias Ascher vs Urton en el mismo ejemplar).
- **Yupana:** no hay consenso sobre el algoritmo original (Wassen decimal simple; Radicati; Hernández; Tawa Pukllay por formas/movimientos). Consenso funcional: yupana calcula, quipu archiva. La literatura educativa la reduce a operaciones básicas; su potencial (paralelismo, resta múltiple, decimales infinitos sin tablas, álgebra) está sin explorar formalmente.
- **Maya:** sistema y correlación GMT bien establecidos; cero temprano; aritmética de distancias y serie suplementaria descifradas (Förstemann, Bowditch, Morley). Frontera: derivar todo el sistema calendárico de un modelo generativo mínimo (Chanier: astronomía a ojo desnudo + números Xultun) y cuantificar su precisión vs efemérides modernas; documentar vigencia viva (ajq'ijab', agricultura de maíz).
- **Mexica:** descripción estructural estable (260/365/52), origen del 260 sin consenso (gestación ≈260 días vs pasos cenitales vs convención). Síntesis reciente Biémont 2025.
- **Educación:** campo en expansión 2015–2024 (Colombia, Ecuador, México, Perú, Chile-Mapuche), pero con brecha implementación: pocos materiales, docentes sin formación EIB, evaluaciones aún débiles (BID 2024–2025). La etnomodelación ofrece marco metodológico maduro (Rosa & Orey, 194+ citas) poco aplicado a corpus andino-mesoamericano con cómputo reproducible.

## 3. Candidatos (8 temas, específicos a nivel de título de paper)

### T1. Relaciones Ascher a escala: re-análisis reproducible del corpus abierto de 650 quipus
- **Motivación:** el resultado Medrano & Khosla 2024 (74% con sumas) es el avance cuantitativo más fuerte en 40 años; necesita réplica independiente con código abierto.
- **Ángulo innovador:** reimplementar búsqueda exhaustiva de sumas + detección de delimitadores blancos + clasificación working/premium en Octave (`quipu_encode` + grafos + estadística), con auditoría de calidad de datos Ascher vs Urton.
- **Factibilidad: alta.** Datos abiertos + Octave verificado en sesión. Sin trabajo de campo.
- **Riesgo/pregunta abierta:** ¿las sumas son convención contable o artefacto de agregación? ¿Resiste a corrección por múltiples comparaciones?

### T2. Yupana como computador paralelo: del Tawa Pukllay a la especificación formal
- **Motivación:** la afirmación "CPU inca" (Prem, Univ. Lima) es testeable: reglas locales + paralelismo + resta múltiple sin agrupar.
- **Ángulo innovador:** especificar Tawa Pukllay como sistema de reescritura (estados del tablero, movimientos legales, invariantes), probar complejidad vs algoritmo indo-arábigo y emularlo en Octave; comparar con Radicati/Wassen/Joseph.
- **Factibilidad: alta.** Tablero 5×4 + semillas simulables; `ancestral_octave`/`ethnomath` como base. Validable con casos 408 257 (ejemplo clásico) y 116×52.
- **Riesgo:** múltiples yupanas arqueológicas distintas; ninguna especificación es "la" original. Exigir honestidad: reconstrucción, no desciframiento.

### T3. El calendario maya como modelo generativo mínimo (Long Count + Xultun + Luna)
- **Motivación:** pasar de describir ciclos a derivarlos: ¿qué modelo mínimo genera Tzolk'in/Haab/Rueda/Long Count y predice Xultun y ecuaciones Palenque/Copán?
- **Ángulo innovador:** implementar en Octave el modelo Chanier + verificación con `maya_long_count` y `archaeoastronomy` (JD, solsticios, edad lunar); medir error vs valores modernos (sin anacronismo: evaluar como astronomía a ojo desnudo, 200–900 d.C.).
- **Factibilidad: alta.** `maya_long_count` + `julian_day` verificados (JD 2456283.0 para 2012-12-21 ✓). Constantes GMT documentadas.
- **Riesgo:** sobremodelar; correlación ≠ intención histórica. Contrastar con crítica de Aldana.

### T4. El 260 mesoamericano: gestación, cenit y aritmética (Tzolk'in vs Tonalpohualli)
- **Motivación:** el origen del 260 sigue disputado; es el invariante que une maya, mixteca, mexica y mixe actuales.
- **Ángulo innovador:** test triple en Octave: (a) aritmética pura (mcm 260/365=18 980, 73/52), (b) astronomía cenital a latitudes 14–21°N (`alignment_check`, pasos cenitales), (c) biología (ventana gestacional) — con criterio de parsimonia y distribución geográfica del ciclo.
- **Factibilidad: media-alta.** Solo requiere `archaeoastronomy` + estadística básica; literatura accesible (Biémont 2025, códices).
- **Riesgo:** evidencia etnográfica inclinará la balanza más que el cálculo; evitar determinismo astronómico.

### T5. Gramáticas textiles andinas: simetría y etnomodelación computable
- **Motivación:** el textil andino codifica grupos de simetría y algoritmos de conteo que la escuela ignora; hay corpus fotográfico (Cusco, museos) sin análisis formal sistemático.
- **Ángulo innovador:** clasificar guardas textiles por grupos de friso/papel tapiz + extraer reglas generativas (gramática de tejido) y llevarlas a actividades etnomodeladas evaluables.
- **Factibilidad: media.** Requiere corpus de imágenes con permiso + `fractal_dimension`/`clustering`/`voronoi` de Octave; trabajo colaborativo con tejedoras.
- **Riesgo:** extractivismo académico si no hay co-investigación y retorno. Protocolo ético obligatorio.

### T6. Cognición numérica viva: quechua, mapuche-rakin, Sierra Nevada, tsimane'
- **Motivación:** PNAS 2023 (tsimane') muestra diversidad de aprendizaje aritmético con escolarización reciente; lenguas con bases y clasificadores distintos predicen estrategias distintas.
- **Ángulo innovador:** estudio comparativo mínimo y replicable: tareas de conteo/aritmética + análisis de la morfología numeral (quechua decimal transparente, mapuche, kogui) con pre-registro y materiales abiertos.
- **Factibilidad: media.** Exige campo, consentimiento, hablantes colaboradores; no es solo cómputo.
- **Riesgo:** sobregeneralizar "mentalidad" desde lengua; control escolarización/edad imprescindibles.

### T7. Etnomodelación para EIB: del quipu/yupana/calendario al aula con evidencia
- **Motivación:** la EIB declara interculturalidad pero opera con matemática monocultural (BID/UNESCO); hay demanda de secuencias didácticas con raíces ancestrales y evaluación real.
- **Ángulo innovador:** diseñar 3 secuencias (quipu-decimal, yupana-operaciones, calendario-mcm) con enfoque ético/émico/dialógico, pilotar y medir (diseño cuasi-experimental + fidelity de implementación).
- **Factibilidad: media.** Requiere alianza escolar; materiales baratos (cuerdas, tableros, Octave).
- **Riesgo:** folclorización (añadir motivos sin cambiar epistemología). Criterio: el saber ancestral debe poder refutar al libro, no solo ilustrarlo.

### T8. Paisaje y cielo: ceques, Qhapaq Ñan y orientaciones con test astronómico
- **Motivación:** la infraestructura inca (14 000 millas de caminos, arquitectura monumental) presupone agrimensura y astronomía; las alineaciones propuestas rara vez pasan un test estadístico.
- **Ángulo innovador:** protocolo `archaeoastronomy(alignment_check)` + análisis espacial sobre muestra de sitios (azimut, horizonte, declinación vs solsticios/lunisticios) con corrección por selección y horizonte real (no azimut plano).
- **Factibilidad: media.** Necesita coordenadas/azimuts publicados + horizonte; Octave ya dispone del motor.
- **Riesgo:** "alineacionitis" (encontrar patrones en ruido). Pre-registro de hipótesis indispensable.

## 4. Recomendación (1–3 a perseguir)

1. **T1 (quipu + data science) — prioridad 1.** Es el frente caliente 2024–2025, 100% reproducible en este workspace con Octave, y produce un resultado verificable (réplica + auditoría de datos) publicable como nota o pre-survey ampliado. Engancha directo con tu pedido "mcp octave".
2. **T3 (calendario maya generativo) — prioridad 2.** Ideal para lucir Octave (`maya_long_count` + `archaeoastronomy` + MCM 18 980), con figuras publicables (ruedas, distancias, errores lunares) y vigencia viva que evita el anticuarismo.
3. **T2 (yupana/Tawa Pukllay) — prioridad 3 / diferencial andino.** Si tu adscripción es andina, T2 te distingue: nadie ha publicado la especificación formal + benchmark de complejidad. T1+T2 combinan en un paper "máquina inca completa" (CPU=yupana, memoria=quipu).

Secuencia sugerida: T1 (4–6 semanas, réplica) → T3 (experimento Octave + figuras) → survey o paper (skill `literature-survey` / `paper-writer`) → T2/T7 como fase con campo/escuela.

## 5. Qué pedir después (handoff)
- Si eliges T1/T3/T2: skill `experiment-suite` (diseño + código Octave + resultados + figuras + reporte).
- Si quieres el estado del arte exhaustivo: skill `literature-survey` (60+ citas reales).
- Si quieres el mapa visual: skill `mindmap-render` consumiendo `topic_matrix.md`.
- Todos los conteos calendáricos propuestos usan GMT (11-ago-3114 a.C.); cualquier cambio de correlación debe declararse.

*Nota de honestidad: los candidatos son sugerencias con factibilidad heurística; la originalidad debe verificarse antes de comprometer tesis o envío. Puntajes verificados solo donde hay evidencia Octave propia (§search_notes); el resto es juicio informado.*
