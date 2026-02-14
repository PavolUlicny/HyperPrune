# HyperPrune Makefile
# Provides debug/release builds and a PGO target for the ttt binary.

CC ?= gcc

# Detect compiler type for PGO flags
# Check for clang-cl first (unsupported), then clang variants, then GCC
ifneq ($(findstring clang-cl,$(CC)),)
	$(error clang-cl is not supported. Use clang or GCC instead.)
else ifneq ($(findstring clang,$(CC)),)
	# Clang-based compiler (clang, /usr/bin/clang, clang-14, etc.)
	PGO_GENERATE := -fprofile-instr-generate
	PGO_USE := -fprofile-instr-use=default.profdata
	PGO_MERGE := llvm-profdata merge -output=default.profdata default.profraw
else
	# GCC or compatible
	PGO_GENERATE := -fprofile-generate
	PGO_USE := -fprofile-use
	PGO_MERGE := @true
endif

BUILD ?= release

SRCDIR := src
OBJDIR := build/$(BUILD)
TARGET := ttt

SOURCES := \
	$(SRCDIR)/main.c \
	$(SRCDIR)/TicTacToe/tic_tac_toe.c \
	$(SRCDIR)/MiniMax/mini_max.c \
	$(SRCDIR)/MiniMax/transposition.c

OBJECTS := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
DEPS := $(OBJECTS:.o=.d)

BOARD_SIZE ?= 3

# Validate BOARD_SIZE at Make level (prevents shell arithmetic errors)
ifeq ($(shell test $(BOARD_SIZE) -ge 3 -a $(BOARD_SIZE) -le 8 && echo ok),ok)
    # Valid range
else
    $(error BOARD_SIZE must be between 3 and 8 (got $(BOARD_SIZE)))
endif

WARNINGS := -Wall -Wextra
BASE_CFLAGS := -std=c11 -MMD -MP -pipe -DBOARD_SIZE=$(BOARD_SIZE)

DEBUG_CFLAGS := -O0 -g
RELEASE_CFLAGS := -O3 -march=native -flto -fomit-frame-pointer -fno-semantic-interposition -DNDEBUG
PORTABLE_CFLAGS := -O3 -fomit-frame-pointer -DNDEBUG

ifeq ($(BUILD),debug)
MODE_CFLAGS := $(DEBUG_CFLAGS)
MODE_LDFLAGS :=
else ifeq ($(BUILD),portable)
MODE_CFLAGS := $(PORTABLE_CFLAGS)
MODE_LDFLAGS :=
else
MODE_CFLAGS := $(RELEASE_CFLAGS)
MODE_LDFLAGS := -flto
endif

CFLAGS := $(WARNINGS) $(BASE_CFLAGS) $(MODE_CFLAGS)
LDFLAGS := $(MODE_LDFLAGS) -lm

.PHONY: all clean run rebuild debug release portable pgo pgo-clean install uninstall test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "[LINK ] $@"
	@$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	@echo "[CC   ] $<"
	@$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

rebuild: clean all

debug:
	@$(MAKE) BUILD=debug all

release:
	@$(MAKE) BUILD=release all

portable:
	@$(MAKE) BUILD=portable all

clean:
	@echo "[CLEAN] removing build artifacts"
	@rm -rf build $(TARGET)

# Installation
PREFIX ?= /usr/local
DESTDIR ?=

install: $(TARGET)
	@echo "[INSTALL] installing to $(DESTDIR)$(PREFIX)/bin"
	@mkdir -p $(DESTDIR)$(PREFIX)/bin
	@install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/

uninstall:
	@echo "[UNINSTALL] removing from $(DESTDIR)$(PREFIX)/bin"
	@rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)

# Profile-Guided Optimization (PGO)
# Usage: make pgo [BOARD_SIZE=N]
# Profiling workload: games = max(10000, 1,000,000 / (BOARD_SIZE - 2)^2)
pgo:
	@echo "[PGO  ] Using compiler: $(CC)"
	@echo "[PGO  ] Step 1/3: Building with profiling instrumentation..."
	@$(MAKE) pgo-clean > /dev/null 2>&1
	@$(MAKE) clean > /dev/null
	@$(CC) -std=c11 -Wall -Wextra -O3 -march=native $(PGO_GENERATE) \
		-fomit-frame-pointer -DNDEBUG -pipe -DBOARD_SIZE=$(BOARD_SIZE) \
		$(SOURCES) -o $(TARGET) -lm
	@PROFILE_GAMES=$$((1000000 / (($(BOARD_SIZE) - 2) * ($(BOARD_SIZE) - 2)))); \
	if [ $$PROFILE_GAMES -lt 10000 ]; then PROFILE_GAMES=10000; fi; \
	echo "[PGO  ] Step 2/3: Running workload to collect profile data ($$PROFILE_GAMES games)..."; \
	./$(TARGET) -s $$PROFILE_GAMES -q
	@$(PGO_MERGE)
	@echo "[PGO  ] Step 3/3: Rebuilding with profile-guided optimizations..."
	@$(CC) -std=c11 -Wall -Wextra -O3 -march=native $(PGO_USE) -flto \
		-fomit-frame-pointer -DNDEBUG -pipe -DBOARD_SIZE=$(BOARD_SIZE) \
		$(SOURCES) -o $(TARGET) -lm
	@$(MAKE) pgo-clean > /dev/null 2>&1
	@echo "[PGO  ] PGO-optimized binary ready"

pgo-clean:
	@echo "[CLEAN] removing PGO profile data"
	@rm -f *.gcda *.gcno default.profraw default.profdata

# Testing
TEST_DIR := test
TEST_UNITY_DIR := $(TEST_DIR)/unity
TEST_SOURCES := \
	$(TEST_DIR)/test_runner.c \
	$(TEST_DIR)/test_bitboard.c \
	$(TEST_DIR)/test_minimax.c \
	$(TEST_DIR)/test_zobrist.c \
	$(TEST_DIR)/test_transposition_table.c \
	$(TEST_DIR)/test_game_scenarios.c \
	$(TEST_DIR)/test_edge_cases.c \
	$(TEST_DIR)/test_correctness.c

# Core objects (excluding main.o)
CORE_SOURCES := \
	$(SRCDIR)/TicTacToe/tic_tac_toe.c \
	$(SRCDIR)/MiniMax/mini_max.c \
	$(SRCDIR)/MiniMax/transposition.c

TEST_TARGET := $(TEST_DIR)/test_runner

.PHONY: test
test: $(TEST_TARGET)
	@echo "[TEST ] Running test suite..."
	@$(TEST_TARGET)
	@echo "[TEST ] All tests passed ✓"

$(TEST_TARGET): $(TEST_SOURCES) $(CORE_SOURCES) $(TEST_UNITY_DIR)/unity.c
	@echo "[BUILD] Test suite..."
	@$(CC) $(WARNINGS) -std=c11 -DBOARD_SIZE=$(BOARD_SIZE) -I$(TEST_UNITY_DIR) \
		$(TEST_SOURCES) $(CORE_SOURCES) $(TEST_UNITY_DIR)/unity.c \
		-o $(TEST_TARGET) -lm

-include $(DEPS)
