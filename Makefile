# Compiler
CC = gcc

# Common flags
CFLAGS_COMMON = -std=c23 -Wall -Wextra -Wpedantic -Werror

# Debug / Release flags
CFLAGS_DEBUG = -ggdb -O0
CFLAGS_RELEASE = -O3 -march=native -mtune=native -DNDEBUG

# Default to debug
CFLAGS = $(CFLAGS_COMMON) $(CFLAGS_DEBUG)

# Directories
OBJDIR = obj

# Target executable
TARGET = zenv

# Source and object files
SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))

# Default target
all: $(TARGET)

# Release target (override flags)
release: CFLAGS = $(CFLAGS_COMMON) $(CFLAGS_RELEASE)
release: clean $(TARGET)

$(OBJDIR)/hook_zsh.o: hook.zsh
	objcopy \
	  --input binary \
	  --output elf64-x86-64 \
	  --binary-architecture i386:x86-64 \
	  hook.zsh $(OBJDIR)/hook_zsh.o

# Link
$(TARGET): $(OBJS) $(OBJDIR)/hook_zsh.o
	$(CC) $(OBJS) $(OBJDIR)/hook_zsh.o -o $@

# Compile
$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Create obj dir
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Clean
clean:
	rm -rf $(OBJDIR) $(TARGET)

-include $(OBJS:.o=.d)

.PHONY: all clean release