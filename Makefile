CC := clang
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -O2
CPPFLAGS := -Inative
LDLIBS := -lm

.PHONY: test sanitize analyze
build/test_core: tests/test_core.c native/spaceship_core.c native/spaceship_core.h
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_core.c native/spaceship_core.c $(LDLIBS) -o $@
test: build/test_core
	./build/test_core
sanitize:
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer tests/test_core.c native/spaceship_core.c $(LDLIBS) -o build/test_sanitize
	./build/test_sanitize
analyze:
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) --analyze native/spaceship_core.c -o build/core.plist
