"""
fig02_residual_cycles.py — Detección de ciclos residuales mod-33
Figura 2 del paper
"""
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'experiment'))
from data import generate_sensor_data
from model import ternary_encode, residual_compressor

# Datos
temp = generate_sensor_data()
trits = ternary_encode(temp)
res = residual_compressor(trits, block_size=4)

fig, axes = plt.subplots(3, 1, figsize=(10, 8), gridspec_kw={'height_ratios': [2, 1.5, 1.5]})

# Panel A: Temperatura y trits
ax1 = axes[0]
t_days = np.arange(len(temp)) * (10.0 / 1440.0)
ax1.plot(t_days, temp, color='#1f77b4', linewidth=0.8, alpha=0.8, label='Temperatura (°C)')
ax1.axhline(y=20, color='gray', linestyle='--', linewidth=0.8, alpha=0.5, label='Umbral frío (20°C)')
ax1.axhline(y=25, color='gray', linestyle='--', linewidth=0.8, alpha=0.5, label='Umbral caliente (25°C)')
ax1.fill_between(t_days, 15, 20, alpha=0.1, color='blue')
ax1.fill_between(t_days, 25, 35, alpha=0.1, color='red')
ax1.set_ylabel('Temperatura (°C)', fontsize=10)
ax1.set_title('Detección de ciclos por residuos mod-33', fontsize=13, fontweight='bold')
ax1.legend(fontsize=8, loc='upper right')
ax1.set_xlim(0, t_days[-1])
ax1.grid(True, alpha=0.3)

# Panel B: Trits
ax2 = axes[1]
ax2.plot(t_days, trits, color='#d62728', linewidth=0.5, alpha=0.7)
ax2.fill_between(t_days, trits, 0, alpha=0.3, color='#d62728')
ax2.set_ylabel('Trits', fontsize=10)
ax2.set_yticks([-1, 0, 1])
ax2.set_yticklabels(['-1 (frío)', '0 (normal)', '+1 (caliente)'])
ax2.set_xlim(0, t_days[-1])
ax2.grid(True, alpha=0.3)

# Panel C: Residuos mod-33
ax3 = axes[2]
block_size = 4
n_blocks = len(trits) // block_size
residues = res['residues']
t_blocks = np.arange(n_blocks) * block_size * (10.0 / 1440.0)
ax3.bar(t_blocks, residues, width=block_size * (10.0/1440.0) * 0.9, color='#2ca02c', alpha=0.7)

# Marcar ciclos detectados
colors_cycle = ['#ff7f0e', '#e377c2', '#9467bd', '#8c564b', '#bcbd22', '#17becf']
for i, ci in enumerate(res.get('cycle_info', [])):
    r = ci['residue']
    period = ci['period']
    mask = residues == r
    ax3.scatter(t_blocks[mask], residues[mask], s=40, c=colors_cycle[i % len(colors_cycle)],
                zorder=5, edgecolors='black', linewidths=0.5,
                label=f'Res={r}, T={period}')

ax3.set_ylabel('Residuo mod-33', fontsize=10)
ax3.set_xlabel('Tiempo (días)', fontsize=10)
ax3.set_xlim(0, t_days[-1])
ax3.legend(fontsize=7, ncol=3, loc='upper right')
ax3.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig('../figures/fig02_residual_cycles.pdf', dpi=300, bbox_inches='tight')
plt.savefig('../figures/fig02_residual_cycles.svg', bbox_inches='tight')
print("fig02_residual_cycles: OK")
