#include "app_runtime.h"

#define DISK_COUNT 12u

static volatile uint8_t disk_rod[DISK_COUNT + 1u];
static volatile uint32_t disk_moves[DISK_COUNT + 1u];

static uint32_t move_count;
static uint32_t illegal_moves;
static uint32_t move_checksum;

static void initialize_puzzle(void);
static void solve_hanoi(uint32_t disks, uint8_t source, uint8_t destination, uint8_t spare);
static void move_disk(uint32_t disk, uint8_t source, uint8_t destination);
static uint32_t should_print_move(void);
static uint32_t validate_solution(void);

int main(void) {
    uint32_t expected_moves = (1u << DISK_COUNT) - 1u;
    uint32_t errors;

    initialize_puzzle();

    ee_printf("\n\nHCMUT - Computer Architecture Laboratory - DOELAB\n\n");
    ee_printf("Tower of Hanoi with legality checking\n\nDisks: %u\n", DISK_COUNT);

    solve_hanoi(DISK_COUNT, (uint8_t)'A', (uint8_t)'C', (uint8_t)'B');
    errors = validate_solution();

    ee_printf("\nTotal moves:     %u (expected %u)\n", move_count, expected_moves);
    ee_printf("Move checksum:   0x%08x (expected 0xd25f5c74)\n", move_checksum);
    ee_printf("Legality errors: %u\n", illegal_moves);
    ee_printf("Final result:    %s\n\n", errors == 0u ? "PASS" : "FAIL");

    app_finish();
}



static void initialize_puzzle(void) {
    move_count    = 0u;
    illegal_moves = 0u;
    move_checksum = 0u;

    for (uint32_t disk = 1u; disk <= DISK_COUNT; ++disk) {
        disk_rod[disk]   = (uint8_t)'A';
        disk_moves[disk] = 0u;
    }
}


static void solve_hanoi(uint32_t disks, uint8_t source, uint8_t destination, uint8_t spare) {
    if (disks == 0u) return;

    solve_hanoi(disks - 1u, source, spare, destination);
    move_disk(disks, source, destination);
    solve_hanoi(disks - 1u, spare, destination, source);
}



static void move_disk(uint32_t disk, uint8_t source, uint8_t destination) {
    ++move_count;

    if (disk_rod[disk] != source)   ++illegal_moves;

    for (uint32_t smaller_disk = 1u; smaller_disk < disk; ++smaller_disk) {
        uint8_t smaller_rod = disk_rod[smaller_disk];

        if (smaller_rod == source || smaller_rod == destination)        ++illegal_moves;
    }

    disk_rod[disk] = destination;
    ++disk_moves[disk];

    move_checksum  = (move_checksum << 5) ^ (move_checksum >> 27);
    move_checksum ^= disk << 16;
    move_checksum ^= (uint32_t)source << 8;
    move_checksum ^= (uint32_t)destination;

    if (should_print_move() != 0u) {
        ee_printf("Move %4u: disk %2u  %c -> %c\n", move_count, disk, source, destination);
    }
}




static uint32_t should_print_move(void) {
    uint32_t expected_moves = (1u << DISK_COUNT) - 1u;
    uint32_t near_start     = move_count <= 8u;
    uint32_t near_end       = move_count > expected_moves - 8u;
    uint32_t checkpoint     = (move_count & 511u) == 0u;

    return near_start || near_end || checkpoint;
}

static uint32_t validate_solution(void) {
    uint32_t expected_moves = (1u << DISK_COUNT) - 1u;
    uint32_t errors = illegal_moves;

    for (uint32_t disk = 1u; disk <= DISK_COUNT; ++disk) {
        uint32_t expected_disk_moves = 1u << (DISK_COUNT - disk);

        if (disk_rod[disk] != (uint8_t)'C')             ++errors;
        if (disk_moves[disk] != expected_disk_moves)    ++errors;
    }

    if (move_count != expected_moves)       ++errors;
    if (move_checksum != 0xd25f5c74u)       ++errors;

    return errors;
}
