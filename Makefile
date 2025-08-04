# https://spin.atomicobject.com/makefile-c-projects/

TARGET_EXEC ?= teeny
DEBUG_EXEC ?= teeny-debug

BUILD_DIR ?= ./builds/release
DEBUG_BUILD_DIR ?= ./builds/debug
SRC_DIRS ?= ./src ./extern

SRCS := $(shell find $(SRC_DIRS) -name *.c -or -name *.s)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEBUG_OBJS := $(SRCS:%=$(DEBUG_BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)
DEBUG_DEPS := $(DEBUG_OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d) 
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

# Compiler
CC := gcc

# Common flags for errors and warning
COMP_FLAGS := -Werror -Wall -Wextra -Wfloat-equal -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wstrict-overflow=5 -Wwrite-strings -Wcast-qual -Wswitch-enum -Wunreachable-code -Wformat=2 

# Release build flags
C_FLAGS := $(INC_FLAGS) $(COMP_FLAGS) \
	-MMD -MP -O3

# Debug build flags  
DEBUG_C_FLAGS := $(INC_FLAGS) $(COMP_FLAGS) \
	-MMD -MP -fsanitize=undefined,address -g -O0 -fno-omit-frame-pointer -DDZ_DEBUG                          # Enable debug macros

# Test build flags
TEST_C_FLAGS := $(DEBUG_C_FLAGS) -DDZ_TESTING=1  # Debug flags + testing macros

# Linker flags
LD_FLAGS := -fsanitize=undefined,address                   # Link AddressSanitizer
DEBUG_LD_FLAGS := -fsanitize=undefined,address -rdynamic   # AddressSanitizer + export symbols for backtraces


# =========================
# Build Targets
# =========================

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LD_FLAGS)

$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(C_FLAGS) -c $< -o $@


# =========================
# Debug Build Targets
# =========================

$(DEBUG_BUILD_DIR)/$(DEBUG_EXEC): $(DEBUG_OBJS)
	$(CC) $(DEBUG_OBJS) -o $@ $(DEBUG_LD_FLAGS)

$(DEBUG_BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(DEBUG_C_FLAGS) -c $< -o $@

# =========================
# Criterion tests
# =========================

TEST_DIR := ./tests
TEST_BUILD_DIR := ./builds/tests
TEST_EXEC ?= teeny-tests

TEST_SRCS := $(shell find $(TEST_DIR) -name *_test.c -o -name test_util.c)
PROD_SRCS_NO_MAIN := $(filter-out ./src/main.c,$(SRCS))
TEST_OBJS := $(TEST_SRCS:%=$(TEST_BUILD_DIR)/%.o)
PROD_TEST_OBJS := $(PROD_SRCS_NO_MAIN:%=$(TEST_BUILD_DIR)/%.o)

$(TEST_BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(TEST_C_FLAGS) -c $< -o $@

$(TEST_BUILD_DIR)/$(TEST_EXEC): $(TEST_OBJS) $(PROD_TEST_OBJS)
	$(CC) $^ -o $@ -lcriterion -pthread $(DEBUG_LD_FLAGS)

test: $(TEST_BUILD_DIR)/$(TEST_EXEC)
	$<

# =========================
# Phony Targets
# =========================

.PHONY: clean debug test format setup

debug: $(DEBUG_BUILD_DIR)/$(DEBUG_EXEC)

clean:
	$(RM) -r $(BUILD_DIR) $(DEBUG_BUILD_DIR) $(TEST_BUILD_DIR)

format:
	find src/ -name '*.c' -o -name '*.h' | xargs clang-format -i

setup:
	bear -- make clean && bear -- make

-include $(DEPS)
-include $(DEBUG_DEPS)

MKDIR_P ?= mkdir -p


