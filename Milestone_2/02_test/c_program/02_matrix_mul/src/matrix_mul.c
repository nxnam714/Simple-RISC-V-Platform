#include "app_runtime.h"

#define MATRIX_SIZE 8u
#define MATRIX_WORDS (MATRIX_SIZE * MATRIX_SIZE)

static uint32_t matrix_index(uint32_t row, uint32_t column);
static void initialize_matrices(volatile int32_t *matrix_a, volatile int32_t *matrix_b);
static void transpose_matrix(volatile int32_t *source, volatile int32_t *destination);
static void multiply_matrices(volatile int32_t *matrix_a, volatile int32_t *matrix_b, volatile int32_t *result);
static uint32_t count_transpose_errors(volatile int32_t *transposed_product, volatile int32_t *reverse_product);
static uint32_t calculate_checksum(volatile int32_t *matrix);
static void print_matrix(volatile int32_t *matrix);

int main(void) {
    volatile int32_t matrix_a[MATRIX_WORDS];
    volatile int32_t matrix_b[MATRIX_WORDS];
    volatile int32_t transpose_a[MATRIX_WORDS];
    volatile int32_t transpose_b[MATRIX_WORDS];
    volatile int32_t product[MATRIX_WORDS];
    volatile int32_t reverse_product[MATRIX_WORDS];
    uint32_t transpose_errors;
    uint32_t checksum;
    uint32_t failures;

    initialize_matrices(matrix_a, matrix_b);

    ee_printf("\n\nHCMUT - Computer Architecture Laboratory - DOELAB\n\n");
    ee_printf("8x8 signed matrix multiplication\n");
    ee_printf("Cross-check: transpose(A) x transpose(B) = transpose(B x A)\n\n");

    ee_printf("Input Matrix A:\n");
    print_matrix(matrix_a);
    ee_printf("\nInput Matrix B:\n");
    print_matrix(matrix_b);

    transpose_matrix(matrix_a, transpose_a);
    transpose_matrix(matrix_b, transpose_b);

    ee_printf("\n=== Transpose Matrix Calculation ===\n");
    ee_printf("Transpose of Matrix A:\n");
    print_matrix(transpose_a);
    ee_printf("\nTranspose of Matrix B:\n");
    print_matrix(transpose_b);

    multiply_matrices(transpose_a, transpose_b, product);
    multiply_matrices(matrix_b, matrix_a, reverse_product);

    transpose_errors = count_transpose_errors(product, reverse_product);
    checksum = calculate_checksum(product);
    failures = transpose_errors;

    if (checksum != 0xaae2cc7eu) {
        ++failures;
    }

    ee_printf("\n=== Multiplication Result transpose(A) x transpose(B) ===\n");
    ee_printf("Multiplication Result:\n");
    print_matrix(product);
    ee_printf("\nChecksum:          0x%08x (expected 0xaae2cc7e)\n", checksum);
    ee_printf("Cross-check errors: %u\n", transpose_errors);
    ee_printf("Final result:      %s\n\n", failures == 0u ? "PASS" : "FAIL");

    app_finish();
}



static uint32_t matrix_index(uint32_t row, uint32_t column) {
    return row * MATRIX_SIZE + column;
}



static void initialize_matrices(volatile int32_t *matrix_a, volatile int32_t *matrix_b) {
    for (uint32_t row = 0u; row < MATRIX_SIZE; ++row) {
        for (uint32_t column = 0u; column < MATRIX_SIZE; ++column) {
            uint32_t index = matrix_index(row, column);
            uint32_t value_a = (row * 13u + column * 7u + 3u) & 31u;
            uint32_t value_b = (row * 5u + column * 11u + 1u) & 31u;

            matrix_a[index] = (int32_t)value_a - 16;
            matrix_b[index] = (int32_t)value_b - 15;
        }
    }
}



static void transpose_matrix(volatile int32_t *source, volatile int32_t *destination) {
    for (uint32_t row = 0u; row < MATRIX_SIZE; ++row) {
        for (uint32_t column = 0u; column < MATRIX_SIZE; ++column) {
            uint32_t source_index = matrix_index(row, column);
            uint32_t destination_index = matrix_index(column, row);

            destination[destination_index] = source[source_index];
        }
    }
}



static void multiply_matrices(volatile int32_t *matrix_a, volatile int32_t *matrix_b,
                              volatile int32_t *result) {
    for (uint32_t row = 0u; row < MATRIX_SIZE; ++row) {
        for (uint32_t column = 0u; column < MATRIX_SIZE; ++column) {
            int32_t sum = 0;

            for (uint32_t inner = 0u; inner < MATRIX_SIZE; ++inner) {
                int32_t left = matrix_a[matrix_index(row, inner)];
                int32_t right = matrix_b[matrix_index(inner, column)];

                sum += multiply_i32(left, right);
            }

            result[matrix_index(row, column)] = sum;
        }
    }
}



static uint32_t count_transpose_errors(volatile int32_t *transposed_product,
                                       volatile int32_t *reverse_product) {
    uint32_t errors = 0u;

    for (uint32_t row = 0u; row < MATRIX_SIZE; ++row) {
        for (uint32_t column = 0u; column < MATRIX_SIZE; ++column) {
            int32_t actual = transposed_product[matrix_index(row, column)];
            int32_t expected = reverse_product[matrix_index(column, row)];

            if (actual != expected) {
                ++errors;
            }
        }
    }

    return errors;
}



static uint32_t calculate_checksum(volatile int32_t *matrix) {
    uint32_t checksum = 0x6d2b79f5u;

    for (uint32_t index = 0u; index < MATRIX_WORDS; ++index) {
        checksum = (checksum << 7) | (checksum >> 25);
        checksum ^= (uint32_t)matrix[index] + index;
    }

    return checksum;
}


static void print_matrix(volatile int32_t *matrix) {
    for (uint32_t row = 0u; row < MATRIX_SIZE; ++row) {
        ee_printf("[");

        for (uint32_t column = 0u; column < MATRIX_SIZE; ++column) {
            ee_printf("%7d", matrix[matrix_index(row, column)]);
        }

        ee_printf(" ]\n");
    }
}

