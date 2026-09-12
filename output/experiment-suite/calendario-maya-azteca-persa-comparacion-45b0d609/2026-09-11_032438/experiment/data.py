"""data.py — constantes del run (réplica Python de config.yaml; no hay dataset externo)."""
GMT_JDN_ZERO = 584283
TROPICAL_YEAR = 365.24219
TZOLKIN, HAAB, ROUND = 260, 365, 18980
BAKTUN, KATUN, TUN, WINAL, KIN = 144000, 7200, 360, 20, 1
ERA_13_BAKTUN = 1872000
PERSIAN_RESIDUES_MOD33 = {1, 5, 9, 13, 17, 22, 26, 30}
TEHRAN_OFFSET_H = 3.5
GREG_MEAN, JULIAN_MEAN = 365.2425, 365.25
# Equinoccios de marzo medidos en sesión (MCP archaeoastronomy, Meeus; hora UTC decimal, JD)
EQUINOX = {
    2024: {"jd": 2460389.62797, "utc_h": 3.0713, "nowruz_jalali": 1403, "nowruz_greg": "2024-03-20"},
    2025: {"jd": 2460754.86874, "utc_h": 8.8498, "nowruz_jalali": 1404, "nowruz_greg": "2025-03-21"},
    2026: {"jd": 2461120.10954, "utc_h": 14.6290, "nowruz_jalali": 1405, "nowruz_greg": "2026-03-21"},
}
