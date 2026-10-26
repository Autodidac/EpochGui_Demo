"""Bundled dependency identity/byte checks; no network or model work."""
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("demo_dependencies", ROOT / "tools/prepare_dependencies.py")
dependencies = importlib.util.module_from_spec(spec)
spec.loader.exec_module(dependencies)


class DependencyBundle(unittest.TestCase):
    def test_exact_inventory_rejects_changed_missing_or_unlisted_source(self):
        parent = ROOT / "build/contracts"
        parent.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=parent, prefix="dependency-") as folder:
            root = Path(folder)
            source = root / "source.cpp"
            source.write_bytes(b"public source")
            record = {"schema": "epoch.demo-dependency-source.v1", "revision": "a" * 40,
                      "files": [{"path": source.name, "sha256": hashlib.sha256(source.read_bytes()).hexdigest()}]}
            receipt = root / dependencies.RECEIPT
            receipt.write_text(json.dumps(record))
            dependencies.verify_bundle(root, "a" * 40)
            with self.assertRaises(ValueError): dependencies.verify_bundle(root, "b" * 40)
            source.write_bytes(b"different bytes")
            with self.assertRaises(ValueError): dependencies.verify_bundle(root, "a" * 40)
            source.write_bytes(b"public source")
            extra = root / "unexpected.cpp"
            extra.write_bytes(b"not in manifest")
            with self.assertRaises(ValueError): dependencies.verify_bundle(root, "a" * 40)
            extra.unlink()
            source.unlink()
            with self.assertRaises(ValueError): dependencies.verify_bundle(root, "a" * 40)

    def test_receipt_cannot_reference_parent_or_repeat_file(self):
        parent = ROOT / "build/contracts"
        parent.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=parent, prefix="dependency-") as folder:
            root = Path(folder)
            outside = root.parent / (root.name + "-outside.cpp")
            outside.write_bytes(b"outside")
            try:
                record = {"schema": "epoch.demo-dependency-source.v1", "revision": "a" * 40,
                          "files": [{"path": "../" + outside.name,
                                     "sha256": hashlib.sha256(outside.read_bytes()).hexdigest()}]}
                (root / dependencies.RECEIPT).write_text(json.dumps(record))
                with self.assertRaises(ValueError): dependencies.verify_bundle(root, "a" * 40)
            finally:
                outside.unlink()


if __name__ == "__main__": unittest.main()
