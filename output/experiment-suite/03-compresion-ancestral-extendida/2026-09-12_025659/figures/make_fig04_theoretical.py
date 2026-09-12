"""Fig4: Análisis teórico — equivalentes en trits por base ancestral."""
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

plt.rcParams.update({"font.size": 10, "axes.grid": True, "grid.alpha": 0.3,
                     "svg.fonttype": "none"})

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10, 4.5))

# Panel izquierdo: Equivalentes en bits
bases = [3, 10, 20, 33, 60]
bits = [np.log2(b) for b in bases]
labels = ["Ternario\n(base 3)", "Decimal\n(base 10)", "Maya\n(base 20)", "Persa\n(base 33)", "Babilónico\n(base 60)"]
colors = ["#999999", "#ff7f0e", "#1f77b4", "#ff7f0e", "#d62728"]

x = np.arange(len(bases))
bars1 = ax1.bar(x, bits, color=colors, edgecolor="black", linewidth=0.6)
ax1.set_xticks(x, labels, fontsize=8)
ax1.set_ylabel("Bits por dígito (log₂(base))")
ax1.set_title("(d) Equivalentes: un dígito babilónico = 5.91 bits")
for i, v in enumerate(bits):
    ax1.text(i, v + 0.1, f"{v:.2f}", ha="center", fontsize=9)

# Panel derecho: Ahorro vs ternario raw
savings = [0, 0, 63.0, 68.2, 74.3]  # % de ahorro en espacio
colors_sav = ["#999999", "#ff7f0e", "#1f77b4", "#ff7f0e", "#d62728"]

bars2 = ax2.bar(x, savings, color=colors_sav, edgecolor="black", linewidth=0.6)
ax2.set_xticks(x, labels, fontsize=8)
ax2.set_ylabel("Ahorro vs ternario raw [%]")
ax2.set_title("(e) Ahorro: babilónico ahorra 74% de espacio")
for i, v in enumerate(savings):
    if v > 0:
        ax2.text(i, v + 1, f"{v:.1f}%", ha="center", fontsize=9, fontweight="bold")

fig.tight_layout()
fig.savefig("fig04_theoretical_analysis.pdf")
fig.savefig("fig04_theoretical_analysis.svg")
print("fig04 OK")
