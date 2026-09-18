function result = quipu_checksum(data_trits, mode)
% QUIPU_CHECKSUM  Checksum redundante inspirado en quipu inca
%   result = quipu_checksum(data_trits, 'encode') — genera paridad
%   result = quipu_checksum(codeword, 'decode') — verifica integridad
%
%   Inspiración: quipu — nudos grandes (+1), pequeños (-1), sin nudo (0)
%   Método: paridad ternaria por posición (GF(3))
%
%   Para cada bloque de 5 trits, se generan 2 trits de paridad:
%   p1 = -(d1 + d2 + d3) mod 3 (balanceado)
%   p2 = -(d1 + d2*d3) mod 3 (producto cruz)

  if nargin < 2
    mode = 'encode';
  end

  % Función para balancear trits: 0,1,2 → -1,0,+1
  function balanced = balance_trits(raw)
    balanced = raw;
    balanced(raw == 2) = -1;
  end

  % Función para mapear a {0,1,2}
  function raw = unbalance_trits(balanced)
    raw = balanced;
    raw(balanced == -1) = 2;
  end

  if strcmp(mode, 'encode')
    % Codificar: generar checksum
    n = length(data_trits);
    block_size = 5;
    n_blocks = floor(n / block_size);

    checksum = [];
    for b = 1:n_blocks
      block = data_trits((b-1)*block_size + 1 : b*block_size);
      p1_raw = mod(-sum(unbalance_trits(block)), 3);
      prod = 1;
      for i = 1:block_size
        prod = prod * unbalance_trits(block(i));
      end
      p2_raw = mod(-prod, 3);
      checksum = [checksum, balance_trits(p1_raw), balance_trits(p2_raw)];
    end

    result.codeword = [data_trits(1:n_blocks*block_size), checksum];
    result.checksum = checksum;
    result.n_data = n_blocks * block_size;
    result.n_checksum = length(checksum);
    result.block_size = block_size;
    result.n_blocks = n_blocks;

  elseif strcmp(mode, 'decode')
    % Estructura del codeword: [data1..dataN, checksum1..checksumN*2]
    % Donde checksum = [p1_b1, p2_b1, p1_b2, p2_b2, ...]
    n = length(data_trits);
    block_size = 5;
    n_blocks = floor(n / (block_size + 2));

    errors = [];
    syndromes = [];

    for b = 1:n_blocks
      % Extraer datos del bloque b
      data_start = (b-1) * block_size + 1;
      block = data_trits(data_start : data_start + block_size - 1);

      % Extraer paridad del bloque b (del final del codeword)
      checksum_start = n_blocks * block_size;
      parity = data_trits(checksum_start + (b-1)*2 + 1 : checksum_start + b*2);

      p1_raw = mod(-sum(unbalance_trits(block)), 3);
      prod = 1;
      for i = 1:block_size
        prod = prod * unbalance_trits(block(i));
      end
      p2_raw = mod(-prod, 3);

      s1 = mod(unbalance_trits(parity(1)) - p1_raw, 3);
      s2 = mod(unbalance_trits(parity(2)) - p2_raw, 3);

      syndromes = [syndromes; s1, s2];
      if s1 ~= 0 || s2 ~= 0
        errors = [errors, b];
      end
    end

    result.valid = isempty(errors);
    result.errors = errors;
    result.syndromes = syndromes;
    result.n_blocks = n_blocks;
    result.block_size = block_size;

  else
    error('Modo desconocido: use encode o decode');
  end

end
