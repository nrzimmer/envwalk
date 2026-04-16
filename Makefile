# Compiler
CC = gcc

# Common flags
CFLAGS_COMMON = -std=c23 -Wall -Wextra -Wpedantic -Werror -I$(THIRDPARTY)

# Debug / Release flags
CFLAGS_DEBUG = -ggdb -O0 -rdynamic -fno-omit-frame-pointer
CFLAGS_RELEASE = -O3 -march=native -mtune=native -DNDEBUG

# Default to debug
CFLAGS = $(CFLAGS_COMMON) $(CFLAGS_DEBUG)

# Directories
OBJDIR     = obj
SRCDIR     = src
HOOKSDIR   = $(SRCDIR)/hooks
THIRDPARTY = $(SRCDIR)/third-party

# Target executable
TARGET = envwalk

# Source and object files
SRCS = $(filter-out $(SRCDIR)/test.c, $(wildcard $(SRCDIR)/*.c))
OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

# Test sources (all except envwalk.c which owns main + NOB_IMPLEMENTATION)
TEST_SRCS = $(SRCDIR)/test.c $(filter-out $(SRCDIR)/envwalk.c, $(SRCS))
TEST_OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/test_%.o,$(TEST_SRCS))
HOOKS = $(wildcard $(HOOKSDIR)/hook.*)
HOOKS_OBJ = $(patsubst $(HOOKSDIR)/hook.%,$(OBJDIR)/hook_%.o,$(HOOKS))

# Default target
all: $(TARGET)

# Release target (override flags)
release:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS_COMMON) $(CFLAGS_RELEASE)" $(TARGET)

$(OBJDIR)/hook_%.o: $(HOOKSDIR)/hook.% | $(OBJDIR)
	cd $(HOOKSDIR) && objcopy \
	  --input binary \
	  --output elf64-x86-64 \
	  --binary-architecture i386:x86-64 \
	  hook.$* $(CURDIR)/$(OBJDIR)/hook_$*.o

# Link
$(TARGET): $(OBJS) $(HOOKS_OBJ)
	$(CC) $(OBJS) $(HOOKS_OBJ) -o $@

# Compile
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Create obj dir
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Test target
test: $(TEST_OBJS) $(HOOKS_OBJ)
	$(CC) $(TEST_OBJS) $(HOOKS_OBJ) -o test_runner
	./test_runner; rm -f test_runner

$(OBJDIR)/test_%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -DTESTING -MMD -MP -c $< -o $@

# Clean
clean:
	rm -rf $(OBJDIR) $(TARGET) test_runner

-include $(OBJS:.o=.d)

ARCH_DIR = packaging/arch
ARCH_STAGE = \
	src/cli.c src/cli.h \
	src/config.c src/config.h \
	src/dotenv.c src/dotenv.h \
	src/path.c src/path.h \
	src/types.c src/types.h \
	src/envwalk.c \
	src/stack_trace.c src/stack_trace.h \
	src/test.c \
	src/third-party/nob.h \
	Makefile \
	src/hooks/hook.zsh \
	src/hooks/hook.bash

arch:
	cp $(ARCH_STAGE) $(ARCH_DIR)/
	cd $(ARCH_DIR) && makepkg -f
	cd $(ARCH_DIR) && rm -f $(notdir $(ARCH_STAGE))

ubuntu:
	ln -sfT packaging/ubuntu debian
	dpkg-buildpackage -us -uc -b
	rm -f debian

package: arch ubuntu

.PHONY: all clean release test arch ubuntu package