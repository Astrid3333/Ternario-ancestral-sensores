% tonalpohualli.m — dia n (1-based) del ciclo 260 -> [numero 1-13, signo 1-20]
% Uso: [num, signo] = tonalpohualli(1)  % -> 1 Cipactli (1,1)
function [num, signo] = tonalpohualli(n)
  if nargin ~= 1 || n < 1 || floor(n) ~= n
    error('tonalpohualli: n debe ser entero >= 1');
  end
  num = mod(n - 1, 13) + 1;
  signo = mod(n - 1, 20) + 1;
end
