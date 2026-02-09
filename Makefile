# =============================================================================
# MiniMax Tic-Tac-Toe Makefile
# =============================================================================
# Requirements:
#   - POSIX-compliant shell (bash, sh, zsh)
#   - GCC 7+ or Clang 6+ with C11 support
#   - Platforms: Linux, macOS, WSL, MinGW, MSYS2
#
# Note: Native Windows (cmd.exe/PowerShell) is not supported.
#       Use WSL, MSYS2, or MinGW for Windows builds.
#
# Build flags:
#   - Uses -march=native for CPU-specific optimizations
#   - Binaries are NOT portable to other CPU architectures
#   - For distributable builds, remove -march=native
# =============================================================================

CC ?= gcc

# Detect compiler type for PGO flags
# Note: clang-cl (MSVC-compatible Clang) is not supported
ifeq ($(CC),clang)
	PGO_GENERATE := -fprofile-instr-generate
	PGO_USE := -fprofile-instr-use=default.profdata
	PGO_MERGE := llvm-profdata merge -output=default.profdata default.profraw
else ifeq ($(findstring clang,$(CC)),clang)
	# Clang variants (but not clang-cl)
	ifneq ($(findstring clang-cl,$(CC)),clang-cl)
		PGO_GENERATE := -fprofile-instr-generate
		PGO_USE := -fprofile-instr-use=default.profdata
		PGO_MERGE := llvm-profdata merge -output=default.profdata default.profraw
	else
		$(error clang-cl is not supported. Use clang or GCC instead.)
	endif
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
RELEASE_CFLAGS := -O3 -march=native -flto -fomit-frame-pointer -DNDEBUG

ifeq ($(BUILD),debug)
MODE_CFLAGS := $(DEBUG_CFLAGS)
MODE_LDFLAGS :=
else
MODE_CFLAGS := $(RELEASE_CFLAGS)
MODE_LDFLAGS := -flto
endif

CFLAGS := $(WARNINGS) $(BASE_CFLAGS) $(MODE_CFLAGS)
LDFLAGS := $(MODE_LDFLAGS) -lm

.PHONY: all clean run rebuild debug release pgo pgo-clean

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

clean:
	@echo "[CLEAN] removing build artifacts"
	@rm -rf build $(TARGET)

# Profile-Guided Optimization (PGO) - 3-step process for maximum performance
# Usage: make pgo [BOARD_SIZE=N]
# Requirements:
#   - POSIX shell (uses shell arithmetic and conditionals)
#   - GCC or Clang compiler (clang-cl not supported)
# Profiling workload: Dynamically scaled based on board size
#   Formula: games = max(10000, 1,000,000 / (BOARD_SIZE - 2)^2)
#   3x3: 1M games, 4x4: 250K games, 5x5: 111K games, etc.
#   Empirically derived from benchmarking on 3x3 and 4x4
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

-include $(DEPS)
