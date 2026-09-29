"""Compile and run the real external PufferLib CPU implementation."""
import argparse
import subprocess
from prepare_pufferlib import PROJECT, verify


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pufferlib-root", required=True)
    parser.add_argument("--cc", default="clang")
    args = parser.parse_args()
    root = verify(args.pufferlib_root)
    subprocess.run(["patch", "--dry-run", "-p1", "-i", str(PROJECT / "patches/pufferl-spaceship.patch")],
                   cwd=root, check=True)
    build = PROJECT / "build"
    build.mkdir(exist_ok=True)
    target = build / "test_puffercpu"
    subprocess.run([args.cc, "-std=c11", "-O2", "-I" + str(root / "src"),
                    '-DPUFFERCPU_SOURCE="' + str(root / "src/puffercpu.c") + '"',
                    str(PROJECT / "tests/test_puffercpu.c"), "-lm", "-o", str(target)], check=True)
    subprocess.run([str(target)], check=True)
    checkpoint = build / "checkpoint"
    subprocess.run([args.cc, "-std=c11", "-O2", "-I" + str(PROJECT / "native"),
                    '-DPUFFERCPU_SOURCE="' + str(root / "src/puffercpu.c") + '"',
                    str(PROJECT / "native/checkpoint.c"), str(PROJECT / "native/spaceship_core.c"),
                    "-lm", "-o", str(checkpoint)], check=True)
    subprocess.run(["python3", str(PROJECT / "tests/check_checkpoint.py"), str(checkpoint)], check=True)
    target = build / "test_adapter"
    subprocess.run([args.cc, "-std=c11", "-O2", "-I" + str(PROJECT / "native"),
                    "-I" + str(PROJECT / "tests/stubs"), "-I" + str(root / "src"),
                    str(PROJECT / "tests/test_adapter.c"), str(PROJECT / "native/spaceship_core.c"),
                    "-lm", "-o", str(target)], check=True)
    subprocess.run([str(target)], check=True)


if __name__ == "__main__":
    main()
