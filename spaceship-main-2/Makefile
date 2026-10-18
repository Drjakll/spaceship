# Inline recipes follow the project's two-space indentation policy.
CC = cc
RAYLIB_ROOT ?= /opt/homebrew/opt/raylib
SDK_FLAGS ?=

# Local toolchain paths belong here, rather than in game source.
-include .work/local.mk

export TMPDIR := $(CURDIR)/.work/tmp
CPPFLAGS += -IHeaders -I$(RAYLIB_ROOT)/include
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic
SOURCES := $(wildcard Sources/*.c)
HEADERS := $(wildcard Headers/*.h)
BUILD := .work/build
RAYLIB_LIB := $(RAYLIB_ROOT)/lib/libraylib.a
FRAMEWORKS := -framework Cocoa -framework IOKit -framework OpenGL \
              -framework CoreVideo -framework CoreAudio \
              -framework AudioToolbox

.PHONY: all run test baseline headers analyze sanitizers clean

all: $(BUILD)/spaceship

$(BUILD) .work/tmp .work/evidence .work/baseline-results .work/analysis: ; \
  set -e; \
  mkdir -p $@

$(BUILD)/spaceship: init.c $(SOURCES) $(HEADERS) | $(BUILD) .work/tmp ; \
  set -e; \
  $(CC) $(CPPFLAGS) $(CFLAGS) $(SDK_FLAGS) init.c $(SOURCES) \
    $(RAYLIB_LIB) $(FRAMEWORKS) -o $@

run: all ; \
  set -e; \
  ./$(BUILD)/spaceship

$(BUILD)/characterization: tests/characterization.c tests/game_adapter.h \
                           tests/raylib_stub.c \
                           init.c $(SOURCES) $(HEADERS) | $(BUILD) .work/tmp ; \
  set -e; \
  $(CC) $(CPPFLAGS) $(CFLAGS) $(SDK_FLAGS) \
    tests/characterization.c tests/raylib_stub.c $(SOURCES) -o $@

$(BUILD)/frame-trace: init.c $(SOURCES) $(HEADERS) tests/raylib_stub.c \
                    | $(BUILD) .work/tmp ; \
  set -e; \
  $(CC) $(CPPFLAGS) $(CFLAGS) $(SDK_FLAGS) \
    init.c $(SOURCES) tests/raylib_stub.c -o $@

baseline: | $(BUILD) .work/tmp .work/baseline-results ; \
  set -e; \
  $(CC) $(CPPFLAGS) $(SDK_FLAGS) -std=c11 -DTEST_ORIGINAL \
    tests/characterization.c tests/raylib_stub.c \
    -o $(BUILD)/original-characterization; \
  $(CC) $(CPPFLAGS) $(SDK_FLAGS) -std=c11 \
    .work/baseline/init.c tests/raylib_stub.c -o $(BUILD)/original-trace; \
  $(BUILD)/original-characterization > .work/baseline-results/contracts.txt; \
  $(BUILD)/original-trace > .work/baseline-results/frames.txt; \
  echo "Original characterization assertions passed."; \
  echo "Captured 3600 original frame hashes."

test: $(BUILD)/characterization $(BUILD)/frame-trace \
      | .work/evidence ; \
  set -e; \
  $(BUILD)/characterization > .work/evidence/contracts.txt; \
  diff -u tests/fixtures/contracts.txt .work/evidence/contracts.txt; \
  $(BUILD)/frame-trace > .work/evidence/frames.txt; \
  diff -u tests/fixtures/frames.txt .work/evidence/frames.txt; \
  echo "Characterization assertions passed."; \
  echo "Snapshot output matches the original byte for byte."; \
  echo "All 3600 frame hashes match the original."

headers: | .work/tmp ; \
  set -e; \
  for header in $(HEADERS); do \
    echo "Checking $$header"; \
    printf '#include "%s"\ntypedef int Header_Check;\n' "$$header" | \
      $(CC) $(CPPFLAGS) $(CFLAGS) $(SDK_FLAGS) -Werror \
        -I. -x c -fsyntax-only - || exit 1; \
  done

analyze: | .work/analysis .work/tmp ; \
  set -e; \
  for source in init.c $(SOURCES); do \
    echo "Analyzing $$source"; \
    $(CC) $(CPPFLAGS) $(CFLAGS) $(SDK_FLAGS) --analyze \
      -Xanalyzer -analyzer-output=text $$source \
      -o .work/analysis/$$(basename $$source).plist || exit 1; \
  done

sanitizers: | $(BUILD) .work/tmp .work/evidence ; \
  set -e; \
  $(CC) $(CPPFLAGS) $(CFLAGS) $(SDK_FLAGS) \
    -fsanitize=address,undefined -fno-omit-frame-pointer -g \
    tests/characterization.c tests/raylib_stub.c $(SOURCES) \
    -o $(BUILD)/characterization-sanitized; \
  $(CC) $(CPPFLAGS) $(CFLAGS) $(SDK_FLAGS) \
    -fsanitize=address,undefined -fno-omit-frame-pointer -g \
    init.c tests/raylib_stub.c $(SOURCES) -o $(BUILD)/trace-sanitized; \
  $(BUILD)/characterization-sanitized \
    > .work/evidence/sanitized-contracts.txt; \
  diff -u tests/fixtures/contracts.txt .work/evidence/sanitized-contracts.txt; \
  $(BUILD)/trace-sanitized > .work/evidence/sanitized-frames.txt; \
  diff -u tests/fixtures/frames.txt .work/evidence/sanitized-frames.txt; \
  echo "Address/undefined-behavior sanitizer checks passed."

clean: ; \
  set -e; \
  rm -rf $(BUILD) .work/analysis
