function result = ternary_codec(data, mode)
% TERNARY_CODEC  Codificador/descodificador ternario balanceado
%   result = ternary_codec(data, 'encode') — convierte valores a trits
%   result = ternary_codec(data, 'decode') — convierte trits a valores
%
%   Inspiración: aritmética ternaria rusa (Setun, 1958)
%   Trits: -1 (frío/bajo), 0 (normal), +1 (caliente/alto)

  if nargin < 2
    mode = 'encode';
  end

  if strcmp(mode, 'encode')
    % Clasificar en 3 estados: -1, 0, +1
    % Umbrales: < 20°C = -1, 20-25°C = 0, > 25°C = +1
    trits = zeros(size(data));
    trits(data < 20) = -1;
    trits(data >= 20 & data <= 25) = 0;
    trits(data > 25) = 1;

    % Para secuencias largas, guardar solo los trits (el decimal overflow)
    n_trits = length(trits);

    result.trits = trits;
    result.n_trits = n_trits;
    result.n_symbols = n_trits;

  elseif strcmp(mode, 'decode')
    % Descodificar número decimal a trits balanceados
    decimal_val = data;
    if decimal_val == 0
      trits = [0];
    else
      n = ceil(log(abs(decimal_val) + 1) / log(3));
      trits = zeros(1, n);
      remaining = decimal_val;
      for i = n:-1:1
        trit = mod(remaining + 1, 3) - 1;
        trits(i) = trit;
        remaining = (remaining - trit) / 3;
      end
    end

    result.trits = trits;
    result.n_trits = length(trits);
    result.decimal = decimal_val;

  else
    error('Modo desconocido: use encode o decode');
  end

end
