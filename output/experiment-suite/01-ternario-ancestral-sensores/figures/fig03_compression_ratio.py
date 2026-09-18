"""
fig03_compression_ratio.py — Ratio de compresión: métodos comparados
Figura 3 del paper
"""
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'experiment'))
from data import generate_sensor_data
from model import benchmark

temp = generate_sensor_data()
bench = benchmark(temp)

methods = [m[0] for m in bench['methods']]
ratios = [m[2] for m in bench['methods']]
bits = [m[1] for m in bench['methods']]

# Colores
colors = ['#aec7e8', '#ffbb78', '#ff9896', '#c5b0d5', '#c7c7c7']

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

# Panel A: Ratio de compresión
bars = ax1.barh(methods, ratios, color=colors, edgecolor='black', linewidth=0.5)
ax1.set_xlabel('Ratio de compresión (menor = mejor)', fontsize=10)
ax1.set_title('Ratio de compresión por método', fontsize=12, fontweight='bold')
ax1.set_xlim(0, 1.1)
ax1.axvline(x=1.0, color='gray', linestyle='--', linewidth=0.8, alpha=0.5)
ax1.grid(True, axis='x', alpha=0.3)

# Anotar valores
for bar, ratio, b in zip(bars, ratios, bits):
    ax1.text(bar.get_width() + 0.02, bar.get_y() + bar.get_height()/2,
             f'{ratio:.3f} ({b:.0f} bits)', va='center', fontsize=9)

# Panel B: Bits absolutos
bars2 = ax2.barh(methods, bits, color=colors, edgecolor='black', linewidth=0.5)
ax2.set_xlabel('Total bits', fontsize=10)
ax2.set_title('Consumo de bits por método', fontsize=12, fontweight='bold')
ax2.grid(True, axis='x', alpha=0.3)

# Anotar valores
for bar, b in zip(bars2, bits):
    ax2.text(bar.get_width() + 50, bar.get_y() + bar.get_height()/2,
             f'{b:.0f}', va='center', fontsize=9)

plt.tight_layout()
plt.savefig('../figures/fig03_compression_ratio.pdf', dpi=300, bbox_inches='tight')
plt.savefig('../figures/fig03_compression_ratio.svg', bbox_inches='tight')
print("fig03_compression_ratio: OK")
