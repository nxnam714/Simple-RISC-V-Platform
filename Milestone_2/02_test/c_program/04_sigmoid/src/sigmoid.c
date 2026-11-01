#include "app_runtime.h"

#define CURVE_COUNT 17u
#define SAMPLE_COUNT 4u
#define FEATURE_COUNT 5u
#define CLASS_COUNT 3u

static const int32_t curve_inputs[CURVE_COUNT] = {
    -937, -681, -423, -257, -131, -53, -11, -1, 0,
       1,   11,   53,  131,  257, 423, 681, 937
};

static const double expected_curve[CURVE_COUNT] = {
    0.000085236123, 0.001101478308, 0.014343656063,
    0.071094304084, 0.212486844123, 0.370516888033,
    0.472527695655, 0.497500020833, 0.500000000000,
    0.502499979167, 0.527472304345, 0.629483111967,
    0.787513155877, 0.928905695916, 0.985656343937,
    0.998898521692, 0.999914763877
};

static const int32_t features[SAMPLE_COUNT][FEATURE_COUNT] = {
    { 125,  -75, 200,   30, -110},
    {-210,  160,  45,  -95,   80},
    {-108,  164, 195, -247,  138},
    {-130,  -40, 175,  205,   90}
};

static const int32_t weights[CLASS_COUNT][FEATURE_COUNT] = {
    { 80, -120,   50,  35, -65},
    {-45,   90, -110,  70, 125},
    {115,   25,   60, -95,  40}
};

static const int32_t biases[CLASS_COUNT] = {-30, 55, -85};
static const uint32_t expected_classes[SAMPLE_COUNT] = {0u, 1u, 2u, 1u};
static volatile double curve_results[CURVE_COUNT];

static void print_inputs(void);
static void evaluate_curve(double one, double quarter, double scale, double tolerance, volatile uint32_t *passed, volatile uint32_t *total);
static void evaluate_classifier(double one, double scale, volatile uint32_t *passed, volatile uint32_t *total);
static double absolute_double(double value);
static double exp_negative(double positive_value);
static double reciprocal_between_one_and_two(double value);
static double sigmoid(double value);

int main(void) {
    volatile uint32_t passed_checks = 0u;
    volatile uint32_t total_checks = 0u;
    volatile uint32_t one_integer = 1u;
    volatile uint32_t quarter_high_word = 0x3fd00000u;
    volatile uint32_t quarter_low_word = 0x00000000u;
    volatile uint32_t hundredth_high_word = 0x3f847ae1u;
    volatile uint32_t hundredth_low_word = 0x47ae147bu;
    double one     = (double)one_integer;
    double quarter = double_from_words(quarter_high_word, quarter_low_word);
    double scale   = double_from_words(hundredth_high_word, hundredth_low_word);
    double tolerance = 0.000002;

    ee_printf("\n\nHCMUT - Computer Architecture Laboratory - DOELAB\n\n");
    ee_printf("Software double-precision sigmoid and classifier\n\n");

    print_inputs();

    ee_printf("\n=== Result ===\n");
    evaluate_curve(one, quarter, scale, tolerance, &passed_checks, &total_checks);
    evaluate_classifier(one, scale, &passed_checks, &total_checks);

    ee_printf("\nSelf-check: %u/%u  %s\n\n", passed_checks, total_checks, passed_checks == total_checks ? "PASS" : "FAIL");

    app_finish();
}

static void print_inputs(void) {
    ee_printf("=== Input ===\n");
    ee_printf("Curve inputs (x0.01):\n[");

    for (uint32_t index = 0u; index < CURVE_COUNT; ++index) {
        if (index != 0u) {
            ee_printf(", ");
        }

        ee_printf("%d", curve_inputs[index]);
    }

    ee_printf("]\n\nClassifier features (x0.01):\n");

    for (uint32_t sample = 0u; sample < SAMPLE_COUNT; ++sample) {
        ee_printf("Sample %u: [", sample);

        for (uint32_t feature = 0u; feature < FEATURE_COUNT; ++feature) {
            if (feature != 0u) {
                ee_printf(", ");
            }

            ee_printf("%d", features[sample][feature]);
        }

        ee_printf("]\n");
    }

    ee_printf("\nClassifier weights (x0.01):\n");

    for (uint32_t class_index = 0u; class_index < CLASS_COUNT; ++class_index) {
        ee_printf("Class %u:  [", class_index);

        for (uint32_t feature = 0u; feature < FEATURE_COUNT; ++feature) {
            if (feature != 0u) {
                ee_printf(", ");
            }

            ee_printf("%d", weights[class_index][feature]);
        }

        ee_printf("]  bias=%d\n", biases[class_index]);
    }
}

static void evaluate_curve(double one, double quarter, double scale, double tolerance,
                           volatile uint32_t *passed, volatile uint32_t *total) {
    ee_printf("Input       Sigmoid        Derivative\n");

    for (uint32_t index = 0u; index < CURVE_COUNT; ++index) {
        double input = (double)curve_inputs[index] * scale;
        double result = sigmoid(input);
        double derivative = result * (one - result);

        curve_results[index] = result;
        ee_printf("%8.2f    %.12f    %.12f\n", input, result, derivative);

        ++*total;
        if (absolute_double(result - expected_curve[index]) <= tolerance) {
            ++*passed;
        }

        ++*total;
        if (derivative >= -tolerance && derivative <= quarter + tolerance) {
            ++*passed;
        }

        if (index != 0u) {
            ++*total;
            if (result > curve_results[index - 1u]) {
                ++*passed;
            }
        }
    }

    for (uint32_t index = 0u; index < CURVE_COUNT / 2u; ++index) {
        uint32_t mirror_index = CURVE_COUNT - 1u - index;
        double symmetric_sum = curve_results[index] + curve_results[mirror_index];

        ++*total;
        if (absolute_double(symmetric_sum - one) <= tolerance) {
            ++*passed;
        }
    }
}

static void evaluate_classifier(double one, double scale,
                                volatile uint32_t *passed, volatile uint32_t *total) {
    ee_printf("\nClassifier output:\n");

    for (uint32_t sample = 0u; sample < SAMPLE_COUNT; ++sample) {
        double best_probability = -one;
        uint32_t selected_class = 0u;

        ee_printf("Sample %u: ", sample);

        for (uint32_t class_index = 0u; class_index < CLASS_COUNT; ++class_index) {
            double score = (double)biases[class_index] * scale;

            for (uint32_t feature = 0u; feature < FEATURE_COUNT; ++feature) {
                double input = (double)features[sample][feature] * scale;
                double weight = (double)weights[class_index][feature] * scale;

                score += input * weight;
            }

            double probability = sigmoid(score);

            ee_printf("C%u(score=%.4f, p=%.8f) ", class_index, score, probability);

            if (probability > best_probability) {
                best_probability = probability;
                selected_class = class_index;
            }
        }

        ++*total;
        if (selected_class == expected_classes[sample]) {
            ++*passed;
        }

        ee_printf("=> class %u\n", selected_class);
    }
}


static double absolute_double(double value) {
    if (value < 0.0) return -value;
    return value;
}

static double exp_negative(double positive_value) {
    volatile uint32_t one_integer = 1u;
    volatile uint32_t scale_high_word = 0x3e700000u;
    volatile uint32_t scale_low_word = 0x00000000u;
    double scale = double_from_words(scale_high_word, scale_low_word);
    double result = (double)one_integer - positive_value * scale;

    for (uint32_t iteration = 0u; iteration < 24u; ++iteration) {
        result *= result;
    }

    return result;
}

static double reciprocal_between_one_and_two(double value) {
    volatile uint32_t two_integer = 2u;
    volatile uint32_t half_high_word = 0x3fe00000u;
    volatile uint32_t half_low_word = 0x00000000u;
    double two = (double)two_integer;
    double estimate = double_from_words(half_high_word, half_low_word);

    for (uint32_t iteration = 0u; iteration < 6u; ++iteration) {
        estimate *= two - value * estimate;
    }

    return estimate;
}

static double sigmoid(double value) {
    volatile uint32_t one_integer = 1u;
    double one = (double)one_integer;
    double magnitude;
    double positive_result;

    if (value < 0.0)    magnitude = -value;
    else                magnitude = value;

    positive_result = reciprocal_between_one_and_two(one + exp_negative(magnitude));

    if (value >= 0.0) {
        return positive_result;
    }

    return one - positive_result;
}
