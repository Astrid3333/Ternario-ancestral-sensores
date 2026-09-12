"""
data.py — Generador de datos sintéticos de sensores IoT
Réplica Python del experimento Octave
"""
import numpy as np
import os


def generate_sensor_data(n_samples=1006, seed=42):
    """
    Genera datos de temperatura simulada con ciclo diario y ruido gaussiano.
    Fallback si no existe sensor_data.csv desde Octave.
    """
    # Intentar cargar datos de Octave primero
    csv_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "sensor_data.csv")
    if os.path.exists(csv_path):
        import csv
        temps = []
        with open(csv_path, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                temps.append(float(row['temperature']))
        return np.array(temps)

    # Fallback: generar con Python (RNG distinto a Octave)
    rng = np.random.default_rng(seed)
    t = np.arange(n_samples) * (10.0 / 1440.0)
    temp_base = 25.0 + 5.0 * np.sin(2.0 * np.pi * t)
    noise = 0.5 * rng.standard_normal(n_samples)
    temperature = temp_base + noise
    temperature = np.clip(temperature, 15.0, 35.0)
    return temperature


if __name__ == "__main__":
    temp = generate_sensor_data()
    print(f"Samples: {len(temp)}, Range: [{temp.min():.1f}, {temp.max():.1f}] °C")
    print(f"Distribution: cold={np.sum(temp < 20)}, normal={np.sum((temp >= 20) & (temp <= 25))}, hot={np.sum(temp > 25)}")
