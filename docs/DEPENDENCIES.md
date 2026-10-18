# External dependencies

PufferLib source, archives, binaries, and third-party libraries remain outside this repository. `pufferlib-lock.json` contains identities and hashes only. The initial source is PufferLib's 5.0 revision `6ffa5b10dbbbe4d1e8288367c7d9d3acd3bad4a2`, matching the reference project's pinned revision. Its external `LICENSE` is MIT. Spaceship setup verifies the archive SHA-256 and native source files; it never installs system packages.

Prepare an external directory, or verify a pre-existing checkout of the same unmodified native source:

```sh
python3 scripts/prepare_pufferlib.py --destination /absolute/external/path/pufferlib-5.0
python3 scripts/prepare_pufferlib.py --verify /absolute/external/path/pufferlib-5.0
python3 scripts/check_external.py --pufferlib-root /absolute/external/path/pufferlib-5.0
```

`--archive /path/to/pinned.tar.gz` reuses an existing archive. Existing preparation destinations are never overwritten. Destinations inside the Spaceship repository are rejected, including resolved symlink paths. The CPU smoke compiles and executes the actual external `src/puffercpu.c` implementation; it is separate from the dependency-free core suite.

Development used `/private/tmp/spaceship-deps/pufferlib-6ffa5b1`. This temporary cache can be recreated with the setup command. No change was made to `~/Developer/puffer-selfplay-modal` or its generated backend.

Dependency review, 2026-09-28: pinned archive/source integrity and MIT license were checked. A web search for PufferLib advisories did not identify a matching advisory; fetching GitHub's repository advisory page failed, so a comprehensive vulnerability scan is unavailable and no vulnerability-free claim is made. Native dependencies are not audited by a Python package scanner. Python tooling currently uses only the standard library.

[Upstream issue/PR 691](https://github.com/PufferAI/PufferLib/pull/691) reports a Muon parameter-alignment problem for some layouts. Its relevance to the selected Spaceship model shape must be checked before the later GPU run, together with rollout-boundary and reward-transport diagnostics. The report's results belong to its author; they are not Spaceship CUDA verification. Local CPU checks do not establish optimizer correctness or successful GPU training.

Chosen initial model: float32, 2554 inputs, hidden 64, two MinGRU layers, decoder 12 (9+2 action logits plus value). Inspection of the actual CPU/runtime constructors confirms these are bias-free matrices: 163456 encoder weights, 768 decoder weights, and two 12288-weight recurrent matrices, totaling 188800 floats (755200 bytes). All counts are divisible by eight, so both four-float native float32 alignment and eight-float CPU alignment introduce no gaps. The unmodified upstream CPU loader consumes the entire expected layout. This avoids the reported padded-layout condition in upstream PR 691; it does not substitute for the later GPU optimizer/checkpoint diagnostic. Training is initially constrained to float32 for transparent target comparisons.

## raylib viewer dependency

External official [raylib 5.5 macOS release](https://github.com/raysan5/raylib/releases/tag/5.5), matching the pinned PufferLib build's graphics version. Archive SHA-256: `930c67b676963c6cffbd965814664523081ecbf3d30fc9df4211d0064aa6ba39`. `scripts/prepare_raylib.py` checks that archive and extracts only its header, static library and zlib license into an external destination. Both arm64 and x86_64 are present in the official static archive. No raylib source/archive/binary is tracked here.

Advisory search on 2026-09-28 surfaced [CVE-2025-15533](https://nvd.nist.gov/vuln/detail/CVE-2025-15533) and upstream [font-atlas issue 5433](https://github.com/raysan5/raylib/issues/5433). The upstream issue describes a master-branch font-atlas overflow; applicability to this prebuilt 5.5 archive was not established. This viewer uses only the repository's existing local font and images and does not accept arbitrary font input. The prebuilt third-party library itself was not instrumented or comprehensively audited. Treat this as a recorded scan limitation, not a clean vulnerability bill of health.
