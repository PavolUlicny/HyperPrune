CC ?= gcc

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

.PHONY: all clean run rebuild debug release

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

-include $(DEPS)
