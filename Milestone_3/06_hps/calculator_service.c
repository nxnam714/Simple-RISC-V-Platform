#define _DEFAULT_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

enum {
    REG_REQ_A = 0x00,
    REG_REQ_B = 0x04,
    REG_REQ_OP = 0x08,
    REG_REQ_SEQ = 0x0c,
    REG_RES_SEQ = 0x18,
    MAILBOX_BYTES = 0x1c,
    DEFAULT_TIMEOUT_MS = 5000
};

static volatile sig_atomic_t stopping;

typedef struct {
    const char *uart_path;
    const char *mem_path;
    uint64_t bridge_base;
    uint64_t bridge_span;
    uint64_t mailbox_offset;
    unsigned timeout_ms;
} options_t;

typedef struct {
    int fd;
    volatile uint32_t *registers;
    void *mapping;
    size_t mapping_length;
} mailbox_t;

static void on_signal(int signal_number)
{
    (void)signal_number;
    stopping = 1;
}

static void print_usage(const char *program)
{
    fprintf(stderr,
        "Usage: %s --uart DEVICE --bridge-base ADDRESS --bridge-span BYTES "
        "--mailbox-offset BYTES [--mem-device PATH] [--timeout-ms N]\n",
        program);
}

static bool parse_u64(const char *text, uint64_t *value)
{
    char *end = NULL;
    unsigned long long parsed;

    if (text[0] == '-') return false;
    errno = 0;
    parsed = strtoull(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0') return false;
    *value = (uint64_t)parsed;
    return true;
}

static bool parse_options(int argc, char **argv, options_t *options)
{
    int index;
    bool have_base = false;
    bool have_span = false;
    bool have_offset = false;

    memset(options, 0, sizeof(*options));
    options->mem_path = "/dev/mem";
    options->timeout_ms = DEFAULT_TIMEOUT_MS;

    for (index = 1; index < argc; ++index) {
        const char *name = argv[index];
        const char *value;

        if (index + 1 >= argc) return false;
        value = argv[++index];
        if (strcmp(name, "--uart") == 0) {
            options->uart_path = value;
        }
        else if (strcmp(name, "--mem-device") == 0) {
            options->mem_path = value;
        }
        else if (strcmp(name, "--bridge-base") == 0) {
            have_base = parse_u64(value, &options->bridge_base);
            if (!have_base) return false;
        }
        else if (strcmp(name, "--bridge-span") == 0) {
            have_span = parse_u64(value, &options->bridge_span);
            if (!have_span) return false;
        }
        else if (strcmp(name, "--mailbox-offset") == 0) {
            have_offset = parse_u64(value, &options->mailbox_offset);
            if (!have_offset) return false;
        }
        else if (strcmp(name, "--timeout-ms") == 0) {
            uint64_t timeout;
            if (!parse_u64(value, &timeout) || timeout == 0 || timeout > UINT_MAX) return false;
            options->timeout_ms = (unsigned)timeout;
        }
        else {
            return false;
        }
    }

    return options->uart_path != NULL &&
           have_base && have_span && have_offset &&
           options->bridge_span >= MAILBOX_BYTES &&
           options->mailbox_offset <= options->bridge_span - MAILBOX_BYTES;
}

static int configure_uart(const char *path)
{
    struct termios settings;
    int fd = open(path, O_RDWR | O_NOCTTY | O_CLOEXEC | O_SYNC);
    if (fd < 0) {
        perror("open UART");
        return -1;
    }

    if (tcgetattr(fd, &settings) != 0) {
        perror("tcgetattr");
        close(fd);
        return -1;
    }

    cfmakeraw(&settings);
    settings.c_cflag &= ~(CSIZE | PARENB | CSTOPB);
    settings.c_cflag |= CS8 | CLOCAL | CREAD;
#ifdef CRTSCTS
    settings.c_cflag &= ~CRTSCTS;
#endif
    settings.c_iflag &= ~(IXON | IXOFF | IXANY);
    settings.c_cc[VMIN] = 1;
    settings.c_cc[VTIME] = 0;

    if (cfsetispeed(&settings, B115200) != 0 ||
        cfsetospeed(&settings, B115200) != 0 ||
        tcsetattr(fd, TCSANOW, &settings) != 0) {
        perror("configure UART");
        close(fd);
        return -1;
    }
    tcflush(fd, TCIOFLUSH);
    return fd;
}

static bool map_mailbox(const options_t *options, mailbox_t *mailbox)
{
    long page_size = sysconf(_SC_PAGE_SIZE);
    uint64_t physical_address;
    uint64_t aligned_address;
    uint64_t page_offset;
    uint64_t required_length;

    memset(mailbox, 0, sizeof(*mailbox));
    mailbox->fd = open(options->mem_path, O_RDWR | O_SYNC | O_CLOEXEC);
    if (mailbox->fd < 0) {
        perror("open memory device");
        return false;
    }
    if (page_size <= 0) {
        fprintf(stderr, "invalid system page size\n");
        close(mailbox->fd);
        return false;
    }

    physical_address = options->bridge_base + options->mailbox_offset;
    if (physical_address < options->bridge_base) {
        fprintf(stderr, "bridge address overflow\n");
        close(mailbox->fd);
        return false;
    }
    if ((physical_address & 3u) != 0) {
        fprintf(stderr, "mailbox physical address must be 32-bit aligned\n");
        close(mailbox->fd);
        return false;
    }
    aligned_address = physical_address & ~((uint64_t)page_size - 1u);
    page_offset = physical_address - aligned_address;
    required_length = page_offset + MAILBOX_BYTES;
    if (required_length > SIZE_MAX || aligned_address > (uint64_t)INT64_MAX) {
        fprintf(stderr, "mailbox mapping is too large\n");
        close(mailbox->fd);
        return false;
    }

    mailbox->mapping_length = (size_t)required_length;
    mailbox->mapping = mmap(NULL, mailbox->mapping_length, PROT_READ | PROT_WRITE,
                            MAP_SHARED, mailbox->fd, (off_t)aligned_address);
    if (mailbox->mapping == MAP_FAILED) {
        perror("mmap mailbox");
        mailbox->mapping = NULL;
        close(mailbox->fd);
        return false;
    }
    mailbox->registers = (volatile uint32_t *)((uint8_t *)mailbox->mapping + page_offset);
    return true;
}

static void close_mailbox(mailbox_t *mailbox)
{
    if (mailbox->mapping != NULL) munmap(mailbox->mapping, mailbox->mapping_length);
    if (mailbox->fd >= 0) close(mailbox->fd);
}

static uint32_t mmio_read(mailbox_t *mailbox, unsigned offset)
{
    __sync_synchronize();
    uint32_t value = mailbox->registers[offset / sizeof(uint32_t)];
    __sync_synchronize();
    return value;
}

static void mmio_write(mailbox_t *mailbox, unsigned offset, uint32_t value)
{
    __sync_synchronize();
    mailbox->registers[offset / sizeof(uint32_t)] = value;
    __sync_synchronize();
}

static bool parse_request(char *line, int32_t *operand_a, int32_t *operand_b, uint32_t *operation)
{
    char *save = NULL;
    char *tokens[4];
    size_t count = 0;
    long values[3];
    size_t index;

    for (char *token = strtok_r(line, " \t\r\n", &save);
         token != NULL;
         token = strtok_r(NULL, " \t\r\n", &save)) {
        if (count == 4) return false;
        tokens[count++] = token;
    }
    if (count != 3) return false;

    for (index = 0; index < 3; ++index) {
        char *end = NULL;
        errno = 0;
        values[index] = strtol(tokens[index], &end, 10);
        if (errno != 0 || end == tokens[index] || *end != '\0') return false;
    }
    if (values[0] < -128 || values[0] > 127 ||
        values[1] < -128 || values[1] > 127 ||
        values[2] < 0 || values[2] > 3) return false;

    *operand_a = (int32_t)values[0];
    *operand_b = (int32_t)values[1];
    *operation = (uint32_t)values[2];
    return true;
}

static bool uart_write_all(int fd, const char *text)
{
    size_t remaining = strlen(text);
    while (remaining > 0) {
        ssize_t written = write(fd, text, remaining);
        if (written < 0 && errno == EINTR) continue;
        if (written <= 0) return false;
        text += written;
        remaining -= (size_t)written;
    }
    return true;
}

static bool wait_for_completion(mailbox_t *mailbox, uint32_t sequence, unsigned timeout_ms)
{
    struct timespec start;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &start);

    while (!stopping) {
        if (mmio_read(mailbox, REG_RES_SEQ) == sequence) return true;
        clock_gettime(CLOCK_MONOTONIC, &now);
        uint64_t elapsed_ms = (uint64_t)(now.tv_sec - start.tv_sec) * 1000u;
        if (now.tv_nsec >= start.tv_nsec) elapsed_ms += (uint64_t)(now.tv_nsec - start.tv_nsec) / 1000000u;
        else elapsed_ms -= (uint64_t)(start.tv_nsec - now.tv_nsec) / 1000000u;
        if (elapsed_ms >= timeout_ms) return false;
        struct timespec delay = {.tv_sec = 0, .tv_nsec = 1000000};
        nanosleep(&delay, NULL);
    }
    return false;
}

static bool submit_request(
    mailbox_t *mailbox,
    const options_t *options,
    int32_t operand_a,
    int32_t operand_b,
    uint32_t operation,
    int uart_fd
)
{
    uint32_t request_sequence = mmio_read(mailbox, REG_REQ_SEQ);
    uint32_t result_sequence = mmio_read(mailbox, REG_RES_SEQ);

    if (request_sequence != result_sequence &&
        !wait_for_completion(mailbox, request_sequence, options->timeout_ms)) {
        (void)uart_write_all(uart_fd, "ERR MCU timeout\n");
        return false;
    }

    request_sequence++;
    if (request_sequence == 0) request_sequence = 1;

    mmio_write(mailbox, REG_REQ_A, (uint32_t)operand_a);
    mmio_write(mailbox, REG_REQ_B, (uint32_t)operand_b);
    mmio_write(mailbox, REG_REQ_OP, operation);
    mmio_write(mailbox, REG_REQ_SEQ, request_sequence);

    if (!wait_for_completion(mailbox, request_sequence, options->timeout_ms)) {
        (void)uart_write_all(uart_fd, "ERR MCU timeout\n");
        return false;
    }

    return true;
}

static bool process_line(char *line, int uart_fd, mailbox_t *mailbox, const options_t *options)
{
    int32_t operand_a;
    int32_t operand_b;
    uint32_t operation;
    char original[256];

    (void)snprintf(original, sizeof(original), "%s", line);
    if (!parse_request(line, &operand_a, &operand_b, &operation)) {
        fprintf(stderr, "rejected malformed or out-of-range UART request: %s", original);
        return uart_write_all(uart_fd, "ERR expected: A B OP (signed 8-bit A/B, OP 0..3)\n");
    }
    return submit_request(mailbox, options, operand_a, operand_b, operation, uart_fd);
}

static int read_uart_line(int fd, char *line, size_t capacity)
{
    size_t length = 0;
    bool overflow = false;

    for (;;) {
        char byte;
        ssize_t count = read(fd, &byte, 1);
        if (count < 0 && errno == EINTR) {
            if (stopping) return -1;
            continue;
        }
        if (count <= 0) {
            if (count < 0) perror("read UART");
            return -1;
        }
        if (byte == '\n') {
            if (capacity > 0) line[length < capacity ? length : capacity - 1] = '\0';
            return overflow ? 0 : 1;
        }
        if (!overflow) {
            if (length + 1 < capacity) line[length++] = byte;
            else overflow = true;
        }
    }
}

int main(int argc, char **argv)
{
    options_t options;
    mailbox_t mailbox;
    int uart_fd;
    int exit_code = EXIT_SUCCESS;
    char line[256];
    struct sigaction action;

    if (!parse_options(argc, argv, &options)) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    if (options.bridge_base > UINT64_MAX - options.bridge_span) {
        fprintf(stderr, "bridge address range overflow\n");
        return EXIT_FAILURE;
    }

    memset(&action, 0, sizeof(action));
    action.sa_handler = on_signal;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGINT, &action, NULL) != 0 || sigaction(SIGTERM, &action, NULL) != 0) {
        perror("sigaction");
        return EXIT_FAILURE;
    }
    uart_fd = configure_uart(options.uart_path);
    if (uart_fd < 0) return EXIT_FAILURE;
    if (!map_mailbox(&options, &mailbox)) {
        close(uart_fd);
        return EXIT_FAILURE;
    }

    fprintf(stderr, "Calculator service ready; MCU results are displayed only on FPGA HEX digits.\n");
    while (!stopping) {
        int line_status = read_uart_line(uart_fd, line, sizeof(line));
        if (line_status < 0) {
            if (!stopping) exit_code = EXIT_FAILURE;
            break;
        }
        if (line_status == 0) {
            (void)uart_write_all(uart_fd, "ERR request line too long\n");
            fprintf(stderr, "discarded overlong UART line\n");
            continue;
        }
        if (!process_line(line, uart_fd, &mailbox, &options)) {
            if (!stopping) fprintf(stderr, "request processing failed\n");
        }
    }

    close_mailbox(&mailbox);
    close(uart_fd);
    return exit_code;
}
