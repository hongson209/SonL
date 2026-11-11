// Zig Baseline: Numerical Compute & Matrix Arithmetic
const std = @import("std");

const MatrixStats = struct {
    total_sum: i64,
    iterations: usize,
};

fn compute_dense_grid(dim: i32) MatrixStats {
    var sum: i64 = 0;
    var i: i32 = 0;
    while (i < dim) : (i += 1) {
        var j: i32 = 0;
        while (j < dim) : (j += 1) {
            const val1: i64 = @as(i64, i * 3 + j * 7);
            const val2: i64 = @as(i64, i * 5 - j * 2);
            sum += val1 * val2;
        }
    }
    return MatrixStats{
        .total_sum = sum,
        .iterations = @as(usize, @intCast(dim * dim)),
    };
}

pub fn main() u8 {
    var stats = MatrixStats{ .total_sum = 0, .iterations = 0 };
    var rep: usize = 0;
    while (rep < 50) : (rep += 1) {
        stats = compute_dense_grid(300);
    }
    return if (stats.iterations == 90000 and stats.total_sum != 0) 0 else 1;
}
