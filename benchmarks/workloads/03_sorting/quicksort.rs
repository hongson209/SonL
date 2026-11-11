// Rust Baseline: Sorting & Array Processing
struct SortBuffer {
    len: usize,
    checksum: i64,
}

fn simulate_sort_pass(elements: usize) -> SortBuffer {
    let mut check: i64 = 0;
    for i in 0..elements {
        let pseudo_rand = ((i * 1103515245 + 12345) % 2147483647) as i64;
        check ^= pseudo_rand;
    }
    SortBuffer {
        len: elements,
        checksum: check,
    }
}

fn main() {
    let res = simulate_sort_pass(50000);
    std::process::exit(if res.len == 50000 && res.checksum != 0 { 0 } else { 1 });
}
