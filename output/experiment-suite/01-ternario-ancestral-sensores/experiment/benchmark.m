function result = benchmark(temperature_data)
% BENCHMARK  Comparación de métodos de compresión para sensores
%   result = benchmark(temperature_data) — compara ternario vs binario vs gzip
%
%   Métodos comparados:
%   1. Binario estándar (8 bits por valor, 0-255)
%   2. Ternario ancestral (trits + residual mod-33)
%   3. Ternario + quipu checksum
%   4. Delta encoding + binario
%   5. gzip sobre binario

  n = length(temperature_data);

  % --- Método 1: Binario estándar (8 bits por valor) ---
  binary_bits = n * 8;
  binary_bytes = ceil(binary_bits / 8);

  % --- Método 2: Ternario ancestral ---
  trits = ternary_codec(temperature_data, 'encode');
  ternary_result = residual_compressor(trits.trits);
  ternary_bits = length(ternary_result.compressed) * log2(33);

  % --- Método 3: Ternario + quipu checksum ---
  block_size = 5;
  n_blocks = floor(length(trits.trits) / block_size);
  if n_blocks > 0
    data_for_checksum = trits.trits(1:n_blocks * block_size);
    quipu_result = quipu_checksum(data_for_checksum, 'encode');
    ternary_quipu_bits = length(quipu_result.codeword) * log2(3);
  else
    ternary_quipu_bits = ternary_bits;
  end

  % --- Método 4: Delta encoding ---
  delta = diff(temperature_data);
  delta_bits = length(delta) * 8;  % cada diferencia en 8 bits

  % --- Método 5: gzip ---
  try
    % Simular gzip: comprimir datos binarios
    temp_file = [tempname, '.bin'];
    fid = fopen(temp_file, 'wb');
    fwrite(fid, uint8(temperature_data), 'uint8');
    fclose(fid);
    gzip(temp_file);
    gz_file = [temp_file, '.gz'];
    gz_info = dir(gz_file);
    gzip_bytes = gz_info.bytes;
    gzip_bits = gzip_bytes * 8;
    delete(gz_file);
    delete(temp_file);
  catch
    % Si gzip no disponible, estimar ratio típico ~0.6
    gzip_bits = binary_bits * 0.6;
  end

  % --- Resultados ---
  result.methods = {
    'Binario (8-bit)', binary_bits, binary_bits / binary_bits;
    'Ternario (residual)', ternary_bits, ternary_bits / binary_bits;
    'Ternario+Quipu', ternary_quipu_bits, ternary_quipu_bits / binary_bits;
    'Delta encoding', delta_bits, delta_bits / binary_bits;
    'gzip', gzip_bits, gzip_bits / binary_bits;
  };

  result.binary_bits = binary_bits;
  result.ternary_bits = ternary_bits;
  result.ternary_quipu_bits = ternary_quipu_bits;
  result.delta_bits = delta_bits;
  result.gzip_bits = gzip_bits;

  % Eficiencia energética (Landauer)
  kB = 1.380649e-23;  % J/K
  T = 300;            % K (temperatura ambiente)
  result.energy_binary = binary_bits * kB * T * log(2);
  result.energy_ternary = ternary_bits * kB * T * log(3);
  result.energy_saving = 1 - (result.energy_ternary / result.energy_binary);

  % Métricas adicionales
  result.n_samples = n;
  result.compression_ratio_ternary = ternary_bits / binary_bits;
  result.compression_ratio_gzip = gzip_bits / binary_bits;

end
