CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror
CPPFLAGS += -I../../src
LDLIBS += -lm -lpthread

CJSON_LIBS := $(shell pkg-config --libs libcjson 2>/dev/null)
ifneq ($(CJSON_LIBS),)
CPPFLAGS += -DHAVE_CJSON $(shell pkg-config --cflags libcjson)
LDLIBS += $(CJSON_LIBS)
endif
RAYLIB_CFLAGS := $(shell pkg-config --cflags raylib 2>/dev/null)
RAYLIB_LIBS := $(shell pkg-config --libs raylib 2>/dev/null)
ifeq ($(shell uname),Darwin)
RAYLIB_LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -framework CoreFoundation
endif

BUILD := build
CORE_SRC := $(filter-out ../../src/main.c ../../src/viewer.c,$(wildcard ../../src/*.c))
COMMON := enemy_ppo.c $(CORE_SRC)
HEADERS := $(wildcard *.h) $(wildcard ../../src/*.h)

.PHONY: all viewer test clean

all: $(BUILD)/enemy_ppo

viewer: $(BUILD)/enemy_ppo_viewer

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/enemy_ppo: main.c $(COMMON) $(HEADERS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) main.c $(COMMON) -o $@ $(LDLIBS)

$(BUILD)/enemy_ppo_viewer: viewer.c $(COMMON) $(HEADERS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(RAYLIB_CFLAGS) $(CFLAGS) viewer.c $(COMMON) -o $@ $(LDLIBS) $(RAYLIB_LIBS)

$(BUILD)/test_enemy_ppo: test_enemy_ppo.c $(COMMON) $(HEADERS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) test_enemy_ppo.c $(COMMON) -o $@ $(LDLIBS)

test: $(BUILD)/test_enemy_ppo
	./$(BUILD)/test_enemy_ppo

clean:
	rm -rf $(BUILD)
