"""External-dependency setup must never vendor PufferLib into the project."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

PROJECT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("prepare_pufferlib", PROJECT / "scripts/prepare_pufferlib.py")
setup = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(setup)


class ExternalSetupTests(unittest.TestCase):
    def test_rejects_project_destination_before_io(self):
        with self.assertRaisesRegex(ValueError, "outside the Spaceship repository"):
            setup.prepare(PROJECT / "build/pufferlib", Path("does-not-exist.tar.gz"))

    def test_rejects_modified_archive(self):
        with tempfile.TemporaryDirectory() as tmp:
            archive = Path(tmp) / "wrong.tar.gz"
            archive.write_bytes(b"not the pinned dependency")
            destination = Path(tmp) / "dependency"
            with self.assertRaisesRegex(ValueError, "checksum"):
                setup.prepare(destination, archive)
            self.assertFalse(destination.exists())

    def test_rejects_unverified_existing_directory(self):
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaisesRegex(ValueError, "already exists"):
                setup.prepare(Path(tmp), Path("does-not-exist.tar.gz"))


if __name__ == "__main__":
    unittest.main()
