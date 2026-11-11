// C Baseline: Integer Math (Clang -O3)
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

static bool is_prime(int32_t n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;

    int32_t i = 5;
    while (i * i <= n) {
        if (n % i == 0 || n % (i + 2) == 0) return false;
        i += 6;
    }
    return true;
}

static int32_t count_primes(int32_t limit) {
    int32_t count = 0;
    for (int32_t n = 2; n < limit; n++) {
        if (is_prime(n)) count++;
    }
    return count;
}

int main(void) {
    int32_t total = count_primes(50000);
    return (total == 5133) ? 0 : 1;
}
