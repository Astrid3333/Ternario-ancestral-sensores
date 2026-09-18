//! main.rs — Benchmark ternario Rust vs C
//!
//! Compilar: rustc -O src/main.rs -o benchmark_rs

mod trit;

use trit::{Trit, Trits5, TernaryNum};
use std::time::Instant;

fn rdtsc() -> u64 {
    #[cfg(target_arch = "x86_64")]
    unsafe {
        let lo: u32;
        let hi: u32;
        core::arch::asm!("rdtsc", out("eax") lo, out("edx") hi);
        ((hi as u64) << 32) | (lo as u64)
    }
    #[cfg(not(target_arch = "x86_64"))]
    0
}

fn bench_trit_add(iterations: u32) -> u64 {
    let start = rdtsc();
    let mut result = Trit::Zero;
    for _ in 0..iterations {
        result = result.add(Trit::Pos);
        result = result.add(Trit::Neg);
        result = result.add(Trit::Zero);
    }
    rdtsc() - start
}

fn bench_trits5(iterations: u32) -> u64 {
    let start = rdtsc();
    let mut a = Trits5(0);
    let mut b = Trits5(255);
    for _ in 0..iterations {
        let mut result = Trits5(0);
        for i in 0..5 {
            let ta = a.get(i);
            let tb = b.get(i);
            result.set(i, ta.add(tb));
        }
        a = result;
        b = Trits5(b.0.wrapping_add(1));
    }
    rdtsc() - start
}

fn bench_ternary_add(iterations: u32) -> u64 {
    let start = rdtsc();
    let mut a = TernaryNum::from_i32(12345);
    let mut b = TernaryNum::from_i32(67890);
    for _ in 0..iterations {
        a = a.add(&b);
        b = b.add(&a);
    }
    rdtsc() - start
}

fn bench_ternary_mul(iterations: u32) -> u64 {
    let start = rdtsc();
    let mut a = TernaryNum::from_i32(123);
    let mut b = TernaryNum::from_i32(456);
    for _ in 0..iterations {
        a = a.mul(&b);
        b = TernaryNum::from_i32((b.to_i32() % 1000) + 1);
    }
    rdtsc() - start
}

fn bench_to_ternary(iterations: u32) -> u64 {
    let start = rdtsc();
    for i in 0..iterations {
        let _ = TernaryNum::from_i32(i as i32);
    }
    rdtsc() - start
}

fn bench_from_ternary(iterations: u32) -> u64 {
    let start = rdtsc();
    let mut num = TernaryNum::from_i32(12345);
    for _ in 0..iterations {
        let _ = num.to_i32();
        num = num.add(&TernaryNum::from_i32(1));
    }
    rdtsc() - start
}

fn main() {
    println!("\n  [Ternary Rust Benchmark]\n");
    
    let iterations = 100000;
    
    let trit_add = bench_trit_add(iterations);
    let trits5 = bench_trits5(iterations);
    let ternary_add = bench_ternary_add(iterations);
    let ternary_mul = bench_ternary_mul(iterations);
    let to_ternary = bench_to_ternary(iterations);
    let from_ternary = bench_from_ternary(iterations);
    
    println!("  Trit add (100k): {} cycles", trit_add);
    println!("  Trits5 packed (100k): {} cycles", trits5);
    println!("  Ternary add (100k): {} cycles", ternary_add);
    println!("  Ternary mul (100k): {} cycles", ternary_mul);
    println!("  Int->ternary (100k): {} cycles", to_ternary);
    println!("  Ternary->int (100k): {} cycles", from_ternary);
    
    let total = trit_add + trits5 + ternary_add + ternary_mul + to_ternary + from_ternary;
    println!("  Total: {} cycles", total);
    println!("\n  Done!");
}
