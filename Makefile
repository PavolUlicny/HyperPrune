CC ?= gcc

# Detect compiler type for PGO flags
ifeq ($(CC),clang)
	PGO_GENERATE := -fprofile-instr-generate
	PGO_USE := -fprofile-instr-use=default.profdata
	PGO_MERGE := llvm-profdata merge -output=default.profdata default.profraw
else ifeq ($(findstring clang,$(CC)),clang)
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
LDFLAGS := $(MODE_LDFLAGS)

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
# Usage: make pgo
# Compiler support: GCC (default), Clang (auto-detected)
# Result: ~20-25% performance improvement over standard release build
pgo:
	@echo "[PGO  ] Using compiler: $(CC)"
	@echo "[PGO  ] Step 1/3: Building with profiling instrumentation..."
	@$(MAKE) pgo-clean > /dev/null 2>&1
	@$(MAKE) clean > /dev/null
	@$(CC) -std=c11 -Wall -Wextra -O3 -march=native $(PGO_GENERATE) -fomit-frame-pointer -DNDEBUG -pipe -DBOARD_SIZE=$(BOARD_SIZE) $(SOURCES) -o $(TARGET)
	@echo "[PGO  ] Step 2/3: Running workload to collect profile data (5M games)..."
	@./$(TARGET) -s 5000000 -q
	@$(PGO_MERGE)
	@echo "[PGO  ] Step 3/3: Rebuilding with profile-guided optimizations..."
	@$(CC) -std=c11 -Wall -Wextra -O3 -march=native $(PGO_USE) -flto -fomit-frame-pointer -DNDEBUG -pipe -DBOARD_SIZE=$(BOARD_SIZE) $(SOURCES) -o $(TARGET)
	@$(MAKE) pgo-clean > /dev/null 2>&1
	@echo "[PGO  ] PGO-optimized binary ready"

pgo-clean:
	@echo "[CLEAN] removing PGO profile data"
	@rm -f *.gcda *.gcno default.profraw default.profdata

-include $(DEPS)
