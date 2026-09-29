CC := clang
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -O2
CPPFLAGS := -Inative
LDLIBS := -lm

.PHONY: test sanitize analyze
build/test_core: tests/test_core.c native/spaceship_core.c native/spaceship_core.h
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_core.c native/spaceship_core.c $(LDLIBS) -o $@
build/test_ppo: tests/test_ppo.c native/spaceship_ppo.h
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_ppo.c $(LDLIBS) -o $@

build/test_bots: tests/test_bots.c native/spaceship_core.c native/spaceship_bots.c native/spaceship_bots.h
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_bots.c native/spaceship_core.c native/spaceship_bots.c $(LDLIBS) -o $@
build/evaluate: native/evaluate.c native/spaceship_core.c native/spaceship_bots.c native/spaceship_core.h native/spaceship_eval.h
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) native/evaluate.c native/spaceship_core.c native/spaceship_bots.c $(LDLIBS) -o $@
test: build/test_core build/test_ppo build/test_bots build/evaluate
	./build/test_core
	./build/test_ppo
	./build/test_bots
	python3 -m unittest discover -s tests -p 'test_*.py'
sanitize:
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer tests/test_core.c native/spaceship_core.c $(LDLIBS) -o build/test_sanitize
	./build/test_sanitize
analyze:
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) --analyze native/spaceship_core.c -o build/core.plist
	$(CC) $(CPPFLAGS) $(CFLAGS) --analyze native/spaceship_bots.c -o build/bots.plist
