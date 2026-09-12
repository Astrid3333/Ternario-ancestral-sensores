"""Fig3: coincidencia Tzolk'in/Haab — peines 260 y 365, Rueda 18 980 — MEDIDO."""
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

plt.rcParams.update({"font.size": 10, "axes.grid": True, "grid.alpha": 0.3,
                     "svg.fonttype": "none"})
N = 2 * 18980 + 1
d = np.arange(N)
tz = (d % 260 == 0).astype(int)
ha = (d % 365 == 0).astype(int)
both = ((d % 18980) == 0).astype(int)
fig, ax = plt.subplots(figsize=(7.5, 4.2))
ax.vlines(d[tz == 1][::1], 0.55, 1.0, colors="#1f77b4", linewidth=0.4, label="retorno Tzolk'in/tonalpohualli (260)")
ax.vlines(d[ha == 1][::1], 0.0, 0.45, colors="#ff7f0e", linewidth=0.4, label="retorno Haab/xiuhpohualli (365)")
ax.stem(d[both == 1], both[both == 1] * 1.35, linefmt="#d62728", markerfmt="ro",
        basefmt=" ", label="Rueda calendárica (18 980 d)")
ax.set_xlim(-200, N)
ax.set_ylim(-0.05, 1.6)
ax.set_xlabel("Días desde una coincidencia inicial")
ax.set_title("(c) Rueda de 52 años: 18 980 = mcm(260,365) = 52×365 = 73×260 (medido)")
ax.legend(fontsize=9, loc="upper right")
fig.tight_layout()
fig.savefig("fig03_round_coincidence.pdf")
fig.savefig("fig03_round_coincidence.svg")
print("fig03 OK")
