//! trit_lib.rs — Librería ternaria Rust para kernel C
//!
//! Compilar: rustc --target i686-unknown-none --crate-type staticlib trit_lib.rs -o libtrit.a

#![no_std]
#![allow(non_camel_case_types)]
#![no_main]

// Panic handler for no_std
#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}

// Eh personality (required by Rust but unused without unwinding)
#[export_name = "rust_eh_personality"]
pub extern "C" fn rust_eh_personality() {}

// =============================================================================
// TYPES
// =============================================================================

/// Trit balanced ternary: -1, 0, +1
#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum trit_t {
    NEG = -1,
    ZERO = 0,
    POS = 1,
}

/// Packed trits (5 per byte)
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct trits5_t {
    pub packed: u8,
}

/// Variable-length ternary number
#[repr(C)]
#[derive(Clone, Debug)]
pub struct ternary_num_t {
    pub trits: [i8; 32],
    pub len: i32,
}

// =============================================================================
// TRIT OPERATIONS
// =============================================================================

#[no_mangle]
pub extern "C" fn trit_add_rust(a: i8, b: i8) -> i8 {
    let sum = a + b;
    match sum {
        -2 => 1,    // -2 mod 3 = +1
        -1 => -1,
        0 => 0,
        1 => 1,
        2 => -1,    // +2 mod 3 = -1
        _ => 0,
    }
}

#[no_mangle]
pub extern "C" fn trit_mul_rust(a: i8, b: i8) -> i8 {
    if a == 0 || b == 0 { return 0; }
    if a == b { 1 } else { -1 }
}

#[no_mangle]
pub extern "C" fn trit_neg_rust(a: i8) -> i8 {
    -a
}

#[no_mangle]
pub extern "C" fn trit_to_char_rust(t: i8) -> u8 {
    match t {
        -1 => b'-',
        0 => b'0',
        1 => b'+',
        _ => b'?',
    }
}

// =============================================================================
// PACKED TRITS (5 per byte)
// =============================================================================

#[no_mangle]
pub extern "C" fn trits5_get_rust(packed: u8, i: i32) -> i8 {
    let v = (packed / 3u8.pow(i as u32)) % 3;
    match v {
        0 => -1,
        1 => 0,
        _ => 1,
    }
}

#[no_mangle]
pub extern "C" fn trits5_set_rust(packed: &mut u8, i: i32, value: i8) {
    let current = trits5_get_rust(*packed, i);
    let power = 3u8.pow(i as u32);
    let current_val = (*packed / power) % 3;
    let new_val = ((value + 1) as u8 + 3) % 3;  // Convert -1,0,1 to 0,1,2
    *packed = (*packed / power * power) + ((current_val + new_val) % 3) * power;
}

#[no_mangle]
pub extern "C" fn trits5_to_i16_rust(packed: u8) -> i16 {
    let mut result: i16 = 0;
    for i in 0..5 {
        result += (trits5_get_rust(packed, i) as i16) * (3i16.pow(i as u32));
    }
    result
}

#[no_mangle]
pub extern "C" fn trits5_from_i16_rust(mut v: i16) -> u8 {
    let mut packed = 0u8;
    for i in 0..5 {
        let rem = ((v % 3) + 3) % 3;
        v = (v - rem) / 3;
        packed += (rem as u8) * 3u8.pow(i as u32);
    }
    packed
}

// =============================================================================
// VARIABLE-LENGTH TERNARY NUMBERS
// =============================================================================

#[no_mangle]
pub extern "C" fn ternary_from_i32_rust(v: i32) -> ternary_num_t {
    let mut trits = [0i8; 32];
    let mut num = v;
    let mut len = 0i32;

    if num == 0 {
        return ternary_num_t { trits, len: 1 };
    }

    while num != 0 && len < 32 {
        let rem = ((num % 3) + 3) % 3;
        num = (num - rem) / 3;
        trits[len as usize] = (rem - 1) as i8;
        len += 1;
    }

    ternary_num_t { trits, len }
}

#[no_mangle]
pub extern "C" fn ternary_to_i32_rust(num: &ternary_num_t) -> i32 {
    let mut result = 0i32;
    for i in 0..num.len {
        result += (num.trits[i as usize] as i32) * 3i32.pow(i as u32);
    }
    result
}

#[no_mangle]
pub extern "C" fn ternary_add_rust(a: &ternary_num_t, b: &ternary_num_t, result: &mut ternary_num_t) {
    let max_len = core::cmp::max(a.len, b.len);
    let mut carry = 0i8;

    for i in 0..max_len.min(31) {
        let va = if i < a.len { a.trits[i as usize] } else { 0 };
        let vb = if i < b.len { b.trits[i as usize] } else { 0 };
        let sum = va + vb + carry;

        match sum {
            -3 => { result.trits[i as usize] = 0; carry = -1; }
            -2 => { result.trits[i as usize] = 1; carry = -1; }
            3 => { result.trits[i as usize] = 0; carry = 1; }
            2 => { result.trits[i as usize] = -1; carry = 1; }
            _ => { result.trits[i as usize] = sum; carry = 0; }
        }
    }

    if carry != 0 && max_len < 31 {
        result.trits[max_len as usize] = carry;
        result.len = core::cmp::min(max_len + 1, 32);
    } else {
        result.len = max_len;
    }
}

#[no_mangle]
pub extern "C" fn ternary_mul_rust(a: &ternary_num_t, b: &ternary_num_t, result: &mut ternary_num_t) {
    let max_len = core::cmp::min(a.len + b.len, 32);

    // Clear result
    for i in 0..32 {
        result.trits[i] = 0;
    }

    for i in 0..a.len.min(31) {
        if a.trits[i as usize] == 0 { continue; }

        let mut carry = 0i8;
        for j in 0..b.len.min(31 - i) {
            let prod = a.trits[i as usize] * b.trits[j as usize] + result.trits[(i + j) as usize] + carry;

            match prod {
                -3 => { result.trits[(i + j) as usize] = 0; carry = -1; }
                -2 => { result.trits[(i + j) as usize] = 1; carry = -1; }
                3 => { result.trits[(i + j) as usize] = 0; carry = 1; }
                2 => { result.trits[(i + j) as usize] = -1; carry = 1; }
                _ => { result.trits[(i + j) as usize] = prod; carry = 0; }
            }
        }
        if carry != 0 && (i + b.len) < 31 {
            result.trits[(i + b.len) as usize] = carry;
        }
    }

    result.len = max_len;
}

#[no_mangle]
pub extern "C" fn ternary_to_str_rust(num: &ternary_num_t, buf: &mut [u8; 64]) {
    let mut i = num.len - 1;
    
    // Skip leading zeros
    while i > 0 && num.trits[i as usize] == 0 {
        i -= 1;
    }
    
    let mut j = 0;
    while i >= 0 {
        buf[j] = match num.trits[i as usize] {
            -1 => b'-',
            0 => b'0',
            1 => b'+',
            _ => b'?',
        };
        j += 1;
        i -= 1;
    }
    buf[j] = 0;
}

// =============================================================================
// BENCHMARK
// =============================================================================

#[no_mangle]
pub extern "C" fn rdtsc_rust() -> u64 {
    unsafe {
        let lo: u32;
        let hi: u32;
        core::arch::asm!("rdtsc", out("eax") lo, out("edx") hi);
        ((hi as u64) << 32) | (lo as u64)
    }
}

#[no_mangle]
pub extern "C" fn bench_ternary_add_rust(iterations: u32) -> u64 {
    let start = rdtsc_rust();
    let mut a = ternary_from_i32_rust(12345);
    let mut b = ternary_from_i32_rust(67890);
    let mut result = ternary_num_t { trits: [0; 32], len: 0 };
    
    for _ in 0..iterations {
        ternary_add_rust(&a, &b, &mut result);
        a = result.clone();
        ternary_add_rust(&b, &a, &mut result);
        b = result.clone();
    }
    
    rdtsc_rust() - start
}

#[no_mangle]
pub extern "C" fn bench_ternary_mul_rust(iterations: u32) -> u64 {
    let start = rdtsc_rust();
    let mut a = ternary_from_i32_rust(123);
    let mut b = ternary_from_i32_rust(456);
    let mut result = ternary_num_t { trits: [0; 32], len: 0 };
    
    for _ in 0..iterations {
        ternary_mul_rust(&a, &b, &mut result);
        a = result.clone();
        b = ternary_from_i32_rust((ternary_to_i32_rust(&b) % 1000) + 1);
    }
    
    rdtsc_rust() - start
}
