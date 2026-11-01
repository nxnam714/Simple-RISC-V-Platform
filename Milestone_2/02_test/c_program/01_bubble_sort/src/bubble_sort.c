#include "app_runtime.h"

#define VALUE_COUNT 42u

typedef void (*sort_function_t)(volatile int32_t *values, uint32_t count);

static const int32_t input_values[VALUE_COUNT] = {
             -2,        -238,          -1,   -73224908,
       60628354,  -706631611,        -683, -1947085240,
            911,        7461,        -607,        -687,
              8,          42,       -7123,   372223513,
     -850971922,           9,           3,         774,
     -519994153,           0,  2147483647,         -10,
      355733392,  2029180876,        1388,          42,
     1728875997,        -125,        2683,       -1969,
     1240427769,    60628355,       -4435,  2010525912,
    -2147483647 - 1,        2600,   574044430,        4911,
           -456,   890138991
};

static const int32_t expected_values[VALUE_COUNT] = {
    -2147483647 - 1, -1947085240,  -850971922,  -706631611,
       -519994153,   -73224908,       -7123,       -4435,
            -1969,        -687,        -683,        -607,
             -456,        -238,        -125,         -10,
               -2,          -1,           0,           3,
                8,           9,          42,          42,
              774,         911,        1388,        2600,
             2683,        4911,        7461,    60628354,
         60628355,   355733392,   372223513,   574044430,
        890138991,  1240427769,  1728875997,  2010525912,
       2029180876,  2147483647
};

static volatile int32_t merge_scratch[VALUE_COUNT];

static void copy_values(volatile int32_t *destination, const int32_t *source, uint32_t count);
static void copy_working_values(volatile int32_t *destination, volatile int32_t *source, uint32_t count);
static void print_values(volatile int32_t *values, uint32_t count);
static uint32_t values_match(volatile int32_t *values, const int32_t *expected, uint32_t count);
static void run_sort(uint32_t number, const char *name, sort_function_t sort_function,
                     volatile int32_t *source, volatile int32_t *work);
static void swap_values(volatile int32_t *left, volatile int32_t *right);
static void bubble_sort(volatile int32_t *values, uint32_t count);
static void insertion_sort(volatile int32_t *values, uint32_t count);
static void selection_sort(volatile int32_t *values, uint32_t count);
static void quick_sort(volatile int32_t *values, uint32_t count);
static void quick_sort_range(volatile int32_t *values, int32_t left, int32_t right);
static void merge_sort(volatile int32_t *values, uint32_t count);
static void merge_sort_range(volatile int32_t *values, uint32_t begin, uint32_t end);
static void heap_sort(volatile int32_t *values, uint32_t count);
static void sift_down(volatile int32_t *values, uint32_t root, uint32_t count);

int main(void) {
    volatile int32_t original[VALUE_COUNT];
    volatile int32_t work[VALUE_COUNT];

    copy_values(original, input_values, VALUE_COUNT);

    ee_printf("\n\nHCMUT - Computer Architecture Laboratory - DOELAB\n\n");
    ee_printf("Sorting-algorithm comparison\n\nInput Array: ");
    print_values(original, VALUE_COUNT);
    ee_printf("\n");

    run_sort(1u, "Bubble", bubble_sort, original, work);
    run_sort(2u, "Insertion", insertion_sort, original, work);
    run_sort(3u, "Selection", selection_sort, original, work);
    run_sort(4u, "Quick", quick_sort, original, work);
    run_sort(5u, "Merge", merge_sort, original, work);
    run_sort(6u, "Heap", heap_sort, original, work);

    ee_printf("\n");
    app_finish();
}

static void copy_values(volatile int32_t *destination, const int32_t *source, uint32_t count) {
    for (uint32_t index = 0u; index < count; ++index) {
        destination[index] = source[index];
    }
}

static void copy_working_values(volatile int32_t *destination, volatile int32_t *source, uint32_t count) {
    for (uint32_t index = 0u; index < count; ++index) {
        destination[index] = source[index];
    }
}

static void print_values(volatile int32_t *values, uint32_t count) {
    ee_printf("[");

    for (uint32_t index = 0u; index < count; ++index) {
        if (index != 0u) ee_printf(", ");
        ee_printf("%d", values[index]);
    }

    ee_printf("]\n");
}

static uint32_t values_match(volatile int32_t *values, const int32_t *expected, uint32_t count) {
    for (uint32_t index = 0u; index < count; ++index) {
        if (values[index] != expected[index]) {
            return 0u;
        }
    }

    return 1u;
}

static void run_sort(uint32_t number, const char *name, sort_function_t sort_function,
                     volatile int32_t *source, volatile int32_t *work) {
    uint32_t passed;

    copy_working_values(work, source, VALUE_COUNT);
    sort_function(work, VALUE_COUNT);
    passed = values_match(work, expected_values, VALUE_COUNT);

    ee_printf("%u. %s Sort: ", number, name);
    print_values(work, VALUE_COUNT);
    ee_printf("==> Result: %s\n\n", passed != 0u ? "PASS" : "FAIL");
}

static void swap_values(volatile int32_t *left, volatile int32_t *right) {
    int32_t temporary = *left;

    *left = *right;
    *right = temporary;
}

static void bubble_sort(volatile int32_t *values, uint32_t count) {
    for (uint32_t end = count; end > 1u; --end) {
        for (uint32_t index = 0u; index + 1u < end; ++index) {
            if (values[index] > values[index + 1u]) {
                swap_values(&values[index], &values[index + 1u]);
            }
        }
    }
}

static void insertion_sort(volatile int32_t *values, uint32_t count) {
    for (uint32_t index = 1u; index < count; ++index) {
        int32_t value = values[index];
        uint32_t position = index;

        while (position != 0u && values[position - 1u] > value) {
            values[position] = values[position - 1u];
            --position;
        }

        values[position] = value;
    }
}

static void selection_sort(volatile int32_t *values, uint32_t count) {
    for (uint32_t index = 0u; index + 1u < count; ++index) {
        uint32_t minimum_index = index;

        for (uint32_t scan = index + 1u; scan < count; ++scan) {
            if (values[scan] < values[minimum_index]) {
                minimum_index = scan;
            }
        }

        if (minimum_index != index) {
            swap_values(&values[index], &values[minimum_index]);
        }
    }
}

static void quick_sort(volatile int32_t *values, uint32_t count) {
    if (count == 0u) return;

    quick_sort_range(values, 0, (int32_t)count - 1);
}

static void quick_sort_range(volatile int32_t *values, int32_t left, int32_t right) {
    int32_t low = left;
    int32_t high = right;
    int32_t middle = left + (right - left) / 2;
    int32_t pivot = values[middle];

    while (low <= high) {
        while (values[low]  < pivot)     ++low;
        while (values[high] > pivot)     --high;


        if (low <= high) {
            swap_values(&values[low], &values[high]);
            ++low;
            --high;
        }
    }

    if (left < high)        quick_sort_range(values, left, high);
    if (low < right)        quick_sort_range(values, low, right);
}

static void merge_sort(volatile int32_t *values, uint32_t count) {
    merge_sort_range(values, 0u, count);
}

static void merge_sort_range(volatile int32_t *values, uint32_t begin, uint32_t end) {
    if (end - begin <= 1u) return;

    uint32_t middle = begin + (end - begin) / 2u;
    uint32_t left   = begin;
    uint32_t right  = middle;
    uint32_t output = begin;

    merge_sort_range(values, begin, middle);
    merge_sort_range(values, middle, end);

    while (left < middle && right < end) {
        if (values[left] <= values[right]) {
            merge_scratch[output] = values[left];
            ++left;
        }
        else {
            merge_scratch[output] = values[right];
            ++right;
        }

        ++output;
    }

    while (left < middle) {
        merge_scratch[output] = values[left];
        ++left;
        ++output;
    }

    while (right < end) {
        merge_scratch[output] = values[right];
        ++right;
        ++output;
    }

    for (output = begin; output < end; ++output) {
        values[output] = merge_scratch[output];
    }
}



static void heap_sort(volatile int32_t *values, uint32_t count) {
    for (uint32_t root = count / 2u; root != 0u; --root) {
        sift_down(values, root - 1u, count);
    }

    for (uint32_t end = count; end > 1u; --end) {
        swap_values(&values[0], &values[end - 1u]);
        sift_down(values, 0u, end - 1u);
    }
}



static void sift_down(volatile int32_t *values, uint32_t root, uint32_t count) {
    for (;;) {
        uint32_t left_child = root * 2u + 1u;
        uint32_t right_child = left_child + 1u;
        uint32_t largest = root;

        if (left_child  < count && values[left_child] > values[largest])             largest = left_child;
        if (right_child < count && values[right_child] > values[largest])           largest = right_child;

        if (largest == root) return;

        swap_values(&values[root], &values[largest]);
        root = largest;
    }
}
