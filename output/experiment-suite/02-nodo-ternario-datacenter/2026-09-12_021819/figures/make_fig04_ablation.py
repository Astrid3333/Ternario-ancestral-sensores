"""Fig4: Ablación — contribución de cada componente — MEDIDO."""
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

plt.rcParams.update({"font.size": 10, "axes.grid": True, "grid.alpha": 0.3,
                     "svg.fonttype": "none"})

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10, 4.5))

# Panel izquierdo: Compresión por ablación
ablaciones = ["Sin residual\n(A1)", "Sin quipu\n(A2)", "Binario puro\n(A3)", "Full ternario"]
compresion = [8.0, 1.996, 1.0, 1.996]
colors_comp = ["#ff9896", "#1f77b4", "#999999", "#d62728"]

x = np.arange(len(ablaciones))
bars1 = ax1.bar(x, compresion, color=colors_comp, edgecolor="black", linewidth=0.6)
ax1.set_xticks(x, ablaciones)
ax1.set_ylabel("Ratio de compresión")
ax1.set_title("(e) Ablación compresión: residual es esencial")
for i, v in enumerate(compresion):
    ax1.text(i, v + 0.1, f"{v:.2f}x", ha="center", fontsize=9, fontweight="bold")

# Panel derecho: Energía por ablación
energia = [80.19, 20.59, 0.0, 20.59]
colors_energy = ["#ff9896", "#1f77b4", "#999999", "#d62728"]

bars2 = ax2.bar(x, energia, color=colors_energy, edgecolor="black", linewidth=0.6)
ax2.set_xticks(x, ablaciones)
ax2.set_ylabel("Ahorro energético [%]")
ax2.set_title("(f) Ablación energía: ternario raw maximiza ahorro")
for i, v in enumerate(energia):
    ax2.text(i, v + 1, f"{v:.1f}%", ha="center", fontsize=9, fontweight="bold")

fig.tight_layout()
fig.savefig("fig04_ablation.pdf")
fig.savefig("fig04_ablation.svg")
print("fig04 OK")
