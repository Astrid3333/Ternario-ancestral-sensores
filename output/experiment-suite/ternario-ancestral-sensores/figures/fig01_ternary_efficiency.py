"""
fig01_ternary_efficiency.py — Eficiencia de codificación: ln(B)/B
Figura 1 del paper
"""
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import math

# Datos de eficiencia (de Landauer ternary tool)
bases = np.array([2, 3, 4, 5, 6, 7, 8, 9, 10, 16])
efficiency = np.array([math.log(b)/b for b in bases])
e_optimal = math.exp(1)

# Anotaciones culturales
annotations = {
    2: ('Binario\n(Digital)', 'gray'),
    3: ('Ternario\n(Setun/Rusia)', '#d62728'),
    5: ('Maya\n(Vigesimal)', '#1f77b4'),
    60: None,  # no cabe en el gráfico
}

fig, ax = plt.subplots(figsize=(8, 5))

# Curva continua
b_cont = np.linspace(1.5, 16, 300)
e_cont = np.array([math.log(b)/b for b in b_cont])
ax.plot(b_cont, e_cont, 'k-', linewidth=1.5, zorder=1)

# Puntos discretos
for b in bases:
    color = '#d62728' if b == 3 else '#1f77b4' if b == 2 else '#2ca02c' if b in [5, 6, 8] else 'gray'
    size = 120 if b == 3 else 60
    ax.scatter(b, math.log(b)/b, s=size, c=color, zorder=3, edgecolors='black', linewidths=0.5)

# Línea óptima e
ax.axvline(x=e_optimal, color='#ff7f0e', linestyle='--', linewidth=1.2, alpha=0.7, label=f'Óptimo teórico (e ≈ {e_optimal:.2f})')

# Anotaciones
ax.annotate('Ternario\n(máximo práctico)', xy=(3, math.log(3)/3),
            xytext=(5.5, 0.375), fontsize=9, fontweight='bold', color='#d62728',
            arrowprops=dict(arrowstyle='->', color='#d62728', lw=1.5),
            bbox=dict(boxstyle='round,pad=0.3', facecolor='#fff0f0', edgecolor='#d62728'))

ax.annotate('Binario\n(8-bit)', xy=(2, math.log(2)/2),
            xytext=(2.2, 0.31), fontsize=8, color='gray',
            arrowprops=dict(arrowstyle='->', color='gray', lw=1))

ax.set_xlabel('Base (B)', fontsize=11)
ax.set_ylabel('Eficiencia: ln(B)/B', fontsize=11)
ax.set_title('Eficiencia de codificación por base numérica', fontsize=13, fontweight='bold')
ax.legend(fontsize=9, loc='upper right')
ax.set_xlim(1, 16)
ax.set_ylim(0.22, 0.40)
ax.grid(True, alpha=0.3)

# Valores numérico
ax.xaxis.set_major_locator(ticker.MultipleLocator(1))
ax.yaxis.set_major_formatter(ticker.FormatStrFormatter('%.3f'))

plt.tight_layout()
plt.savefig('../figures/fig01_ternary_efficiency.pdf', dpi=300, bbox_inches='tight')
plt.savefig('../figures/fig01_ternary_efficiency.svg', bbox_inches='tight')
print("fig01_ternary_efficiency: OK")
