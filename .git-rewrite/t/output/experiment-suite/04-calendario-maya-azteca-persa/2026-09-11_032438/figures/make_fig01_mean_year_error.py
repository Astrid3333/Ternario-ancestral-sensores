"""Fig1: error de año medio (s/año, escala simlog) — MEDIDO."""
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

plt.rcParams.update({"font.size": 10, "axes.grid": True, "grid.alpha": 0.3,
                     "svg.fonttype": "none"})
labels = ["Tun maya\n(360 d)", "Haab / xiuhpohualli\n(365 d)", "Juliano\n(365.25)",
          "Gregoriano\n(365.2425)", "Persa ciclo-33\n(365.242424)"]
err_s = [(360 - 365.24219) * 86400, (365 - 365.24219) * 86400,
         (365.25 - 365.24219) * 86400, (365.2425 - 365.24219) * 86400,
         (12053 / 33 - 365.24219) * 86400]
colors = ["#999999", "#1f77b4", "#ff7f0e", "#2ca02c", "#d62728"]
fig, ax = plt.subplots(figsize=(7.5, 4.2))
y = np.arange(len(labels))
ax.barh(y, err_s, color=colors, edgecolor="black", linewidth=0.6)
ax.set_yticks(y, labels)
ax.set_xlabel("Error de año medio respecto al año trópico 365.24219 d  [s/año, escala simlog]")
ax.set_xscale("symlog", linthresh=10)
ax.axvline(0, color="black", linewidth=0.8)
for i, v in enumerate(err_s):
    ax.text(v, i, f" {v:+.1f} s/año", va="center", ha="left" if v > 0 else "right", fontsize=9)
ax.set_title("(a) Precisión del año medio: calendarios ancestrales y europeos (cómputo medido)")
fig.tight_layout()
fig.savefig("fig01_mean_year_error.pdf")
fig.savefig("fig01_mean_year_error.svg")
print("fig01 OK", [round(v, 2) for v in err_s])
