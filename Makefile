# Compiler
CC = gcc

# Detect supported C standard flag
CSTD := $(shell echo 'int main(void){return 0;}' > .conftest.c && \
	$(CC) -std=c23 .conftest.c -o /dev/null >/dev/null 2>&1 && \
	rm -f .conftest.c && echo c23 || (rm -f .conftest.c && echo c2x))

# Common flags
CFLAGS_COMMON = -std=$(CSTD) -Wall -Wextra -Wpedantic -Werror -I$(THIRDPARTY)

# GCC 13 fails if fread result is ignored
CFLAGS_COMMON += -Wno-error=unused-result

# Debug / Release flags
CFLAGS_DEBUG = -ggdb -O0 -rdynamic -fno-omit-frame-pointer -no-pie
CFLAGS_RELEASE = -O3 -march=native -mtune=native -DNDEBUG

# Default to debug
CFLAGS = $(CFLAGS_COMMON) $(CFLAGS_DEBUG)

# Directories
OBJDIR     = obj
SRCDIR     = src
TESTSDIR   = $(SRCDIR)/tests
HOOKSDIR   = $(SRCDIR)/hooks
THIRDPARTY = $(SRCDIR)/third-party

# Target executable
TARGET = envwalk

# Source and object files (excludes test.c if present for compatibility)
SRCS = $(filter-out $(SRCDIR)/test.c, $(wildcard $(SRCDIR)/*.c))
OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

# Test sources: src/tests/*.c + all src/*.c except envwalk.c
TEST_MOD_SRCS = $(filter-out $(SRCDIR)/envwalk.c, $(SRCS))
TEST_MOD_OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/test_%.o,$(TEST_MOD_SRCS))
TEST_SUITE_SRCS = $(wildcard $(TESTSDIR)/*.c)
TEST_SUITE_OBJS = $(patsubst $(TESTSDIR)/%.c,$(OBJDIR)/tests_%.o,$(TEST_SUITE_SRCS))
TEST_OBJS = $(TEST_MOD_OBJS) $(TEST_SUITE_OBJS)
HOOKS = $(wildcard $(HOOKSDIR)/hook.*)
HOOKS_OBJ = $(patsubst $(HOOKSDIR)/hook.%,$(OBJDIR)/hook_%.o,$(HOOKS))

# Default target
all: $(TARGET)

# Release target
release:
	$(MAKE) clean
	$(MAKE) $(TARGET) CFLAGS="$(CFLAGS_COMMON) $(CFLAGS_RELEASE)"

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

# Test target — runs the suite under AddressSanitizer + LeakSanitizer + UBSan.
# Sources are compiled directly (no shared objs) so the sanitizers instrument
# everything. The envwalk binary is built first so integration tests can spawn it.
CFLAGS_ASAN = -fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer
test: $(TARGET) $(HOOKS_OBJ)
	$(CC) $(CFLAGS_COMMON) $(CFLAGS_DEBUG) $(CFLAGS_ASAN) -DTESTING \
	  -I$(SRCDIR) $(TEST_MOD_SRCS) $(TEST_SUITE_SRCS) $(HOOKS_OBJ) -o test_runner
	ENVWALK_BIN=$(CURDIR)/$(TARGET) ASAN_OPTIONS=detect_leaks=1 \
	  UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
	  ./test_runner; status=$$?; rm -f test_runner; exit $$status

$(OBJDIR)/test_%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -DTESTING -MMD -MP -c $< -o $@

$(OBJDIR)/tests_%.o: $(TESTSDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -DTESTING -I$(SRCDIR) -I$(THIRDPARTY) -MMD -MP -c $< -o $@

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
	src/string_utils.c src/string_utils.h \
	src/envwalk.c \
	src/stack_trace.c src/stack_trace.h \
	src/third-party/nob.h \
	Makefile \
	src/hooks/hook.zsh \
	src/hooks/hook.bash

arch:
	cp $(ARCH_STAGE) $(ARCH_DIR)/
	cd $(ARCH_DIR); \
	makepkg $(if $(INSTALL),-si,-f); status=$$?; \
	rm -f $(notdir $(ARCH_STAGE)); \
	exit $$status

arch-install: INSTALL=1
arch-install: arch

ubuntu:
	ln -sfT packaging/ubuntu debian
	dpkg-buildpackage -us -uc -b
	rm -f debian

package: arch ubuntu

.PHONY: all clean release test arch ubuntu package
