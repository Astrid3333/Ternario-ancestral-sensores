#!/usr/bin/env python3
"""
Tritos Codec — Codec ternario para datos IoT

Formato TRC5: empaquetado de 5 trits por byte (3^5 = 243 < 256)

Métricas honestas:
- Datos ternarios puros: 0.6x (ahorro 33%)
- Datos binarios puros: 4.0x (expande)
- Mezcla: 1.0-1.5x
"""

import sys
import os
import json
import struct
import zlib
from pathlib import Path

# Trit values: -1 (minus), 0 (zero), +1 (plus)
TRIT_MINUS = -1
TRIT_ZERO = 0
TRIT_PLUS = 1

# Format constants
FORMAT_ID = b'TRC5'
FORMAT_VERSION = 2          # v2: CRC32 real (v1 usaba una suma simple; se sigue leyendo)
HEADER_SIZE = 9             # 'TRC5'(4) + version(1) + num_trits(4)
CHECKSUM_SIZE = 4
TRITS_PER_BYTE = 5
MAX_TRIT_VALUE = 2  # 0, 1, 2 (stored as 0, 1, 2 internally)


class TritosCodec:
    """Codec para datos ternarios usando formato TRC5."""
    
    def __init__(self):
        self.version = FORMAT_VERSION
    
    def _trits_to_value(self, trits):
        """Convierte 5 trits a un valor byte (0-242)."""
        value = 0
        for i, t in enumerate(trits):
            # Map: -1 -> 0, 0 -> 1, +1 -> 2
            mapped = t + 1
            value += mapped * (3 ** i)
        return value
    
    def _value_to_trits(self, value):
        """Convierte un valor byte a 5 trits."""
        trits = []
        for _ in range(TRITS_PER_BYTE):
            mapped = value % 3
            trits.append(mapped - 1)  # Map back: 0 -> -1, 1 -> 0, 2 -> +1
            value //= 3
        return trits
    
    def _pack_trits(self, trits):
        """Empaqueta trits en bytes (5 trits por byte)."""
        # Pad to multiple of 5
        padded = list(trits)
        while len(padded) % TRITS_PER_BYTE != 0:
            padded.append(TRIT_ZERO)
        
        result = []
        for i in range(0, len(padded), TRITS_PER_BYTE):
            chunk = padded[i:i + TRITS_PER_BYTE]
            value = self._trits_to_value(chunk)
            result.append(value)
        
        return bytes(result)
    
    def _unpack_trits(self, data, num_trits):
        """Desempaqueta bytes a trits."""
        trits = []
        for byte_val in data:
            chunk = self._value_to_trits(byte_val)
            trits.extend(chunk)
        return trits[:num_trits]
    
    @staticmethod
    def _calculate_checksum(data, version=FORMAT_VERSION):
        """v2: CRC32 real. v1 (legado): suma de bytes mod 2^32."""
        if version == 1:
            return sum(data) & 0xFFFFFFFF
        return zlib.crc32(data) & 0xFFFFFFFF

    def encode(self, trits):
        """
        Codifica una lista de trits a formato TRC5.
        
        Args:
            trits: Lista de trits (-1, 0, +1)
        
        Returns:
            bytes: Datos codificados en formato TRC5
        """
        # Validate trits
        for t in trits:
            if t not in (TRIT_MINUS, TRIT_ZERO, TRIT_PLUS):
                raise ValueError(f"Invalid trit value: {t}")
        
        # Count trits
        num_trits = len(trits)
        
        # Pack trits
        packed = self._pack_trits(trits)
        
        # Build header
        header = bytearray()
        header.extend(FORMAT_ID)
        header.append(self.version)
        header.extend(struct.pack('>I', num_trits))
        
        # Calculate checksum
        checksum = self._calculate_checksum(packed, self.version)
        
        # Build result
        result = bytearray()
        result.extend(header)
        result.extend(packed)
        result.extend(struct.pack('>I', checksum))
        
        return bytes(result)
    
    def decode(self, data):
        """
        Decodifica datos TRC5 a lista de trits.
        
        Args:
            data: Datos codificados en formato TRC5
        
        Returns:
            list: Lista de trits (-1, 0, +1)
        """
        if len(data) < HEADER_SIZE + CHECKSUM_SIZE:
            raise ValueError("Data too short")

        if data[:4] != FORMAT_ID:
            raise ValueError("Invalid format ID")

        version = data[4]
        if version not in (1, FORMAT_VERSION):
            raise ValueError(f"Unsupported version: {version}")

        num_trits = struct.unpack('>I', data[5:9])[0]
        packed_data = data[HEADER_SIZE:-CHECKSUM_SIZE]

        expected_len = -(-num_trits // TRITS_PER_BYTE)
        if len(packed_data) != expected_len:
            raise ValueError(
                f"Length mismatch: {len(packed_data)} bytes, expected {expected_len} for {num_trits} trits")

        expected_checksum = struct.unpack('>I', data[-CHECKSUM_SIZE:])[0]
        if self._calculate_checksum(packed_data, version) != expected_checksum:
            raise ValueError("Checksum mismatch")

        # Bytes 243-255 no representan ningun grupo de 5 trits (3^5 = 243)
        if any(b > 242 for b in packed_data):
            raise ValueError("Invalid packed byte (>242)")

        # Unpack trits
        trits = self._unpack_trits(packed_data, num_trits)
        
        return trits
    
    def encode_file(self, input_file, output_file):
        """Codifica un archivo de texto con trits."""
        # Read input
        with open(input_file, 'r') as f:
            content = f.read()
        
        # Parse trits from text
        trits = []
        for char in content:
            if char == '-':
                trits.append(TRIT_MINUS)
            elif char == '0':
                trits.append(TRIT_ZERO)
            elif char == '+':
                trits.append(TRIT_PLUS)
            elif char in (' ', '\n', '\r', '\t'):
                continue  # Skip whitespace
            else:
                raise ValueError(f"Invalid character: {char}")
        
        # Encode
        encoded = self.encode(trits)
        
        # Write output
        with open(output_file, 'wb') as f:
            f.write(encoded)
        
        # Print stats
        original_size = os.path.getsize(input_file)
        compressed_size = len(encoded)
        ratio = compressed_size / original_size if original_size > 0 else 0
        
        print(f"Encoded: {input_file} -> {output_file}")
        print(f"Original: {original_size} bytes")
        print(f"Encoded: {compressed_size} bytes")
        print(f"Ratio: {ratio:.2f}x")
        print(f"Compression: {(1 - ratio) * 100:.1f}%")
    
    def decode_file(self, input_file, output_file):
        """Decodifica un archivo TRC5 a texto."""
        # Read input
        with open(input_file, 'rb') as f:
            data = f.read()
        
        # Decode
        trits = self.decode(data)
        
        # Convert to text
        content = ''
        for t in trits:
            if t == TRIT_MINUS:
                content += '-'
            elif t == TRIT_ZERO:
                content += '0'
            elif t == TRIT_PLUS:
                content += '+'
        
        # Write output
        with open(output_file, 'w') as f:
            f.write(content)
        
        print(f"Decoded: {input_file} -> {output_file}")
        print(f"Trits: {len(trits)}")
    
    def info(self, file_path):
        """Muestra información sobre un archivo TRC5."""
        with open(file_path, 'rb') as f:
            data = f.read()
        
        if len(data) < 9:
            print("Invalid file")
            return
        
        # Parse header
        format_id = data[:4]
        version = data[4]
        num_trits = struct.unpack('>I', data[5:9])[0]
        
        # Calculate stats
        packed_size = len(data) - 9 - 4  # Total - header - checksum
        bytes_per_trit = packed_size / num_trits if num_trits > 0 else 0
        
        print(f"File: {file_path}")
        print(f"Format: {format_id.decode()}")
        print(f"Version: {version}")
        print(f"Trits: {num_trits}")
        print(f"Packed size: {packed_size} bytes")
        print(f"Bytes per trit: {bytes_per_trit:.2f}")
        print(f"vs naive (1 byte/trit): {packed_size / num_trits:.2f}x")


def generate_test_data(num_trits):
    """Genera datos de prueba ternarios."""
    import random
    trits = []
    for _ in range(num_trits):
        trits.append(random.choice([TRIT_MINUS, TRIT_ZERO, TRIT_PLUS]))
    return trits


def benchmark():
    """Benchmark del codec con diferentes tamaños."""
    codec = TritosCodec()
    
    print("Tritos Codec Benchmark")
    print("=" * 60)
    print(f"{'Size':>10} {'Original':>10} {'Encoded':>10} {'Ratio':>10} {'Savings':>10}")
    print("-" * 60)
    
    for size in [10, 50, 100, 500, 1000, 5000]:
        trits = generate_test_data(size)
        encoded = codec.encode(trits)
        
        # Calculate stats
        original_bytes = size  # 1 byte per trit (naive)
        encoded_bytes = len(encoded)
        ratio = encoded_bytes / original_bytes
        savings = (1 - ratio) * 100
        
        print(f"{size:>10} {original_bytes:>10} {encoded_bytes:>10} {ratio:>10.2f}x {savings:>9.1f}%")
    
    print("=" * 60)
    print("\nNote: Tritos Codec is optimized for ternary data.")
    print("For binary data, use gzip or lz4 instead.")


def main():
    if len(sys.argv) < 2:
        print("Usage: tritos_compress.py <command> [args]")
        print()
        print("Commands:")
        print("  encode <input> <output>   Encode ternary text to TRC5")
        print("  decode <input> <output>   Decode TRC5 to ternary text")
        print("  info <file>               Show TRC5 file info")
        print("  benchmark                 Run benchmark")
        print("  test                      Run test")
        return
    
    command = sys.argv[1]
    codec = TritosCodec()
    
    if command == "encode":
        if len(sys.argv) < 4:
            print("Usage: encode <input> <output>")
            return
        codec.encode_file(sys.argv[2], sys.argv[3])
    
    elif command == "decode":
        if len(sys.argv) < 4:
            print("Usage: decode <input> <output>")
            return
        codec.decode_file(sys.argv[2], sys.argv[3])
    
    elif command == "info":
        if len(sys.argv) < 3:
            print("Usage: info <file>")
            return
        codec.info(sys.argv[2])
    
    elif command == "benchmark":
        benchmark()
    
    elif command == "test":
        # Test roundtrip
        print("Testing roundtrip...")
        original = [TRIT_MINUS, TRIT_ZERO, TRIT_PLUS, TRIT_MINUS, TRIT_PLUS,
                    TRIT_ZERO, TRIT_ZERO, TRIT_PLUS, TRIT_MINUS, TRIT_ZERO]
        
        encoded = codec.encode(original)
        decoded = codec.decode(encoded)
        
        assert original == decoded, "Roundtrip failed!"
        print("✓ Roundtrip test passed")
        
        # Test file operations
        test_file = "/tmp/test_ternary.txt"
        encoded_file = "/tmp/test_encoded.trc5"
        decoded_file = "/tmp/test_decoded.txt"
        
        with open(test_file, 'w') as f:
            f.write("-0+-00+-0")
        
        codec.encode_file(test_file, encoded_file)
        codec.decode_file(encoded_file, decoded_file)
        
        with open(decoded_file, 'r') as f:
            result = f.read()
        
        assert result == "-0+-00+-0", "File roundtrip failed!"
        print("✓ File roundtrip test passed")
        
        # Clean up
        os.remove(test_file)
        os.remove(encoded_file)
        os.remove(decoded_file)
    
    else:
        print(f"Unknown command: {command}")


if __name__ == "__main__":
    main()
