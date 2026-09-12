% persa_bisiesto.m — aproximacion ciclo-33 (Borkowski/jalcal): restos {1,5,9,13,17,22,26,30} mod 33
% NOTA: la regla real es astronomica (equinoccio Teheran); el ciclo-33 es aproximacion
% documentada con rupturas de 29 anios. Uso: persa_bisiesto(1404) % -> 0 (comun)
function leap = persa_bisiesto(y)
  if nargin ~= 1 || floor(y) ~= y
    error('persa_bisiesto: y debe ser anio jalali entero');
  end
  r = mod(y, 33);
  leap = any(r == [1, 5, 9, 13, 17, 22, 26, 30]);
end
