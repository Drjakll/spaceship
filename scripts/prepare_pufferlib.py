"""Prepare a checksum-verified PufferLib checkout outside this repository."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import tarfile
import tempfile
import urllib.request

PROJECT = Path(__file__).resolve().parents[1]
LOCK = json.loads((PROJECT / "pufferlib-lock.json").read_text())


def digest(path):
    result = hashlib.sha256()
    with Path(path).open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            result.update(chunk)
    return result.hexdigest()


def external_path(path):
    result = Path(path).expanduser().resolve()
    if result == PROJECT or PROJECT in result.parents:
        raise ValueError("PufferLib must remain outside the Spaceship repository")
    return result


def verify(root):
    root = external_path(root)
    for name, expected in LOCK["source_files"].items():
        if not (root / name).is_file() or digest(root / name) != expected:
            raise ValueError("Pinned source checksum mismatch: " + name)
    return root


def prepare(destination, archive=None):
    destination = external_path(destination)
    if destination.exists():
        raise ValueError("Destination already exists; use --verify for an existing checkout")
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".spaceship-prepare-", dir=destination.parent) as temporary:
        temporary = Path(temporary)
        if archive is None:
            archive = temporary / "pufferlib.tar.gz"
            with urllib.request.urlopen(LOCK["archive_url"], timeout=90) as response, archive.open("wb") as output:
                shutil.copyfileobj(response, output)
        archive = Path(archive)
        if digest(archive) != LOCK["archive_sha256"]:
            raise ValueError("PufferLib archive checksum mismatch")
        with tarfile.open(archive) as source:
            for member in source.getmembers():
                output = (temporary / member.name).resolve()
                if temporary not in output.parents or not (member.isdir() or member.isfile()):
                    raise ValueError("Unsupported archive member: " + member.name)
            source.extractall(temporary)
        extracted = temporary / ("PufferLib-" + LOCK["commit"])
        verify(extracted)
        extracted.rename(destination)
    return destination


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--destination", type=Path)
    group.add_argument("--verify", type=Path)
    parser.add_argument("--archive", type=Path, help="Reuse the exact pinned archive without downloading")
    args = parser.parse_args()
    try:
        root = verify(args.verify) if args.verify else prepare(args.destination, args.archive)
    except (ValueError, OSError) as error:
        parser.exit(1, str(error) + "\n")
    print(json.dumps({"pufferlib_root": str(root), "commit": LOCK["commit"], "verified": True}))


if __name__ == "__main__":
    main()
