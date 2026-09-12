# Notas de búsqueda — modelos matemáticos ancestrales / saberes indígenas

Fecha: 2026-09-11. Idioma de entregables: español. Herramientas Octave-MCP disponibles y verificadas en sesión.

## Verificación Octave-MCP (evidencia propia, 2026-09-11)
- `archaeoastronomy(mode=julian_day, 2012-12-21)` → 2456283.0 ✓ (coincide con JD conocido del fin del ciclo 13 baktun).
- `ancestral_octave(preset=quipu_encode, value=804)` → corre y devuelve cuerdas por posición decimal ✓ (estructura posicional base-10 confirmada; convención de tipo de nudo simplificada respecto a Ascher: cifra de unidades con nudo largo).
- Schemas confirmados: `ethnomath` (maya_long_count vigesimal mixto 18–20 ↔ juliano/Tzolk'in/Haab; quipu_encode encode/decode; resto chino; vedico; Arquímedes; Enri), `archaeoastronomy` (solar_position, lunar_position, equinox_solstice, alignment_check, julian_day), `ancestral_octave` (suanpan, TCR, vedico, Arquímedes, quipu, Ifa binario, etak, Madhava).
- Implicación: los temas 1–4 de la exploración son modelables ya en Octave sin dependencias externas.

## Dimensión 1 — Hot topics
Query ES: "etnomatemática saberes ancestrales indígenas 2024 2025 avances investigación".
Hallazgos: expansión 2015–2024 en Colombia (tesis Revueltas 2025, USB); Revista Latinoamericana de Etnomatemática Vol.17(2024) activa; PNAS 2023 O'Shaughnessy sobre cognición numérica tsimane' (Amazonía boliviana, citado 18 veces); MAA 2025 Bland sobre etnomatemática como pedagogía culturalmente receptiva; Sage 2024 Batiibwe revisión rol en educación (149–161 citas); Springer 2024 Indigenous Mathematics (debate universalismo vs especificidad cultural); Vaticano/PASS 2024 booklet conocimiento indígena.

## Dimensión 2 — Núcleo andino: quipu/yupana
Queries: "quipu inca yupana sistema numérico matemático investigación".
Hallazgos:
- Rojas-Gamarra & Stepanova 2015: formalización algebraica (sumatorias, matrices, vectores) del sistema Inka en yupana (binario/quinario/decimal según lectura) y khipu (posicional decimal de escritura). Yupana calcula, khipu registra.
- Catepillán & Szymanski 2012: algoritmos con yupana + actividades de aula; quechua hablado y conteo.
- Cultura Científica (Ibáñez): quipu = escritura numérica posicional decimal por nudos; yupana de Guamán Poma explicada (Wassen, Radicati, Joseph); multiplicación por descomposición.
- EL PAÍS 2022 (de León): sistema Ascher (S/L/E, X=cero); quipucamayoc; yupana=CPU, quipu=memoria (tesis Tawa Pukllay, hermanos Prem, Univ. Lima).
- Tawa Pukllay: reconocimiento de formas y movimientos, operaciones en paralelo, resta múltiple sin agrupar, multiplicación/división sin tablas; libro "Yupana inka. Decodificando la matemática inka".
- Idicap 2024: la yupana muy estudiada en operaciones básicas; pendiente: pensamiento formal (Radicati), álgebra, potencial no explorado.

## Dimensión 3 — Calendario maya
Query: "Maya vigesimal calendar mathematical model Long Count astronomy".
Hallazgos:
- Estructura: kin(1)/winal(20)/tun(360=18×20)/katun(7200)/baktun(144000); base-20 rota en 3ª posición; cero con glifo concha; 13 baktunes=1 872 000 días≈5125 años; fecha creación 13.0.0.0.0 4 Ajaw 8 Kumk'u = 11-ago-3114 a.C. (GMT); fin ciclo 21-dic-2012.
- Tzolk'in 260=13×20, Haab 365=18×20+5(Wayeb), Calendar Round 18 980=mcm(260,365)=52×365=73×260.
- Smithsonian NMAI (fetch en sesión): Haab/Tzolk'in/Calendar Round/Long Count vigentes hoy (ajq'ijab', Wajxaqib' B'atz); agricultores usan observación solar/maíz.
- Chanier 2016/2018 (arXiv:1601.03132): modelo aritmético de astronomía a ojo desnudo; ecuaciones lunares Palenque/Copán como soluciones; números Xultun (2012) como evidencia.
- Aldana (Encyclopedia History of Science, CMU): Long Count olmeca→maya clásico 8.17–10.5; crítica a interpretaciones astronómicas simplistas del origen; valor cronológico absoluto.
- Herramientas: ClassicMayan Portal (cálculo de distancias, serie suplementaria G/Y, edad lunar); FAMSI calendrics.

## Dimensión 4 — Desciframiento quipu: problema abierto + data science
Query: "quipu decipherment open problems binary coding Urton 2023 2024".
Hallazgos:
- Urton 2003 (Signs of the Inka Khipu): código binario 7-bit (torsión, hilado, dirección nudo, color, material, anexo, eje). Crítica Brokaw 2005 (JIH): falta evidencia etnográfica, inconsistencias lógicas; debate abierto.
- Ascher & Ascher 1969 Nature / 1981 Code of the Quipu: relaciones matemáticas internas (sumas) a nivel de ejemplar.
- Medrano & Khosla 2024 (Latin American Antiquity 36(2):497–516, fetch en sesión): corpus 650 quipus (el mayor computacional); relaciones Ascher en ≥74%; top cords = quipus "de trabajo" de bajo nivel; reensamblaje de fragmentos por aritmética; cuerdas blancas como delimitadores. Métodos: búsqueda exhaustiva, confirmación, reensamblaje matemático, generación de hipótesis.
- Clindaniel 2018 (tesis Harvard): gramática de signos no numéricos; Hyland 2016/2020: bandas de color, seriación, indicadores de sujeto/género; quipus coloniales Santa Valley como par papel-nudo (camino a "Rosetta").
- Datos: ~870–1000 ejemplares; Open Khipu Repository (GitHub khipulab); Khipu Field Guide; base Ascher/Brezine; problema de calidad de datos (Urton vs Ascher discrepan en montaje/nudos del mismo quipu — Tandfonline 2024).
- Knill (Harvard, PDF): quipu como graph database; reto criptológico sin "piedra Rosetta" (no fonético).

## Dimensión 5 — Aplicaciones / educación intercultural
Query ES: "etnomatemática educación intercultural aplicaciones currículo América Latina 2024".
Hallazgos: EIB/MOSEIB (Ecuador), etnoeducación Colombia, educación propia; UNESCO 2022 IESII + seminario 2022/2026; BID 2024–2025 (crisis aprendizaje: 75% sin competencias básicas mate; marco EIB de calidad; evaluación muestra mejoras en etnomatemáticas); tensión decolonial (matemática monocultural eurocéntrica vs saberes propios); metodologías: etnografía, investigación colaborativa, MII (Gasché/Bertely), etnomodelación (Rosa & Orey).

## Dimensión 6 — Mexica/azteca
Query: "Aztec calendar piedra del sol mathematical astronomy model tonalpohualli xiuhpohualli".
Hallazgos: xiuhpohualli 365=18×20+5 nemontemi; tonalpohualli 260=20 trecenas×13; Rueda 52 años xiuhmolpilli / 73 tonalpohualli; hipótesis origen 260 (gestación, cenit solar, ninguno-Sahagún); Biémont 2025 (Springer, Maya and Aztecs' Sky, cap.5, pp.147–168); Piedra del Sol (Tonatiuh + 20 signos).

## Dimensión 7 — Cross-field: cómputo, geometría, etnomodelación
Query: "ethnomathematics computation modeling fractal architecture weaving Andean Amazonian geometry".
Hallazgos: etnomodelación = intersección antropología/etnomatemática/modelación (Rosa & Orey 2013, ERIC, 194 citas); traducción emic→etic→dialógica; tejidos (simetrías, transformaciones geométricas), arquitectura, volumen/masa PNG (Owens), calendario agrícola lunar; fractales africanos/andinos como analogía; Ascher 2005 spin/ply/knot; Tawa Pukllay como cómputo paralelo con reglas locales.

## URLs canónicas fetcheadas en sesión (verificación directa)
1. https://maya.nmai.si.edu/calendar/calendar-system (Smithsonian NMAI, sistema calendario maya + vigencia actual).
2. https://www.cambridge.org/core/journals/latin-american-antiquity/article/how-can-data-science-contribute-to-understanding-the-khipu-code/E2D0E68A0515F3F9582A23C63693F4CE (Medrano & Khosla 2024, abstract+resumen+refs).
3. Intento https://www.revista.etnomatematica.org/index.php/RevLatEm/issue/view/50 → 500 (se usa URL de la revista como referencia de existencia Vol.17 2024 vía resultados de búsqueda).
