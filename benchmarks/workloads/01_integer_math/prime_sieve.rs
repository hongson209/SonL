// Rust Baseline: Integer Math
fn is_prime(n: i32) -> bool {
    if n <= 1 { return false; }
    if n <= 3 { return true; }
    if n % 2 == 0 || n % 3 == 0 { return false; }

    let mut i = 5;
    while i * i <= n {
        if n % i == 0 || n % (i + 2) == 0 {
            return false;
        }
        i += 6;
    }
    true
}

fn count_primes(limit: i32) -> i32 {
    let mut count = 0;
    for n in 2..limit {
        if is_prime(n) {
            count += 1;
        }
    }
    count
}

fn main() {
    let total = count_primes(50000);
    std::process::exit(if total == 5133 { 0 } else { 1 });
}
