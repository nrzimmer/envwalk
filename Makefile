# Compiler
CC = gcc

# Common flags
CFLAGS_COMMON = -std=c23 -Wall -Wextra -Wpedantic -Werror

# Debug / Release flags
CFLAGS_DEBUG = -ggdb -O0 -rdynamic -fno-omit-frame-pointer
CFLAGS_RELEASE = -O3 -march=native -mtune=native -DNDEBUG

# Default to debug
CFLAGS = $(CFLAGS_COMMON) $(CFLAGS_DEBUG)

# Directories
OBJDIR = obj

# Target executable
TARGET = envwalk

# Source and object files
SRCS = $(filter-out test.c, $(wildcard *.c))
OBJS = $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))

# Test sources (all except envwalk.c which owns main + NOB_IMPLEMENTATION)
TEST_SRCS = test.c $(filter-out envwalk.c, $(SRCS))
TEST_OBJS = $(patsubst %.c,$(OBJDIR)/test_%.o,$(TEST_SRCS))
HOOKS = $(wildcard hook.*)
HOOKS_OBJ = $(patsubst hook.%,$(OBJDIR)/hook_%.o,$(HOOKS))

# Default target
all: $(TARGET)

# Release target (override flags)
release: CFLAGS = $(CFLAGS_COMMON) $(CFLAGS_RELEASE)
release: clean $(TARGET)

$(OBJDIR)/hook_%.o: hook.% | $(OBJDIR)
	objcopy \
	  --input binary \
	  --output elf64-x86-64 \
	  --binary-architecture i386:x86-64 \
	  $< $@

# Link
$(TARGET): $(OBJS) $(HOOKS_OBJ)
	$(CC) $(OBJS) $(HOOKS_OBJ) -o $@

# Compile
$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Create obj dir
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Test target
test: $(TEST_OBJS) $(HOOKS_OBJ)
	$(CC) $(TEST_OBJS) $(HOOKS_OBJ) -o test_runner
	./test_runner; rm -f test_runner

$(OBJDIR)/test_%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -DTESTING -MMD -MP -c $< -o $@

# Clean
clean:
	rm -rf $(OBJDIR) $(TARGET) test_runner

-include $(OBJS:.o=.d)

.PHONY: all clean release test