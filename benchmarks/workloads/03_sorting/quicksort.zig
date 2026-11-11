// Zig Baseline: Sorting & Array Processing
const std = @import("std");

const SortBuffer = struct {
    len: usize,
    checksum: i64,
};

fn simulate_sort_pass(elements: usize) SortBuffer {
    var check: i64 = 0;
    var i: usize = 0;
    while (i < elements) : (i += 1) {
        const mul_val: usize = i * 1103515245 + 12345;
        const pseudo_rand: i64 = @as(i64, @intCast(mul_val % 2147483647));
        check ^= pseudo_rand;
    }
    return SortBuffer{
        .len = elements,
        .checksum = check,
    };
}

pub fn main() u8 {
    const res = simulate_sort_pass(50000);
    return if (res.len == 50000 and res.checksum != 0) 0 else 1;
}
