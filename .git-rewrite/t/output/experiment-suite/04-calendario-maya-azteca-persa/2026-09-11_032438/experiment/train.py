"""train.py — genera results.json determinista (no hay entrenamiento; 'train' = cómputo).
Uso: cd <RUN> && python -m experiment.train   (o: python experiment/train.py)
"""
import json, os
from .model import lcm, maya_lc_to_jdn, tonalpohualli, persa_bisiesto, mean_year_error_s
from . import data as D

out = {"simulated": False,
       "provenance": "measured: aritmetica exacta + motor archaeoastronomy (Meeus) verificado en sesion 2026-09-11; replica Python de los .m de Octave (run_all.m OK en Octave CLI)",
       "checks": {}, "metrics": {}, "ablations": {}, "anchors": {}}
c = out["checks"]
c["lcm_260_365"] = lcm(260, 365)
c["round_52x365"] = 52 * 365
c["round_73x260"] = 73 * 260
c["era_13_baktun"] = 13 * 144000
c["persian33_days"] = 25 * 365 + 8 * 366
c["lc_13_0_0_0_0"] = maya_lc_to_jdn(13, 0, 0, 0, 0)
c["lc_0_0_0_0_0"] = maya_lc_to_jdn(0, 0, 0, 0, 0)
c["tonal_1"], c["tonal_260"], c["tonal_261"] = tonalpohualli(1), tonalpohualli(260), tonalpohualli(261)
c["persa_1403_leap"], c["persa_1404_leap"], c["persa_1405_leap"] = (
    persa_bisiesto(1403), persa_bisiesto(1404), persa_bisiesto(1405))
m = out["metrics"]
m["persian33_mean_year"] = (25 * 365 + 8 * 366) / 33
m["persian33_err_s_per_year"] = mean_year_error_s(m["persian33_mean_year"])
m["gregorian_err_s_per_year"] = mean_year_error_s(D.GREG_MEAN)
m["julian_err_s_per_year"] = mean_year_error_s(D.JULIAN_MEAN)
m["haab_err_s_per_year"] = mean_year_error_s(365.0)
m["haab_drift_days_per_52y_round"] = 52 * (D.TROPICAL_YEAR - 365.0)
m["era_13baktun_tropical_years"] = D.ERA_13_BAKTUN / D.TROPICAL_YEAR
m["tun_vs_tropical_days_per_year"] = 360 - D.TROPICAL_YEAR
a = out["ablations"]
a["persian_no_leap_drift_s_per_year"] = mean_year_error_s(365.0)   # A1 -> converge a Haab
a["gregorian_no_leap_drift_s_per_year"] = mean_year_error_s(365.0)  # A2 -> idem
a["gmt_plus2_lc13_jdn"] = maya_lc_to_jdn(13, 0, 0, 0, 0) + 2       # A3 sensibilidad
a["tropical_alt_365_2422_persian33_err_s"] = mean_year_error_s(m["persian33_mean_year"], 365.2422)  # A4
an = out["anchors"]
an["gmt_zero_julian_proleptic"] = "6-sep-3114 a.C. (JD 584283.0 verificado)"
an["gmt_zero_gregorian_proleptic"] = "11-ago-3114 a.C. (convencion citada)"
an["cycle_end"] = {"lc": "13.0.0.0.0", "gregorian": "2012-12-21", "jd": 2456283.0}
an["equinox_march"] = D.EQUINOX

path = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "results.json")
with open(path, "w") as f:
    json.dump(out, f, indent=2, default=str)
print("results.json escrito en", path)
