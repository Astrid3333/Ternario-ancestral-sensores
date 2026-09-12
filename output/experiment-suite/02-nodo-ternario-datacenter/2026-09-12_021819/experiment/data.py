"""data.py — Generación de datos de sensores sintéticos + carga OpenWeatherMap."""

import numpy as np
import csv
import os

# Constantes del run (réplica de config.yaml)
BOLTZMANN_K = 1.380649e-23
TEMPERATURE_K = 300
TERNARY_LOW_C = 20.0
TERNARY_HIGH_C = 25.0
BLOCK_SIZE = 4
QUIPU_BLOCK_SIZE = 5
MODULUS = 33


def generate_sensor_data(n_samples=1006, base_temp=22.0, amplitude=5.0,
                         noise_std=0.5, seed=42):
    """Genera datos sintéticos de temperatura con ciclo diario."""
    rng = np.random.default_rng(seed)
    t = np.arange(n_samples) * (10 / (24 * 60))  # fracción de día
    daily_cycle = amplitude * np.sin(2 * np.pi * t - np.pi / 2)
    noise = rng.normal(0, noise_std, n_samples)
    temperature = base_temp + daily_cycle + noise
    return temperature


def sensor_to_trit(value, low=TERNARY_LOW_C, high=TERNARY_HIGH_C):
    """Convierte un valor de sensor a trit balanceado."""
    if value < low:
        return -1
    elif value > high:
        return 1
    else:
        return 0


def trits_from_temps(temps, low=TERNARY_LOW_C, high=TERNARY_HIGH_C):
    """Convierte array de temperaturas a array de trits."""
    return np.array([sensor_to_trit(t, low, high) for t in temps])


def save_sensor_data(temps, filepath):
    """Guarda datos de temperatura en CSV."""
    os.makedirs(os.path.dirname(filepath), exist_ok=True)
    with open(filepath, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['sample', 'temperature_c'])
        for i, t in enumerate(temps):
            writer.writerow([i, f"{t:.3f}"])


def load_sensor_data(filepath):
    """Carga datos de temperatura desde CSV."""
    temps = []
    with open(filepath, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            temps.append(float(row['temperature_c']))
    return np.array(temps)
