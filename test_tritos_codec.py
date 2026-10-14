import random, struct, zlib, pytest
from tritos_codec import TritosCodec, FORMAT_ID

c = TritosCodec()

def rnd(n): return [random.choice([-1, 0, 1]) for _ in range(n)]

@pytest.mark.parametrize("n", [0, 1, 4, 5, 6, 99, 1000])
def test_roundtrip(n):
    t = rnd(n); assert c.decode(c.encode(t)) == t

def test_checksum_detects_swap():
    e = bytearray(c.encode(rnd(10))); e[9], e[10] = e[10], e[9]
    if e[9] != e[10]:
        with pytest.raises(ValueError): c.decode(bytes(e))

def test_invalid_byte():
    d = bytearray(c.encode(rnd(5))); d[9] = 250
    d[-4:] = struct.pack('>I', zlib.crc32(bytes([250])))
    with pytest.raises(ValueError): c.decode(bytes(d))

def test_num_trits_mismatch():
    d = bytearray(c.encode(rnd(5))); d[5:9] = struct.pack('>I', 1000)
    with pytest.raises(ValueError): c.decode(bytes(d))

def test_legacy_v1_readable():
    packed = c._pack_trits([1, 0, -1, 1, 0])
    v1 = FORMAT_ID + bytes([1]) + struct.pack('>I', 5) + packed + struct.pack('>I', sum(packed))
    assert c.decode(v1) == [1, 0, -1, 1, 0]
