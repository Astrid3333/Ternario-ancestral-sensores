"""evaluate.py — verifica results.json contra los valores medidos esperados.
Uso: cd <RUN> && python -m experiment.evaluate
"""
import json, os, sys

path = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "results.json")
r = json.load(open(path))
assert r["simulated"] is False, "este run es measured"
c, m = r["checks"], r["metrics"]
assert c["lcm_260_365"] == 18980 == c["round_52x365"] == c["round_73x260"]
assert c["era_13_baktun"] == 1872000 and c["persian33_days"] == 12053
assert c["lc_13_0_0_0_0"] == 2456283 and c["lc_0_0_0_0_0"] == 584283
assert c["tonal_1"] == [1, 1] and c["tonal_260"] == [13, 20] and c["tonal_261"] == [1, 1]
assert c["persa_1403_leap"] is True and c["persa_1404_leap"] is False
assert abs(m["persian33_mean_year"] - 365.24242424) < 1e-6
assert abs(m["persian33_err_s_per_year"] - 20.24) < 0.05
assert abs(m["haab_drift_days_per_52y_round"] - 12.594) < 0.002
print("evaluate: TODO MEDIDO OK")
