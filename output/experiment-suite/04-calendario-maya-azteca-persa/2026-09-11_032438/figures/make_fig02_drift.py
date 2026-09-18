"""Fig2: deriva acumulada vs Sol (días) 0–2000 años — MEDIDO."""
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

plt.rcParams.update({"font.size": 10, "axes.grid": True, "grid.alpha": 0.3,
                     "svg.fonttype": "none"})
T = 365.24219
yrs = np.linspace(0, 2000, 501)
series = {"Haab 365 (sin intercalar)": 365.0, "Juliano 365.25": 365.25,
          "Gregoriano 365.2425": 365.2425, "Persa ciclo-33": 12053 / 33}
fig, ax = plt.subplots(figsize=(7.5, 4.2))
for name, mean in series.items():
    ax.plot(yrs, (mean - T) * yrs, label=name, linewidth=1.8)
ax.axhline(0, color="black", linewidth=0.8)
ax.set_xlabel("Años transcurridos")
ax.set_ylabel("Adelanto del calendario sobre el Sol [días]")
ax.set_title("(b) Deriva acumulada: sin intercalación el Haab pierde 12.6 días por Rueda (medido)")
ax.legend(fontsize=9)
fig.tight_layout()
fig.savefig("fig02_drift_accumulation.pdf")
fig.savefig("fig02_drift_accumulation.svg")
print("fig02 OK")
