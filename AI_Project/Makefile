CC = cc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Wpedantic
RAYFLAGS := $(shell pkg-config --cflags --libs raylib)

all: build/rl_spaceship

build/rl_spaceship: src/main.c
	mkdir -p build
	$(CC) $(CFLAGS) $< -o $@ $(RAYFLAGS) -lm

train: all
	./build/rl_spaceship --train 10000

play: all
	./build/rl_spaceship

test: all
	./build/rl_spaceship --test

.PHONY: all train play test
