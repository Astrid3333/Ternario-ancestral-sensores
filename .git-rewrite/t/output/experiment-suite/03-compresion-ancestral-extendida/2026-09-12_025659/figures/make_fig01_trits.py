"""Fig1: Compresión ancestral teórica — trits por dígito."""
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

plt.rcParams.update({"font.size": 10, "axes.grid": True, "grid.alpha": 0.3,
                     "svg.fonttype": "none"})

bases = ["Ternario\n(raw)", "Maya\n(base 20)", "Persa\n(base 33)", "Babilónico\n(base 60)"]
trits_per_digit = [1.0, 2.72, 3.15, 3.91]
colors = ["#999999", "#1f77b4", "#ff7f0e", "#d62728"]

fig, ax = plt.subplots(figsize=(8, 4.5))
x = np.arange(len(bases))
bars = ax.bar(x, trits_per_digit, color=colors, edgecolor="black", linewidth=0.6)
ax.set_xticks(x, bases)
ax.set_ylabel("Trits por dígito (log₃(base))")
ax.set_title("(a) Compresión ancestral: un símbolo babilónico = 3.91 trits (medido)")
ax.axhline(1.0, color="black", linewidth=0.8, linestyle="--", alpha=0.5)

for i, v in enumerate(trits_per_digit):
    ax.text(i, v + 0.05, f"{v:.2f}", ha="center", fontsize=10, fontweight="bold")

fig.tight_layout()
fig.savefig("fig01_trits_per_digit.pdf")
fig.savefig("fig01_trits_per_digit.svg")
print("fig01 OK", trits_per_digit)
