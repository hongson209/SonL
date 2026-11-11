// Rust Baseline: Numerical Compute & Matrix Arithmetic
struct MatrixStats {
    total_sum: i64,
    iterations: usize,
}

fn compute_dense_grid(dim: i32) -> MatrixStats {
    let mut sum: i64 = 0;
    for i in 0..dim {
        for j in 0..dim {
            let val1 = (i * 3 + j * 7) as i64;
            let val2 = (i * 5 - j * 2) as i64;
            sum += val1 * val2;
        }
    }
    MatrixStats {
        total_sum: sum,
        iterations: (dim * dim) as usize,
    }
}

fn main() {
    let mut stats = MatrixStats { total_sum: 0, iterations: 0 };
    for _ in 0..50 {
        stats = compute_dense_grid(300);
    }
    std::process::exit(if stats.iterations == 90000 && stats.total_sum != 0 { 0 } else { 1 });
}
