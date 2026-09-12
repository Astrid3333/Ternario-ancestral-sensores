% run_all.m — verificacion completa del experimento (medido, determinista)
% Uso desde el directorio experiment/: octave --quiet run_all.m
disp('== M1 aritmetica de ciclos ==');
assert(lcm(260, 365) == 18980);            disp('mcm(260,365) = 18980 OK');
assert(52 * 365 == 18980 && 73 * 260 == 18980); disp('52x365 = 73x260 = 18980 OK');
assert(13 * 144000 == 1872000);            disp('13 baktunes = 1872000 dias OK');
assert(25 * 365 + 8 * 366 == 12053);       disp('ciclo persa-33 = 12053 dias OK');

disp('== M2 conversiones ==');
assert(maya_lc_to_jdn(13, 0, 0, 0, 0) == 2456283); disp('13.0.0.0.0 -> 2456283 OK');
assert(maya_lc_to_jdn(0, 0, 0, 0, 0) == 584283);   disp('0.0.0.0.0 -> 584283 OK');
[n1, s1] = tonalpohualli(1);   assert(n1 == 1 && s1 == 1);
[n2, s2] = tonalpohualli(260); assert(n2 == 13 && s2 == 20);
[n3, s3] = tonalpohualli(261); assert(n3 == 1 && s3 == 1);
disp('tonalpohualli 1/260/261 OK');
assert(persa_bisiesto(1403) == 1);  % 1403 fue bisiesto (Esfand 30 dias)
assert(persa_bisiesto(1404) == 0 && persa_bisiesto(1405) == 0);
disp('persa 1403 leap / 1404-1405 comunes OK');

disp('== M4 metricas ==');
T = 365.24219;
fprintf('anio medio persa-33 : %.8f (err %+.2f s/anio)\n', 12053/33, (12053/33 - T) * 86400);
fprintf('anio medio gregoriano: %.6f (err %+.2f s/anio)\n', 365.2425, (365.2425 - T) * 86400);
fprintf('anio medio juliano   : %.6f (err %+.2f s/anio)\n', 365.25, (365.25 - T) * 86400);
fprintf('Haab 365 exacto      : err %+.2f s/anio\n', (365 - T) * 86400);
fprintf('deriva Haab/Rueda 52a: %.3f dias\n', 52 * (T - 365));
fprintf('era 13 baktunes      : %.2f anios tropicos\n', 1872000 / T);
disp('== TODO MEDIDO OK ==');
