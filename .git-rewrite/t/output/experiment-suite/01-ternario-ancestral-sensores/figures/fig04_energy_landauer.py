"""
fig04_energy_landauer.py — Consumo energético (Landauer) por símbolo
Figura 4 del paper
"""
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import math

kB = 1.380649e-23  # J/K
T = 300.0  # K

bases = np.array([2, 3, 4, 5, 8, 10, 16])
energies = np.array([kB * T * math.log(b) for b in bases])
efficiencies = np.array([math.log(b)/b for b in bases])
labels = ['Binario\n(B=2)', 'Ternario\n(B=3)', 'Cuarto\n(B=4)', 'Quinto\n(B=5)',
          'Octal\n(B=8)', 'Decimal\n(B=10)', 'Hex\n(B=16)']

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

# Panel A: Energía por símbolo
colors = ['#aec7e8', '#d62728', '#ffbb78', '#ff9896', '#c5b0d5', '#c7c7c7', '#98df8a']
bars = ax1.bar(labels, energies / 1e-21, color=colors, edgecolor='black', linewidth=0.5)
ax1.set_ylabel('Energía por símbolo (×10⁻²¹ J)', fontsize=10)
ax1.set_title('Límite de Landauer por base', fontsize=12, fontweight='bold')
ax1.grid(True, axis='y', alpha=0.3)

# Anotar valores
for bar, e in zip(bars, energies):
    ax1.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.1,
             f'{e/1e-21:.2f}', ha='center', fontsize=8)

# Resaltar ternario
bars[1].set_edgecolor('#d62728')
bars[1].set_linewidth(2.5)

# Panel B: Eficiencia (ln(B)/B) — energía por bit de información
ax2.plot(bases, efficiencies, 'ko-', linewidth=2, markersize=8, zorder=2)
ax2.scatter([3], [math.log(3)/3], s=200, c='#d62728', zorder=3, edgecolors='black',
            linewidths=1.5, label='Ternario (máximo práctico)')

# Línea óptima
e_opt = math.exp(1)
ax2.axvline(x=e_opt, color='#ff7f0e', linestyle='--', linewidth=1.2, alpha=0.7,
            label=f'Óptimo teórico (e ≈ {e_opt:.2f})')

ax2.set_xlabel('Base (B)', fontsize=10)
ax2.set_ylabel('Eficiencia: ln(B)/B', fontsize=10)
ax2.set_title('Eficiencia energética por base', fontsize=12, fontweight='bold')
ax2.legend(fontsize=9)
ax2.grid(True, alpha=0.3)
ax2.set_xlim(1, 17)

# Ahorro ternario vs binario
e_bin = kB * T * math.log(2)
e_ter = kB * T * math.log(3)
saving = (1 - e_ter / e_bin) * 100

fig.suptitle(f'Ahorro ternario vs binario: {saving:.1f}% menos energía por operación',
             fontsize=11, fontweight='bold', y=1.02)

plt.tight_layout()
plt.savefig('../figures/fig04_energy_landauer.pdf', dpi=300, bbox_inches='tight')
plt.savefig('../figures/fig04_energy_landauer.svg', bbox_inches='tight')
print("fig04_energy_landauer: OK")
