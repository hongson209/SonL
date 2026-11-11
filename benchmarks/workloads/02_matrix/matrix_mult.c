// C Baseline: Numerical Compute & Matrix Arithmetic (Clang -O3)
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {
    int64_t total_sum;
    size_t iterations;
} MatrixStats;

static MatrixStats compute_dense_grid(int32_t dim) {
    int64_t sum = 0;
    for (int32_t i = 0; i < dim; i++) {
        for (int32_t j = 0; j < dim; j++) {
            int64_t val1 = (int64_t)(i * 3 + j * 7);
            int64_t val2 = (int64_t)(i * 5 - j * 2);
            sum += (val1 * val2);
        }
    }
    MatrixStats s = { .total_sum = sum, .iterations = (size_t)(dim * dim) };
    return s;
}

int main(void) {
    MatrixStats stats = {0, 0};
    for (int rep = 0; rep < 50; rep++) {
        stats = compute_dense_grid(300);
    }
    return (stats.iterations == 90000 && stats.total_sum != 0) ? 0 : 1;
}
