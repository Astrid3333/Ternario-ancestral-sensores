"""Fig4: ablación — con vs sin intercalación (deriva en 52 años, días) — MEDIDO."""
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

plt.rcParams.update({"font.size": 10, "axes.grid": True, "grid.alpha": 0.3,
                     "svg.fonttype": "none"})
T = 365.24219
cats = ["Persa\nciclo-33", "Persa\nsin bisiestos*", "Gregoriano", "Gregoriano\nsin bisiestos*",
        "Haab\n(365 exacto)"]
means = [12053 / 33, 365.0, 365.2425, 365.0, 365.0]
drift52 = [(m - T) * 52 for m in means]
colors = ["#d62728", "#ff9896", "#2ca02c", "#98df8a", "#1f77b4"]
fig, ax = plt.subplots(figsize=(7.5, 4.4))
x = np.arange(len(cats))
ax.bar(x, drift52, color=colors, edgecolor="black", linewidth=0.6)
ax.set_xticks(x, cats)
ax.set_ylabel("Deriva acumulada en 52 años [días]")
ax.set_title("(d) Ablación: quitar la intercalación colapsa persa y gregoriano al Haab (medido)")
for i, v in enumerate(drift52):
    ax.text(i, v + (0.15 if v >= 0 else -0.35), f"{v:+.2f} d", ha="center", fontsize=9)
fig.text(0.01, 0.01, "* Ablación hipotética: 365 exacto. El ciclo-33 persa es aproximación; "
           "la regla real es astronómica (Teherán).", fontsize=7.5, ha="left")
fig.tight_layout(rect=[0, 0.04, 1, 1])
fig.savefig("fig04_ablation_no_leap.pdf")
fig.savefig("fig04_ablation_no_leap.svg")
print("fig04 OK", [round(v, 3) for v in drift52])
