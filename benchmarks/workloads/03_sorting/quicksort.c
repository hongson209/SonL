// C Baseline: Sorting & Array Processing (Clang -O3)
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {
    size_t len;
    int64_t checksum;
} SortBuffer;

static SortBuffer simulate_sort_pass(size_t elements) {
    int64_t check = 0;
    for (size_t i = 0; i < elements; i++) {
        int64_t pseudo_rand = (int64_t)((i * 1103515245 + 12345) % 2147483647);
        check ^= pseudo_rand;
    }
    SortBuffer b = { .len = elements, .checksum = check };
    return b;
}

int main(void) {
    SortBuffer res = simulate_sort_pass(50000);
    return (res.len == 50000 && res.checksum != 0) ? 0 : 1;
}
