//! trit.rs — Tipos ternarios para Tritos
//!
//! Balanced ternary: -1 (neg), 0 (zero), +1 (pos)

#![no_std]

/// Trit balanced ternary: -1, 0, +1
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord)]
#[repr(i8)]
pub enum Trit {
    Neg = -1,
    Zero = 0,
    Pos = 1,
}

impl Trit {
    /// Create from i8
    pub fn from_i8(v: i8) -> Self {
        match v {
            -1 => Trit::Neg,
            0 => Trit::Zero,
            1 => Trit::Pos,
            _ => Trit::Zero,
        }
    }

    /// Convert to i8
    pub fn to_i8(self) -> i8 {
        self as i8
    }

    /// Add two trits (balanced ternary)
    pub fn add(self, other: Trit) -> Trit {
        let sum = (self as i8) + (other as i8);
        match sum {
            -2 => Trit::Pos,   // -2 mod 3 = +1
            -1 => Trit::Neg,
            0 => Trit::Zero,
            1 => Trit::Pos,
            2 => Trit::Neg,   // +2 mod 3 = -1
            _ => Trit::Zero,
        }
    }

    /// Multiply two trits
    pub fn mul(self, other: Trit) -> Trit {
        if self == Trit::Zero || other == Trit::Zero {
            Trit::Zero
        } else if self == other {
            Trit::Pos
        } else {
            Trit::Neg
        }
    }

    /// Negate a trit
    pub fn neg(self) -> Trit {
        match self {
            Trit::Neg => Trit::Pos,
            Trit::Pos => Trit::Neg,
            Trit::Zero => Trit::Zero,
        }
    }

    /// Compare two trits
    pub fn cmp(self, other: Trit) -> i8 {
        (self as i8) - (other as i8)
    }

    /// Convert to char
    pub fn to_char(self) -> char {
        match self {
            Trit::Neg => '-',
            Trit::Zero => '0',
            Trit::Pos => '+',
        }
    }
}

impl core::fmt::Display for Trit {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        write!(f, "{}", self.to_char())
    }
}

/// Packed trits (5 trits per byte)
#[derive(Clone, Copy, Debug)]
pub struct Trits5(pub u8);

impl Trits5 {
    /// Get trit at position i (0-4)
    pub fn get(self, i: usize) -> Trit {
        let v = (self.0 / 3u8.pow(i as u32)) % 3;
        match v {
            0 => Trit::Neg,
            1 => Trit::Zero,
            _ => Trit::Pos,
        }
    }

    /// Set trit at position i
    pub fn set(&mut self, i: usize, t: Trit) {
        let current = self.get(i);
        let diff = (t as i8) - (current as i8);
        let power = 3u8.pow(i as u32);
        self.0 = (self.0 / power) * power + ((self.0 / power + diff as u8) % 3) * power;
    }

    /// Convert to i16 (sign-extend)
    pub fn to_i16(self) -> i16 {
        let mut result: i16 = 0;
        for i in 0..5 {
            result += (self.get(i) as i16) * (3i16.pow(i as u32));
        }
        result
    }

    /// Create from i16
    pub fn from_i16(mut v: i16) -> Self {
        let mut packed = 0u8;
        for i in 0..5 {
            let rem = ((v % 3) + 3) % 3;
            v = (v - rem) / 3;
            packed += (rem as u8) * 3u8.pow(i as u32);
        }
        Trits5(packed)
    }
}

/// Variable-length balanced ternary number
#[derive(Clone, Debug)]
pub struct TernaryNum {
    pub trits: [i8; 32],  // -1, 0, +1
    pub len: usize,
}

impl TernaryNum {
    /// Create from i32
    pub fn from_i32(mut v: i32) -> Self {
        let mut trits = [0i8; 32];
        let mut len = 0;

        if v == 0 {
            return TernaryNum { trits, len: 1 };
        }

        while v != 0 && len < 32 {
            let rem = ((v % 3) + 3) % 3;
            v = (v - rem) / 3;
            trits[len] = (rem - 1) as i8;  // Convert to balanced: 0,1,2 → -1,0,+1
            len += 1;
        }

        TernaryNum { trits, len }
    }

    /// Convert to i32
    pub fn to_i32(&self) -> i32 {
        let mut result = 0i32;
        for i in 0..self.len {
            result += (self.trits[i] as i32) * 3i32.pow(i as u32);
        }
        result
    }

    /// Add two ternary numbers
    pub fn add(&self, other: &TernaryNum) -> TernaryNum {
        let max_len = core::cmp::max(self.len, other.len);
        let mut result = [0i8; 32];
        let mut carry = 0i8;

        for i in 0..max_len.min(31) {
            let a = if i < self.len { self.trits[i] } else { 0 };
            let b = if i < other.len { other.trits[i] } else { 0 };
            let sum = a + b + carry;

            match sum {
                -3 => { result[i] = 0; carry = -1; }
                -2 => { result[i] = 1; carry = -1; }
                3 => { result[i] = 0; carry = 1; }
                2 => { result[i] = -1; carry = 1; }
                _ => { result[i] = sum; carry = 0; }
            }
        }

        if carry != 0 && max_len < 31 {
            result[max_len] = carry;
        }

        TernaryNum { trits: result, len: core::cmp::min(max_len + 1, 32) }
    }

    /// Multiply two ternary numbers
    pub fn mul(&self, other: &TernaryNum) -> TernaryNum {
        let mut result = [0i8; 32];
        let max_len = core::cmp::min(self.len + other.len, 32);

        for i in 0..self.len.min(31) {
            if self.trits[i] == 0 { continue; }

            let mut carry = 0i8;
            for j in 0..other.len.min(31 - i) {
                let prod = self.trits[i] * other.trits[j] + result[i + j] + carry;

                match prod {
                    -3 => { result[i + j] = 0; carry = -1; }
                    -2 => { result[i + j] = 1; carry = -1; }
                    3 => { result[i + j] = 0; carry = 1; }
                    2 => { result[i + j] = -1; carry = 1; }
                    _ => { result[i + j] = prod; carry = 0; }
                }
            }
            if carry != 0 && i + other.len < 31 {
                result[i + other.len] = carry;
            }
        }

        TernaryNum { trits: result, len: max_len }
    }

    /// Display as string
    pub fn to_string(&self) -> [char; 64] {
        let mut buf = ['\0'; 64];
        let start = if self.len > 1 {
            // Skip leading zeros
            let mut s = self.len - 1;
            while s > 0 && self.trits[s] == 0 { s -= 1; }
            s
        } else {
            0
        };

        let mut j = 0;
        for i in (0..=start).rev() {
            buf[j] = match self.trits[i] {
                -1 => '-',
                0 => '0',
                1 => '+',
                _ => '?',
            };
            j += 1;
        }
        buf
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_trit_add() {
        assert_eq!(Trit::add(Trit::Pos, Trit::Pos), Trit::Neg);   // 1+1=-1 (mod 3)
        assert_eq!(Trit::add(Trit::Neg, Trit::Neg), Trit::Pos);   // -1+-1=+1 (mod 3)
        assert_eq!(Trit::add(Trit::Pos, Trit::Neg), Trit::Zero);  // 1+(-1)=0
        assert_eq!(Trit::add(Trit::Zero, Trit::Pos), Trit::Pos);  // 0+1=1
    }

    #[test]
    fn test_ternary_add() {
        let a = TernaryNum::from_i32(5);
        let b = TernaryNum::from_i32(3);
        let c = a.add(&b);
        assert_eq!(c.to_i32(), 8);
    }

    #[test]
    fn test_ternary_mul() {
        let a = TernaryNum::from_i32(5);
        let b = TernaryNum::from_i32(3);
        let c = a.mul(&b);
        assert_eq!(c.to_i32(), 15);
    }
}
