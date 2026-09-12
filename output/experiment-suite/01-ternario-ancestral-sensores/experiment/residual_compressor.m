function result = residual_compressor(trits, block_size)
% RESIDUAL_COMPRESSOR  Compresión por residuos mod-33
%   result = residual_compressor(trits) — detecta ciclos usando mod-33
%   Inspiración: ciclo persa de 33 años (calendario perpetuo)
%
%   Método:
%   1. Dividir secuencia de trits en bloques de 4
%   2. Calcular residuo mod-33 de cada bloque
%   3. Detectar repeticiones de residuos (= patrones cíclicos)
%   4. Comprimir secuencias con run-length encoding ternario

  if nargin < 2
    block_size = 4;
  end

  n_trits = length(trits);
  n_blocks = floor(n_trits / block_size);

  if n_blocks == 0
    result.compressed = trits;
    result.residues = [];
    result.cycles_detected = 0;
    result.compression_ratio = 1.0;
    return;
  end

  % Calcular residuos mod-33 de cada bloque
  residues = zeros(1, n_blocks);
  for i = 1:n_blocks
    block = trits((i-1)*block_size + 1 : i*block_size);
    % Convertir bloque a valor decimal (base 3)
    val = 0;
    for j = 1:block_size
      val = val + block(j) * 3^(block_size - j);
    end
    % Residuo mod-33
    residues(i) = mod(val, 33);
  end

  % Detectar ciclos: buscar repeticiones de residuos
  unique_residues = unique(residues);
  cycle_count = 0;
  cycle_info = {};

  for r = 1:length(unique_residues)
    positions = find(residues == unique_residues(r));
    if length(positions) >= 2
      % Calcular distancia entre apariciones
      diffs = diff(positions);
      if all(diffs == diffs(1)) && diffs(1) > 0
        % Ciclo regular detectado
        cycle_count = cycle_count + 1;
        cycle_info{cycle_count}.residue = unique_residues(r);
        cycle_info{cycle_count}.period = diffs(1);
        cycle_info{cycle_count}.occurrences = length(positions);
      end
    end
  end

  % Run-length encoding ternario sobre residuos
  compressed_residues = [];
  i = 1;
  while i <= length(residues)
    current = residues(i);
    count = 1;
    while i + count <= length(residues) && residues(i + count) == current
      count = count + 1;
    end
    % Codificar: [residuo, conteo] usando ternario
    compressed_residues = [compressed_residues, current, count];
    i = i + count;
  end

  % Ratio de compresión
  original_bits = n_trits * log2(3);
  compressed_bits = length(compressed_residues) * log2(33);
  compression_ratio = compressed_bits / original_bits;

  result.compressed = compressed_residues;
  result.residues = residues;
  result.cycles_detected = cycle_count;
  result.cycle_info = cycle_info;
  result.compression_ratio = compression_ratio;
  result.n_blocks = n_blocks;
  result.block_size = block_size;

end
