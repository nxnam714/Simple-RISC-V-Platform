#include "app_runtime.h"

#define INPUT_SIZE 7u
#define KERNEL_SIZE 3u
#define OUTPUT_SIZE 5u
#define INPUT_WORDS (INPUT_SIZE * INPUT_SIZE)
#define KERNEL_WORDS (KERNEL_SIZE * KERNEL_SIZE)
#define OUTPUT_WORDS (OUTPUT_SIZE * OUTPUT_SIZE)
#define TEMPORARY_WORDS (INPUT_SIZE * OUTPUT_SIZE)

static const int32_t input_values[INPUT_WORDS] = {
     12,  -7, 103,  19,   0, -44,  58,
     -4,  88,  15,  -2,  61,   7, -35,
     90,   1, -14,  73,  20,  -6,  41,
    -16, 105,   4,  11,  -3,  67, -28,
      5,  -9, 117,   2, -12,  39,   8,
    -55,  26,  -1,  84,  13, -72,  64,
     31, -18,  47,   6,  99,  -5,  22
};

static const int32_t gaussian_coefficients[KERNEL_WORDS] = {
    1, 2, 1,
    2, 4, 2,
    1, 2, 1
};

static const int32_t sobel_x_coefficients[KERNEL_WORDS] = {
    -1, 0, 1,
    -2, 0, 2,
    -1, 0, 1
};

static const int32_t sobel_y_coefficients[KERNEL_WORDS] = {
    -1, -2, -1,
     0,  0,  0,
     1,  2,  1
};

static const int32_t expected_gaussian_numerator[OUTPUT_WORDS] = {
    553, 496, 437, 336,  99,
    541, 332, 399, 413, 241,
    578, 521, 307, 268, 329,
    402, 686, 421, 144, 184,
    138, 525, 627, 292,  51
};

static const int32_t expected_sobel_x[OUTPUT_WORDS] = {
      25, -82,   23, -124, -113,
    -169, -40,  107,  -93,  -79,
      48,-105, -109,   70,   -9,
     298, -14, -251,  -26,   66,
     236, 151,  -49, -286,   45
};

static const int32_t expected_sobel_y[OUTPUT_WORDS] = {
     -23, -172,  11, 132,   79,
      11,    8, -49, -55,   63,
      26,  181, -43, -90,   25,
    -202,  -16, 157, -34, -170,
     -62, -145,  49, 182,   37
};

static void copy_integer_values(volatile int32_t *destination, const int32_t *source,
                                uint32_t count);
static void initialize_kernels(volatile int32_t *gaussian_display,
                               volatile int32_t *sobel_x_display,
                               volatile int32_t *sobel_y_display,
                               volatile double *gaussian_kernel,
                               volatile double *sobel_x_kernel,
                               volatile double *sobel_y_kernel,
                               double sixteenth);
static void convolve(volatile int32_t *input, volatile double *kernel,
                     volatile double *output);
static void gaussian_separable(volatile int32_t *input, volatile double *temporary,
                               volatile double *output, double quarter);
static void check_results(volatile double *gaussian_output,
                          volatile double *sobel_x_output,
                          volatile double *sobel_y_output,
                          volatile double *edge_magnitude,
                          volatile double *separable_output,
                          double sixteenth,
                          double tolerance,
                          volatile uint32_t *passed,
                          volatile uint32_t *total);
static void print_inputs(volatile int32_t *input,
                         volatile int32_t *gaussian_display,
                         volatile int32_t *sobel_x_display,
                         volatile int32_t *sobel_y_display);
static void print_results(volatile double *gaussian_output,
                          volatile double *sobel_x_output,
                          volatile double *sobel_y_output,
                          volatile double *edge_magnitude,
                          uint32_t passed,
                          uint32_t total);
static double absolute_double(double value);
static void print_int_matrix(volatile int32_t *matrix, uint32_t rows, uint32_t columns);
static void print_double_matrix(volatile double *matrix, uint32_t rows, uint32_t columns);

int main(void) {
    volatile int32_t input[INPUT_WORDS];
    volatile int32_t gaussian_display[KERNEL_WORDS];
    volatile int32_t sobel_x_display[KERNEL_WORDS];
    volatile int32_t sobel_y_display[KERNEL_WORDS];
    volatile double gaussian_kernel[KERNEL_WORDS];
    volatile double sobel_x_kernel[KERNEL_WORDS];
    volatile double sobel_y_kernel[KERNEL_WORDS];
    volatile double gaussian_output[OUTPUT_WORDS];
    volatile double sobel_x_output[OUTPUT_WORDS];
    volatile double sobel_y_output[OUTPUT_WORDS];
    volatile double edge_magnitude[OUTPUT_WORDS];
    volatile double separable_temporary[TEMPORARY_WORDS];
    volatile double separable_output[OUTPUT_WORDS];
    volatile uint32_t passed_checks = 0u;
    volatile uint32_t total_checks = 0u;
    volatile uint32_t sixteenth_high_word = 0x3fb00000u;
    volatile uint32_t sixteenth_low_word = 0x00000000u;
    volatile uint32_t quarter_high_word = 0x3fd00000u;
    volatile uint32_t quarter_low_word = 0x00000000u;
    double sixteenth = double_from_words(sixteenth_high_word, sixteenth_low_word);
    double quarter = double_from_words(quarter_high_word, quarter_low_word);
    double tolerance = 0.000000001;

    copy_integer_values(input, input_values, INPUT_WORDS);
    initialize_kernels(gaussian_display,
                       sobel_x_display,
                       sobel_y_display,
                       gaussian_kernel,
                       sobel_x_kernel,
                       sobel_y_kernel,
                       sixteenth);

    convolve(input, gaussian_kernel, gaussian_output);
    convolve(input, sobel_x_kernel, sobel_x_output);
    convolve(input, sobel_y_kernel, sobel_y_output);
    gaussian_separable(input, separable_temporary, separable_output, quarter);

    check_results(gaussian_output,
                  sobel_x_output,
                  sobel_y_output,
                  edge_magnitude,
                  separable_output,
                  sixteenth,
                  tolerance,
                  &passed_checks,
                  &total_checks);

    ee_printf("\n\nHCMUT - Computer Architecture Laboratory - DOELAB\n\n");
    ee_printf("Two-dimensional double-precision convolution\n\n");

    print_inputs(input, gaussian_display, sobel_x_display, sobel_y_display);
    print_results(gaussian_output,
                  sobel_x_output,
                  sobel_y_output,
                  edge_magnitude,
                  passed_checks,
                  total_checks);

    app_finish();
}

static void copy_integer_values(volatile int32_t *destination, const int32_t *source,
                                uint32_t count) {
    for (uint32_t index = 0u; index < count; ++index) {
        destination[index] = source[index];
    }
}

static void initialize_kernels(volatile int32_t *gaussian_display,
                               volatile int32_t *sobel_x_display,
                               volatile int32_t *sobel_y_display,
                               volatile double *gaussian_kernel,
                               volatile double *sobel_x_kernel,
                               volatile double *sobel_y_kernel,
                               double sixteenth) {
    for (uint32_t index = 0u; index < KERNEL_WORDS; ++index) {
        gaussian_display[index] = gaussian_coefficients[index];
        sobel_x_display[index] = sobel_x_coefficients[index];
        sobel_y_display[index] = sobel_y_coefficients[index];

        gaussian_kernel[index] = (double)gaussian_coefficients[index] * sixteenth;
        sobel_x_kernel[index] = (double)sobel_x_coefficients[index];
        sobel_y_kernel[index] = (double)sobel_y_coefficients[index];
    }
}

static void convolve(volatile int32_t *input, volatile double *kernel,
                     volatile double *output) {
    for (uint32_t row = 0u; row < OUTPUT_SIZE; ++row) {
        for (uint32_t column = 0u; column < OUTPUT_SIZE; ++column) {
            double sum = 0.0;

            for (uint32_t kernel_row = 0u; kernel_row < KERNEL_SIZE; ++kernel_row) {
                for (uint32_t kernel_column = 0u; kernel_column < KERNEL_SIZE; ++kernel_column) {
                    uint32_t input_row = row + kernel_row;
                    uint32_t input_column = column + kernel_column;
                    uint32_t input_index = input_row * INPUT_SIZE + input_column;
                    uint32_t kernel_index = kernel_row * KERNEL_SIZE + kernel_column;

                    sum += (double)input[input_index] * kernel[kernel_index];
                }
            }

            output[row * OUTPUT_SIZE + column] = sum;
        }
    }
}

static void gaussian_separable(volatile int32_t *input, volatile double *temporary,
                               volatile double *output, double quarter) {
    for (uint32_t row = 0u; row < INPUT_SIZE; ++row) {
        for (uint32_t column = 0u; column < OUTPUT_SIZE; ++column) {
            uint32_t input_index = row * INPUT_SIZE + column;
            double left = (double)input[input_index];
            double center = (double)input[input_index + 1u] * 2.0;
            double right = (double)input[input_index + 2u];
            double horizontal_sum = left + center + right;

            temporary[row * OUTPUT_SIZE + column] = horizontal_sum * quarter;
        }
    }

    for (uint32_t row = 0u; row < OUTPUT_SIZE; ++row) {
        for (uint32_t column = 0u; column < OUTPUT_SIZE; ++column) {
            uint32_t temporary_index = row * OUTPUT_SIZE + column;
            double top = temporary[temporary_index];
            double center = temporary[temporary_index + OUTPUT_SIZE] * 2.0;
            double bottom = temporary[temporary_index + OUTPUT_SIZE * 2u];
            double vertical_sum = top + center + bottom;

            output[row * OUTPUT_SIZE + column] = vertical_sum * quarter;
        }
    }
}

static void check_results(volatile double *gaussian_output,
                          volatile double *sobel_x_output,
                          volatile double *sobel_y_output,
                          volatile double *edge_magnitude,
                          volatile double *separable_output,
                          double sixteenth,
                          double tolerance,
                          volatile uint32_t *passed,
                          volatile uint32_t *total) {
    for (uint32_t index = 0u; index < OUTPUT_WORDS; ++index) {
        double expected_gaussian = (double)expected_gaussian_numerator[index] * sixteenth;
        double expected_x = (double)expected_sobel_x[index];
        double expected_y = (double)expected_sobel_y[index];
        double expected_magnitude = absolute_double(expected_x) + absolute_double(expected_y);

        edge_magnitude[index] = absolute_double(sobel_x_output[index]);
        edge_magnitude[index] += absolute_double(sobel_y_output[index]);

        ++*total;
        if (absolute_double(gaussian_output[index] - expected_gaussian) <= tolerance) {
            ++*passed;
        }

        ++*total;
        if (absolute_double(sobel_x_output[index] - expected_x) <= tolerance) {
            ++*passed;
        }

        ++*total;
        if (absolute_double(sobel_y_output[index] - expected_y) <= tolerance) {
            ++*passed;
        }

        ++*total;
        if (absolute_double(edge_magnitude[index] - expected_magnitude) <= tolerance) {
            ++*passed;
        }

        ++*total;
        if (absolute_double(separable_output[index] - gaussian_output[index]) <= tolerance) {
            ++*passed;
        }
    }
}

static void print_inputs(volatile int32_t *input,
                         volatile int32_t *gaussian_display,
                         volatile int32_t *sobel_x_display,
                         volatile int32_t *sobel_y_display) {
    ee_printf("=== Input ===\n");
    ee_printf("Input matrix (7x7):\n");
    print_int_matrix(input, INPUT_SIZE, INPUT_SIZE);

    ee_printf("\nGaussian kernel coefficients (scale 1/16):\n");
    print_int_matrix(gaussian_display, KERNEL_SIZE, KERNEL_SIZE);

    ee_printf("\nSobel-X kernel:\n");
    print_int_matrix(sobel_x_display, KERNEL_SIZE, KERNEL_SIZE);

    ee_printf("\nSobel-Y kernel:\n");
    print_int_matrix(sobel_y_display, KERNEL_SIZE, KERNEL_SIZE);
}

static void print_results(volatile double *gaussian_output,
                          volatile double *sobel_x_output,
                          volatile double *sobel_y_output,
                          volatile double *edge_magnitude,
                          uint32_t passed,
                          uint32_t total) {
    ee_printf("\n=== Result ===\n");
    ee_printf("Gaussian blur (5x5):\n");
    print_double_matrix(gaussian_output, OUTPUT_SIZE, OUTPUT_SIZE);

    ee_printf("\nSobel-X response (5x5):\n");
    print_double_matrix(sobel_x_output, OUTPUT_SIZE, OUTPUT_SIZE);

    ee_printf("\nSobel-Y response (5x5):\n");
    print_double_matrix(sobel_y_output, OUTPUT_SIZE, OUTPUT_SIZE);

    ee_printf("\nEdge magnitude |X| + |Y| (5x5):\n");
    print_double_matrix(edge_magnitude, OUTPUT_SIZE, OUTPUT_SIZE);

    ee_printf("\nSelf-check: %u/%u  %s\n\n",
              passed,
              total,
              passed == total ? "PASS" : "FAIL");
}

static double absolute_double(double value) {
    if (value < 0.0) {
        return -value;
    }

    return value;
}

static void print_int_matrix(volatile int32_t *matrix, uint32_t rows, uint32_t columns) {
    for (uint32_t row = 0u; row < rows; ++row) {
        ee_printf("[");

        for (uint32_t column = 0u; column < columns; ++column) {
            ee_printf("%6d", matrix[row * columns + column]);
        }

        ee_printf(" ]\n");
    }
}

static void print_double_matrix(volatile double *matrix, uint32_t rows, uint32_t columns) {
    for (uint32_t row = 0u; row < rows; ++row) {
        ee_printf("[");

        for (uint32_t column = 0u; column < columns; ++column) {
            ee_printf("%10.4f", matrix[row * columns + column]);
        }

        ee_printf(" ]\n");
    }
}
