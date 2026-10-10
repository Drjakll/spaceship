# PRD — behavior-preserving Spaceship refactor

Status: Approved
Approval evidence: The user selected "Approve plan and characterization tests"
on October 10, 2026.
Documentation follow-up: the user requested method docstrings on the same date.
Authoritative backlog: section 5 of this document.

## 1. Problem and users

The game mixes implementations, global definitions, data capture, rendering,
and simulation in headers plus a large entry point. The project owner needs
clear responsibilities and extension points without changing game behavior.

## 2. Goals and scope

- Separate declarations from implementations in self-contained C modules.
- Give game state, assets, entities, collections, collisions, frame sequencing,
  and snapshot serialization clear ownership and interfaces.
- Reduce duplication, clarify names, use two-space indentation, document the
  build, add regression coverage, then launch the refactored game.
- Preserve gameplay, random-call order, frame order, constants, visuals,
  snapshot output, and disabled functionality.
- Document production functions using Doxygen comments, including parameters,
  return values, ownership, preconditions, and relevant side effects.
- Exclude new features, gameplay corrections, AI/control changes, data-format
  changes, asset changes, dependency upgrades, and Git initialization.

## 3. Requirements and constraints

This is an existing C/raylib project on macOS with no Git repository, build
instructions, or test source. Keep existing images, fonts, snapshots, and
prebuilt executables intact. Build the refactored executable separately.

The standard compiler requires Xcode license acceptance; its installed
compiler binary works directly. Discover a usable local raylib installation
before building. Record the actual dependency used for baseline and refactor.

Characterization tests must compare original and refactored behavior under
deterministic input. Passing baseline tests are expected for a pure refactor;
do not manufacture a failing functional test or introduce a behavior change.
Implementation source was inspected during scoping; disclose this in evidence.

The documentation follow-up changes comments only in C files and headers.
Keep public contracts in headers; implementations reference those contracts.
Document private helpers directly. Verify that C tokens remain unchanged.

## 4. Acceptance criteria and validation

| ID | Acceptance criterion | Verification |
| --- | --- | --- |
| AC-001 | Original gameplay results remain equivalent | Deterministic tests and baseline/refactor trace comparison |
| AC-002 | Modules have explicit, self-contained interfaces | Compile headers independently; review module dependencies |
| AC-003 | Duplicated factories and serialization are consolidated | Regression tests plus source review |
| AC-004 | Startup, frame order, rendering, controls remain equivalent | Headless full-loop trace plus interactive launch |
| AC-005 | Existing assets and snapshots remain byte-identical | Before/after file hashes |
| AC-006 | The refactored game builds and runs for user verification | Fresh native build plus launched window |
| AC-007 | All production functions have complete documentation | Documentation coverage; Clang documentation warnings |
| AC-008 | Docstrings preserve executable C tokens | Lexical comparison with the follow-up baseline |

Tests/build commands: establish a small native C test runner and Makefile.
Static analysis: installed Clang warnings and analyzer when available.
Dependency audit: inspect the actual linked local raylib version; no existing
dependency audit configuration is present.

## 5. Sprint backlog

Sprint: readability, maintainability, extensibility.
Review base: original files preserved under `.work/baseline`; Git unavailable
because this existing project is not a repository.
Review scope: `init.c`, `Headers/`, new C modules, build/test documentation.

- [x] T001 | P1 | depends: none | AC-001, AC-002 | Extract resource ownership into C modules; done when the baseline trace remains equivalent.
- [x] T002 | P1 | depends: T001 | AC-001, AC-003 | Refactor simulation entities into explicit modules; done when deterministic gameplay tests pass.
- [x] T003 | P1 | depends: T002 | AC-001, AC-003 | Refactor snapshot serialization into one documented module; done when serialized fixtures match the baseline byte for byte.
- [x] T004 | P1 | depends: T003 | AC-004, AC-005, AC-006 | Extract the game lifecycle from the entry point; done when the fresh build launches after full regression validation.
- [x] T005 | P1 | depends: T004 | AC-007, AC-008 | Document production functions; done when function-documentation validation passes.

T005 review base: `.work/docstrings-baseline/`, captured before comment edits.
Approval evidence: direct user request to write docstrings for the methods.
Coverage validation initially exited 1 with the actual summary:
`Documentation coverage: 97 function sites, 97 missing/incomplete comments.`
Implementation source was already visible from the preceding refactor.
Final coverage includes pointer-returning functions: 55 distinct functions at
99 declaration/definition sites, with zero missing/incomplete comments.
T005 evidence: documentation validation exited 0; lexical comparison of all
18 production C/header files found zero executable-token or style changes.
Fresh tests, standalone headers, sanitizers, and native compilation exited 0
with Clang documentation warnings treated as errors. All 3600 frame hashes
and serialized fixtures still match. Static analysis retains the same four
serializer allocation-failure diagnostics; dependency configuration is unchanged.
Commit unavailable: this existing project still has no Git repository.

T001 evidence: `make test all` exited 0; all 3600 frame hashes and
characterization fixtures match the saved original. The original-source
compiler warnings persist in modules scheduled for later tasks.

T002 evidence: `make test all` exited 0 with matching fixtures and all
3600 frame hashes; Clang analysis of assets, entities, and collections
exited 0 without diagnostics. Remaining warnings concern later tasks.

T003 evidence: `make test all` exited 0; empty/full snapshot fixtures
match byte for byte; all 3600 frame hashes match. Clang identified
realloc-failure leak paths inherited from the original serializer; these
remain a documented limitation rather than a behavior correction.

T004 evidence: native compilation with `-Werror` exited 0; all fixtures,
3600 frame hashes, standalone headers, and ASan/UBSan checks passed.
All original asset/data/binary hashes match. The real raylib game opened
its 1000x1000 window with successful OpenGL, texture, and font initialization.
The first sandboxed launch could not access desktop services; it was stopped,
then the authorized interactive launch succeeded with desktop access.

Record actual test evidence next to each task. Commits are unavailable because
this existing project has no Git repository; do not initialize one for a refactor.

## 6. Open questions

none. The user approved the module plan and the narrow DEV exception to use
passing original-code characterization tests followed by regression checks.
