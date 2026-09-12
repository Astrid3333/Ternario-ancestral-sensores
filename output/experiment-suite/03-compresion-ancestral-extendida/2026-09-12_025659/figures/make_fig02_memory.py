"""Fig2: Comparación de memoria por método — MEDIDO."""
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

plt.rcParams.update({"font.size": 10, "axes.grid": True, "grid.alpha": 0.3,
                     "svg.fonttype": "none"})

methods = ["Ternario raw", "Maya", "Persa", "Babilónico", "Maya+residual", "Bab+residual", "Full ancestral"]
memory = [1006, 370, 320, 257, 402, 287, 314]
colors = ["#999999", "#1f77b4", "#ff7f0e", "#d62728", "#9467bd", "#8c564b", "#e377c2"]

fig, ax = plt.subplots(figsize=(9, 4.5))
x = np.arange(len(methods))
bars = ax.bar(x, memory, color=colors, edgecolor="black", linewidth=0.6)
ax.set_xticks(x, methods, fontsize=8, rotation=15)
ax.set_ylabel("Bytes por 1006 trits")
ax.set_title("(b) Memoria: babilónico usa 25% menos que ternario raw (medido)")
ax.axhline(1006, color="black", linewidth=0.8, linestyle="--", alpha=0.5)

for i, v in enumerate(memory):
    ax.text(i, v + 15, f"{v}", ha="center", fontsize=9)

fig.tight_layout()
fig.savefig("fig02_memory_comparison.pdf")
fig.savefig("fig02_memory_comparison.svg")
print("fig02 OK", memory)
