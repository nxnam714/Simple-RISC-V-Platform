#include <stdint.h>
#define EXPECT_PRIME_COUNT 5133u
static const uint32_t expect_primes_last[5] = {49943, 49957, 49991, 49993, 49999};
#define EXPECT_SLOPE 24467
#define EXPECT_INTERCEPT -3067
static const uint16_t expect_big[3][12] = {{0, 8000, 3084, 8636, 9105, 8121, 2859, 6525, 2, 0, 0, 0}, {5376, 320, 4967, 9401, 2822, 6002, 7650, 126, 0, 0, 0, 0}, {5075, 6191, 1792, 4848, 5422, 3, 0, 0, 0, 0, 0, 0}};
static const uint32_t expect_pascal[13] = {1, 12, 66, 220, 495, 792, 924, 792, 495, 220, 66, 12, 1};
#define EXPECT_CHECKSUM 0x91706e35u
#define EXPECT_DET -142
static const int32_t expect_solution[3] = {2, 3, -1};
static const int32_t expect_sine[5] = {0, 5000, 7071, 8660, 10000};
#define TEXT_PRIMES "5133 primes below N, last 5 = [49943, 49957, 49991, 49993, 49999]"
#define TEXT_LINE "y = 2.4467x - 0.3067"
#define TEXT_BIG "30!    = 265252859812191058636308480000000\n           2^100  = 1267650600228229401496703205376\n           F(100) = 354224848179261915075"
#define TEXT_PASCAL_SUM "4096 = 2^12"
#define TEXT_CHECKSUM "0x91706e35"
#define TEXT_DET "-142"
#define TEXT_SOLUTION "[x, y, z] = [2, 3, -1]"
#define TEXT_SINE "sin = [0.0000, 0.5000, 0.7071, 0.8660, 1.0000]"
