CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror
BUILD_DIR := build

.PHONY: all clean
all: $(BUILD_DIR)/calculator_service

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/calculator_service: calculator_service.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f calculator_service $(BUILD_DIR)/calculator_service
