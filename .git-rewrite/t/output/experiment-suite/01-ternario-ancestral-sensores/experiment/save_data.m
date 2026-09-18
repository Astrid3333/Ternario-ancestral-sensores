% save_data.m — Guarda datos sintéticos para réplica Python
n_samples = 1006;
t = (0:n_samples-1) * (10/1440);
temp_base = 25 + 5 * sin(2 * pi * t);
rng(42);
noise = 0.5 * randn(1, n_samples);
temperature = temp_base + noise;
temperature = max(15, min(35, temperature));

% Guardar como CSV
fid = fopen('sensor_data.csv', 'w');
fprintf(fid, 'temperature\n');
for i = 1:n_samples
  fprintf(fid, '%.6f\n', temperature(i));
end
fclose(fid);

% Guardar como JSON
fid = fopen('sensor_data.json', 'w');
fprintf(fid, '[ ');
for i = 1:n_samples
  if i < n_samples
    fprintf(fid, '%.6f, ', temperature(i));
  else
    fprintf(fid, '%.6f ', temperature(i));
  end
end
fprintf(fid, ']\n');
fclose(fid);

fprintf('Guardado: sensor_data.csv, sensor_data.json\n');
fprintf('Rango: [%.4f, %.4f]\n', min(temperature), max(temperature));
