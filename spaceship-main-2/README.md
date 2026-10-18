# Spaceship

A C/raylib game with three autonomous spaceships. This refactor keeps the
original game rules, assets, random controls, scoring, and drawing order.

## Build and run

Run commands from this project root so relative image/font paths resolve:

```sh
make
make run
```

The fresh executable is `.work/build/spaceship`. Close its window or press
Escape to exit. The original bundled binaries are preserved.

This Mac currently has an unaccepted Xcode license and no raylib installation
at the path expected by the bundled executable. `.work/local.mk` selects the
installed Clang binary directly, its macOS SDK, and the existing raylib 5.5
static library under `~/Developer/self-play-pong`. Nothing is installed or
upgraded. On this Mac, invoke Make directly if `/usr/bin/make` is blocked:

```sh
/Applications/Xcode.app/Contents/Developer/usr/bin/make run
```

For another macOS toolchain, set `CC`, `SDK_FLAGS`, and `RAYLIB_ROOT` in a local
`.work/local.mk`, or pass them as Make variables. The Makefile uses the static
raylib archive and macOS frameworks. A Windows/Linux build configuration is
outside this refactor's scope.

## Code organization

`Headers/` contains self-contained interfaces; `Sources/` contains definitions.
`init.c` delegates to `game_run()`.

| Module | Responsibility |
| --- | --- |
| `game.h` / `game.c` | Lifecycle, controls, frame order, HUD |
| `state.c` | Entity ownership, insertion, projectile IDs, cleanup |
| `assets.h` / `assets.c` | Image/texture loading and unloading |
| `objects.h` / `objects.c` | Entity types, factories, motion callbacks |
| `object_collections.h` / `.c` | Owned linked lists and visitor traversal |
| `detections.h` / `.c` | Collision rules and movement/drawing visitors |
| `data.h` / `data.c` | Snapshot values, collection, serialization |
| `macros.h` | Existing game settings and action flags |
| `types.h` | Shared forward declarations |
| `self_control.h`, `ml_utilities.h` | Existing inactive control/ML scaffolding |

`Game_State` owns mutable game state. Factories receive textures explicitly;
visitors receive a caller-supplied context. Collision handling uses a local
projectile context, so it does not depend on global scratch variables.
Enemy tuning lives in one definition table. Motion and weapon callbacks remain
extension points. Serializers return strings owned by their callers.

The frame sequence is deliberately preserved: update spaceship controls, spawn
an enemy when due, draw ships, resolve projectile hits, move/draw enemies,
decrement enemy attack cooldowns, move/draw projectiles, then draw the HUD.
Collection traversal retains the original promoted-head deferral. Snapshot
recording, keyboard controls, contact-driven game-over checks, and the time
limit were disabled in the original; their runtime behavior is preserved.

## Verification

```sh
make test
make headers
make analyze
make sanitizers
```

`tests/fixtures/` holds results captured from the original source before edits.
The characterization suite checks all 32 control combinations, normalization,
boundaries, enemy variants, trajectories, cooldowns, collision scoring,
collection traversal, and empty/single/multiple snapshot serialization.
A deterministic raylib boundary compares every frame hash across 3,600 frames,
including random calls, assets, textures, draw calls, HUD text, and shutdown.
The native interactive build uses real raylib rather than that test boundary.

`make baseline` recaptures the original results into `.work/baseline-results/`
using the preserved local `.work/baseline/` source. It does not overwrite the
saved fixture files. Those baseline copies are local review artifacts;
normal regression tests only require the fixture files.

The analyzer still reports original serializer allocation-failure leak paths.
Original serialization capacity/truncation and escaping limitations are also
preserved. These are documented in `WALKTHROUGH.md`. There is no dependency
vulnerability audit configured in this project.
