"""Fig2: Ahorro energético Landauer — MEDIDO."""
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

plt.rcParams.update({"font.size": 10, "axes.grid": True, "grid.alpha": 0.3,
                     "svg.fonttype": "none"})

methods = ["Binario\n(raw)", "gzip", "LZ4", "Ternario\n(raw)", "Ternario\n residual", "Ternario\n+ Quipu", "Ternario\n+ ZSTD"]
energy = [0.0, -649.3, 18.53, 80.19, 20.59, -43.06, 22.65]
colors = ["#999999", "#ff7f0e", "#2ca02c", "#d62728", "#1f77b4", "#9467bd", "#8c564b"]

fig, ax = plt.subplots(figsize=(8, 4.5))
x = np.arange(len(methods))
bars = ax.bar(x, energy, color=colors, edgecolor="black", linewidth=0.6)
ax.set_xticks(x, methods)
ax.set_ylabel("Ahorro energético vs binario [%]")
ax.set_title("(b) Energía Landauer: ternario raw ahorra 80% (medido)")
ax.axhline(0, color="black", linewidth=0.8, linestyle="--", alpha=0.5)

for i, v in enumerate(energy):
    offset = 2 if v >= 0 else -4
    ax.text(i, v + offset, f"{v:+.1f}%", ha="center", fontsize=9, fontweight="bold")

ax.axhline(50, color="green", linewidth=0.8, linestyle=":", alpha=0.5)
ax.text(6.5, 52, "50% umbral práctico", fontsize=8, ha="right", color="green")

fig.tight_layout()
fig.savefig("fig02_energy_landauer.pdf")
fig.savefig("fig02_energy_landauer.svg")
print("fig02 OK", energy)
