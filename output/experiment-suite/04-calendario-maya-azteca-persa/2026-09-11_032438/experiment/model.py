"""model.py — funciones calendáricas puras (réplica exacta de los .m de Octave)."""
from math import gcd
from .data import GMT_JDN_ZERO, PERSIAN_RESIDUES_MOD33

LC_WEIGHTS = (144000, 7200, 360, 20, 1)

def lcm(a, b):
    return a // gcd(a, b) * b

def maya_lc_to_jdn(b, k, t, w, n):
    return GMT_JDN_ZERO + (b * 144000 + k * 7200 + t * 360 + w * 20 + n)

def tonalpohualli(n):
    assert isinstance(n, int) and n >= 1
    return ((n - 1) % 13 + 1, (n - 1) % 20 + 1)

def persa_bisiesto(y):
    """Aproximación ciclo-33 (ver persa_bisiesto.m para la salvedad astronómica)."""
    return (y % 33) in PERSIAN_RESIDUES_MOD33

def mean_year_error_s(mean_year, tropical=365.24219):
    return (mean_year - tropical) * 86400.0
