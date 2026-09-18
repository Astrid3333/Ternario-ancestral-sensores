% maya_lc_to_jdn.m — Long Count (baktun katun tun winal kin) -> JDN (GMT 584283)
% Uso: jdn = maya_lc_to_jdn(13,0,0,0,0)  % -> 2456283
function jdn = maya_lc_to_jdn(b, k, t, w, n)
  if nargin ~= 5
    error('maya_lc_to_jdn: se requieren 5 digitos (b k t w n)');
  end
  pesos = [144000, 7200, 360, 20, 1];
  digs = [b, k, t, w, n];
  jdn = 584283 + sum(digs .* pesos);
end
