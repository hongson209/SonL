// Zig Baseline: Integer Math
const std = @import("std");

fn is_prime(n: i32) bool {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (@mod(n, 2) == 0 or @mod(n, 3) == 0) return false;

    var i: i32 = 5;
    while (i * i <= n) : (i += 6) {
        if (@mod(n, i) == 0 or @mod(n, i + 2) == 0) return false;
    }
    return true;
}

fn count_primes(limit: i32) i32 {
    var count: i32 = 0;
    var n: i32 = 2;
    while (n < limit) : (n += 1) {
        if (is_prime(n)) count += 1;
    }
    return count;
}

pub fn main() u8 {
    const total = count_primes(50000);
    return if (total == 5133) 0 else 1;
}
