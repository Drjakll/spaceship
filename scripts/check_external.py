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
    build = PROJECT / "build"
    build.mkdir(exist_ok=True)
    target = build / "test_puffercpu"
    subprocess.run([args.cc, "-std=c11", "-O2", "-I" + str(root / "src"),
                    '-DPUFFERCPU_SOURCE="' + str(root / "src/puffercpu.c") + '"',
                    str(PROJECT / "tests/test_puffercpu.c"), "-lm", "-o", str(target)], check=True)
    subprocess.run([str(target)], check=True)


if __name__ == "__main__":
    main()
