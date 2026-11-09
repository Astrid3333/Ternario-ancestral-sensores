% run_all.m  —  Motor Ternario Ancestral para Sensores IoT
% Ejecuta todo el pipeline: codec → compresión → checksum → benchmark
% Modo: MEDIDO (resultados reales de ejecución)

fprintf('=== MOTOR TERNARIO ANCESTRAL PARA SENSORES IoT ===\n');
fprintf('Fecha: %s\n\n', datestr(now));

% --- 1. Generar datos sintéticos de sensores ---
fprintf('--- 1. Generando datos de sensores ---\n');
n_samples = 1006;  % 7 días × 144 lecturas/día (cada 10 min)
t = (0:n_samples-1) * (10/1440);  % días
% Temperatura: ciclo diario ± 5°C, ruido gaussiano
temp_base = 25 + 5 * sin(2 * pi * t);  % ciclo diario
rng(42);  % semilla fija para reproducibilidad
noise = 0.5 * randn(1, n_samples);
temperature = temp_base + noise;
temperature = max(15, min(35, temperature));  % clamp 15-35°C

fprintf('  Muestras: %d, Rango: [%.1f, %.1f] °C\n', n_samples, min(temperature), max(temperature));

% --- 2. Codificación ternaria ---
fprintf('\n--- 2. Codificando en ternario ---\n');
trits_result = ternary_codec(temperature, 'encode');
fprintf('  Trits generados: %d\n', trits_result.n_trits);

% Verificar: contar distribución de trits
n_cold = sum(trits_result.trits == -1);
n_normal = sum(trits_result.trits == 0);
n_hot = sum(trits_result.trits == 1);
fprintf('  Distribución: frío=%d, normal=%d, caliente=%d\n', n_cold, n_normal, n_hot);

% --- 3. Compresión residual ---
fprintf('\n--- 3. Compresión por residuos mod-33 ---\n');
residual_result = residual_compressor(trits_result.trits, 4);
fprintf('  Bloques procesados: %d\n', residual_result.n_blocks);
fprintf('  Residuos únicos: %d\n', length(unique(residual_result.residues)));
fprintf('  Ciclos detectados: %d\n', residual_result.cycles_detected);
fprintf('  Ratio de compresión: %.3f\n', residual_result.compression_ratio);

if residual_result.cycles_detected > 0
  fprintf('  Ciclos encontrados:\n');
  for c = 1:length(residual_result.cycle_info)
    ci = residual_result.cycle_info{c};
    fprintf('    Residuo %d: periodo=%d, ocurrencias=%d\n', ...
      ci.residue, ci.period, ci.occurrences);
  end
end

% --- 4. Checksum quipu ---
fprintf('\n--- 4. Checksum tipo quipu ---\n');
block_size = 5;
n_blocks_checksum = floor(length(trits_result.trits) / block_size);
if n_blocks_checksum > 0
  data_for_checksum = trits_result.trits(1:n_blocks_checksum * block_size);
  quipu_result = quipu_checksum(data_for_checksum, 'encode');
  fprintf('  Bloques de paridad: %d\n', quipu_result.n_blocks);
  fprintf('  Trits de datos: %d\n', quipu_result.n_data);
  fprintf('  Trits de checksum: %d\n', quipu_result.n_checksum);
  fprintf('  Total codeword: %d trits\n', length(quipu_result.codeword));

  % Verificar checksum
  verify = quipu_checksum(quipu_result.codeword, 'decode');
  fprintf('  Verificación: %s\n', mat2str(verify.valid));
  fprintf('  Errores detectados: %d\n', length(verify.errors));
end

% --- 5. Benchmark ---
fprintf('\n--- 5. Benchmark: comparación de métodos ---\n');
bench = benchmark(temperature);

fprintf('  Método                 | Bits   | Ratio  | Energía (J)\n');
fprintf('  -----------------------|--------|--------|------------\n');
for i = 1:size(bench.methods, 1)
  fprintf('  %-22s | %6d | %.3f  | %e\n', ...
    bench.methods{i,1}, bench.methods{i,2}, bench.methods{i,3}, ...
    bench.methods{i,2} * 1.380649e-23 * 300 * log(2));
end

fprintf('\n  Energía ternario vs binario:\n');
fprintf('    Binario: %e J\n', bench.energy_binary);
fprintf('    Ternario: %e J\n', bench.energy_ternary);
fprintf('    Ahorro: %.1f%%\n', bench.energy_saving * 100);

% --- 6. Verificación H4 (Landauer) ---
% Landauer: E_min = kB*T*ln(S) para borrar un símbolo de S estados equiprobables.
% Ojo con las unidades: un trit vale log2(3) = 1.585 bits, así que "por símbolo"
% y "por bit" NO son la misma magnitud. Se reportan las dos por separado y con
% el modelo explícito; comparar ln(S)/S (memoria multinivel: se borra un nivel
% entre S) contra ln(S) (Landauer clásico) mezclaba dos modelos distintos.
fprintf('\n--- 6. Verificación Landauer (H4) ---\n');
kB = 1.380649e-23;
T = 300;
E_binary = kB * T * log(2);          % por bit (S = 2)
E_ternary = kB * T * log(3);         % por trit (S = 3)
bits_per_symbol_binary = log2(2);
bits_per_symbol_ternary = log2(3);

fprintf('  [Landauer clásico: E = kB*T*ln(S)]\n');
fprintf('  Energía por bit  (binario): %e J\n', E_binary);
fprintf('  Energía por trit (ternario): %e J\n', E_ternary);
fprintf('  Coste por símbolo: ternario/binario = %.3f (el trit gasta %.1f%% más energía por símbolo)\n', ...
        E_ternary / E_binary, (E_ternary / E_binary - 1) * 100);

fprintf('  [Por bit de información: E/bit = E/log2(S)]\n');
e_per_bit_binary = E_binary / bits_per_symbol_binary;
e_per_bit_ternary = E_ternary / bits_per_symbol_ternary;
fprintf('  Energía por bit (binario): %e J/bit\n', e_per_bit_binary);
fprintf('  Energía por bit (ternario): %e J/bit\n', e_per_bit_ternary);
fprintf('  Diferencia por bit: %.1f%% (Landauer clásico da el MISMO coste por bit)\n', ...
        (e_per_bit_ternary - e_per_bit_binary) / e_per_bit_binary * 100);

fprintf('  [Memoria multinivel: E = kB*T*ln(S)/S, se borra un nivel entre S]\n');
e_ml_binary = kB * T * log(2) / 2;
e_ml_ternary = kB * T * log(3) / 3;
e_ml_bit_binary = e_ml_binary / bits_per_symbol_binary;
e_ml_bit_ternary = e_ml_ternary / bits_per_symbol_ternary;
fprintf('  Energía por símbolo (binario): %e J\n', e_ml_binary);
fprintf('  Energía por símbolo (ternario): %e J\n', e_ml_ternary);
fprintf('  Coste por símbolo: ternario es %.1f%% más caro que el binario\n', ...
        (e_ml_ternary - e_ml_binary) / e_ml_binary * 100);
fprintf('  Ahorro por bit:    ternario es %.1f%% más barato que el binario\n', ...
        (e_ml_bit_binary - e_ml_bit_ternary) / e_ml_bit_binary * 100);
fprintf('  Nota: el ahorro global de energía (%0.1f%%) viene de la compresión, no del modelo termodinámico.\n', ...
        bench.energy_saving * 100);

% --- 7. Guardar resultados ---
fprintf('\n--- 7. Guardando resultados ---\n');
results.mode = 'measured';
results.date = datestr(now);
results.n_samples = n_samples;
results.temperature_range = [min(temperature), max(temperature)];
results.ternary.n_trits = trits_result.n_trits;
results.ternary.distribution = [n_cold, n_normal, n_hot];
results.residual.n_blocks = residual_result.n_blocks;
results.residual.cycles_detected = residual_result.cycles_detected;
results.residual.compression_ratio = residual_result.compression_ratio;
results.residual.unique_residues = length(unique(residual_result.residues));
results.quipu.n_blocks = n_blocks_checksum;
results.quipu.total_codeword = length(quipu_result.codeword);
results.quipu.valid = verify.valid;
results.benchmark.binary_bits = bench.binary_bits;
results.benchmark.ternary_bits = bench.ternary_bits;
results.benchmark.ternary_quipu_bits = bench.ternary_quipu_bits;
results.benchmark.delta_bits = bench.delta_bits;
results.benchmark.gzip_bits = bench.gzip_bits;
results.benchmark.energy_binary = bench.energy_binary;
results.benchmark.energy_ternary = bench.energy_ternary;
results.benchmark.energy_saving_pct = bench.energy_saving * 100;
results.landauer.E_binary = E_binary;
results.landauer.E_ternary = E_ternary;
results.landauer.efficiency_ratio = log(3)/log(2);
results.landauer.ln3_over_3 = log(3)/3;
results.landauer.ln2_over_2 = log(2)/2;
results.landauer.advantage_pct = (log(3)/3 - log(2)/2) / (log(2)/2) * 100;

% Guardar JSON
fid = fopen('results.json', 'w');
fprintf(fid, '{\n');
fprintf(fid, '  "mode": "measured",\n');
fprintf(fid, '  "date": "%s",\n', datestr(now));
fprintf(fid, '  "n_samples": %d,\n', n_samples);
fprintf(fid, '  "temperature_range": [%.2f, %.2f],\n', min(temperature), max(temperature));
fprintf(fid, '  "ternary": {\n');
fprintf(fid, '    "n_trits": %d,\n', trits_result.n_trits);
fprintf(fid, '    "distribution": [%d, %d, %d]\n', n_cold, n_normal, n_hot);
fprintf(fid, '  },\n');
fprintf(fid, '  "residual": {\n');
fprintf(fid, '    "n_blocks": %d,\n', residual_result.n_blocks);
fprintf(fid, '    "cycles_detected": %d,\n', residual_result.cycles_detected);
fprintf(fid, '    "compression_ratio": %.6f,\n', residual_result.compression_ratio);
fprintf(fid, '    "unique_residues": %d\n', length(unique(residual_result.residues)));
fprintf(fid, '  },\n');
fprintf(fid, '  "quipu": {\n');
fprintf(fid, '    "n_blocks": %d,\n', n_blocks_checksum);
fprintf(fid, '    "total_codeword": %d,\n', length(quipu_result.codeword));
fprintf(fid, '    "valid": %d\n', verify.valid);
fprintf(fid, '  },\n');
fprintf(fid, '  "benchmark": {\n');
fprintf(fid, '    "binary_bits": %d,\n', bench.binary_bits);
fprintf(fid, '    "ternary_bits": %d,\n', bench.ternary_bits);
fprintf(fid, '    "ternary_quipu_bits": %d,\n', bench.ternary_quipu_bits);
fprintf(fid, '    "delta_bits": %d,\n', bench.delta_bits);
fprintf(fid, '    "gzip_bits": %d,\n', bench.gzip_bits);
fprintf(fid, '    "energy_binary_J": %e,\n', bench.energy_binary);
fprintf(fid, '    "energy_ternary_J": %e,\n', bench.energy_ternary);
fprintf(fid, '    "energy_saving_pct": %.2f\n', bench.energy_saving * 100);
fprintf(fid, '  },\n');
fprintf(fid, '  "landauer": {\n');
fprintf(fid, '    "E_binary_J": %e,\n', E_binary);
fprintf(fid, '    "E_ternary_J": %e,\n', E_ternary);
fprintf(fid, '    "efficiency_ratio": %.6f,\n', log(3)/log(2));
fprintf(fid, '    "ln3_over_3": %.6f,\n', log(3)/3);
fprintf(fid, '    "ln2_over_2": %.6f,\n', log(2)/2);
fprintf(fid, '    "advantage_pct": %.4f\n', (log(3)/3 - log(2)/2) / (log(2)/2) * 100);
fprintf(fid, '  }\n');
fprintf(fid, '}\n');
fclose(fid);

fprintf('  results.json guardado.\n');
fprintf('\n=== TODO MEDIDO OK ===\n');
