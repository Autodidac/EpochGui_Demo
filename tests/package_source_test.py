"""Real local Git/source ZIP fixtures; no compilation, network or native GUI proof."""
import importlib.util
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("demo_package_source", ROOT / "tools/package_source.py")
packager = importlib.util.module_from_spec(spec)
spec.loader.exec_module(packager)
runtime_spec = importlib.util.spec_from_file_location("demo_package_runtime", ROOT / "tools/package_runtime.py")
runtime = importlib.util.module_from_spec(runtime_spec)
runtime_spec.loader.exec_module(runtime)
prepare_spec = importlib.util.spec_from_file_location("demo_prepare_dependencies", ROOT / "tools/prepare_dependencies.py")
prepare = importlib.util.module_from_spec(prepare_spec)
prepare_spec.loader.exec_module(prepare)


def git(root, *args):
    return subprocess.check_output(["git", "-C", str(root), *args], stderr=subprocess.STDOUT).decode().strip()


def repository(root, name, files):
    root.mkdir(parents=True)
    git(root, "init", "-q")
    git(root, "config", "user.name", "Source fixture")
    git(root, "config", "user.email", "fixture@example.invalid")
    git(root, "config", "core.autocrlf", "false")
    git(root, "remote", "add", "origin", f"https://github.com/Autodidac/{name}.git")
    for name, data in files.items():
        path = root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
    git(root, "add", "--all")
    git(root, "commit", "-qm", "Local source fixture")
    return git(root, "rev-parse", "HEAD")


class SourcePackage(unittest.TestCase):
    def test_clean_git_closure_roundtrip_and_fail_closed_boundaries(self):
        parent = ROOT / "build/contracts"
        parent.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=parent, prefix="source-package-") as temporary:
            base = Path(temporary)
            dependencies = base / "libraries"
            pins = {}
            for owner, project in packager.OWNERS.items():
                pins[owner] = repository(dependencies / project, project,
                    {"LICENSE": b"fixture license\n", "modules/public.ixx": b"export module fixture;\n"})
                prepare.prepare(dependencies, owner, pins[owner])
                self.assertEqual(git(dependencies / project, "status", "--porcelain"), "")
                self.assertFalse((dependencies / project / prepare.RECEIPT).exists())
            demo = base / "Demo"
            revision = repository(demo, "EpochGui_Demo", {
                "LICENSE": b"MIT fixture\n", "VERSION": b"0.7.0\n", ".gitignore": b"build/\n",
                "CMakeLists.txt": b"# source fixture, never built\n",
                "assets/epochgui_demo.ppm": b"P3\n1 1\n255\n0 0 0\n",
                "dependencies.json": json.dumps({"source_revisions": pins}).encode(),
                "tools/prepare_dependencies.py": (ROOT / "tools/prepare_dependencies.py").read_bytes()})
            artifact, manifest = packager.bundle(demo, dependencies, demo / "build/packages/accepted")
            self.assertEqual(manifest["demo_revision"], revision)
            self.assertEqual(set(manifest["dependency_revisions"]), set(packager.OWNERS.values()))
            with zipfile.ZipFile(artifact) as archive:
                names = archive.namelist()
                self.assertFalse(any("/.git/" in name for name in names))
                for project in packager.OWNERS.values():
                    self.assertTrue(any(name.endswith(f"dependencies/{project}/LICENSE") for name in names))
                    self.assertTrue(any(name.endswith(f"dependencies/{project}/epoch-demo-source.json") for name in names))
                self.assertTrue(any(name.endswith("THIRD_PARTY_NOTICES.md") for name in names))
            self.assertTrue(Path(str(artifact) + ".sha256").is_file())
            binary = demo / "build/offline/EpochGuiDemo.exe"
            binary.parent.mkdir(parents=True)
            binary.write_bytes(b"MZ" + b"fixture only, never executable")
            with patch.object(runtime, "ROOT", demo):
                candidate = runtime.package(binary, artifact, "windows-x64", demo / "build/runtime-candidate")
                with zipfile.ZipFile(candidate) as archive:
                    self.assertEqual(json.loads(archive.read("candidate.json"))["native_acceptance"], "not_run")
                    self.assertNotIn("VALIDATION.json", archive.namelist())
                receipt = {"schema": "epoch.demo-native-acceptance.v1", "platform": "windows-x64",
                           "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
                           "source_archive_sha256": hashlib.sha256(artifact.read_bytes()).hexdigest(),
                           "checks": {name: "pass" for name in runtime.NATIVE_CHECKS},
                           "private_log": "fixture must not be copied to the public artifact"}
                owner_receipt = demo / "build/native-fixture.json"
                owner_receipt.write_text(json.dumps(receipt))
                final = runtime.package(binary, artifact, "windows-x64", demo / "build/runtime-verified-fixture", owner_receipt)
                with zipfile.ZipFile(final) as archive:
                    validation = json.loads(archive.read("VALIDATION.json"))
                    self.assertEqual(validation["native_acceptance"], "owner_verified")
                    self.assertNotIn("private_log", validation)
                    self.assertNotIn("candidate.json", archive.namelist())
                    self.assertIn(b"owner-verified", archive.read("README.txt"))
                receipt["binary_sha256"] = "0" * 64
                owner_receipt.write_text(json.dumps(receipt))
                with self.assertRaisesRegex(ValueError, "exact binary"):
                    runtime.package(binary, artifact, "windows-x64", demo / "build/wrong-native", owner_receipt)
                receipt["binary_sha256"] = hashlib.sha256(binary.read_bytes()).hexdigest()
                del receipt["checks"]["multiline_clip"]
                owner_receipt.write_text(json.dumps(receipt))
                with self.assertRaisesRegex(ValueError, "required checks"):
                    runtime.package(binary, artifact, "windows-x64", demo / "build/incomplete-native", owner_receipt)
            with self.assertRaises(FileExistsError):
                packager.bundle(demo, dependencies, demo / "build/packages/accepted")
            with self.assertRaisesRegex(ValueError, "under the demo's build"):
                packager.bundle(demo, dependencies, base / "escape")
            extra = demo / "uncommitted.cpp"
            extra.write_text("uncommitted")
            with self.assertRaisesRegex(ValueError, "clean checkout"):
                packager.bundle(demo, dependencies, demo / "build/packages/dirty")
            extra.unlink()
            platform = dependencies / "EpochPlatformEngine"
            (platform / "next.cpp").write_text("new revision")
            git(platform, "add", "next.cpp")
            git(platform, "commit", "-qm", "Unadmitted dependency revision")
            with self.assertRaisesRegex(ValueError, "differs from the demo pin"):
                packager.bundle(demo, dependencies, demo / "build/packages/wrong-pin")

    def test_candidate_workflows_cannot_publish_or_create_issues(self):
        for name in ("windows-release.yml", "linux-release.yml"):
            workflow = (ROOT / ".github/workflows" / name).read_text()
            with self.subTest(workflow=name):
                self.assertIn("workflow_dispatch:", workflow)
                self.assertNotIn("workflow_run:", workflow)
                self.assertIn("contents: read", workflow)
                self.assertNotIn("contents: write", workflow)
                self.assertNotIn("issues: write", workflow)
                self.assertNotIn("gh release", workflow)
                self.assertNotIn("gh issue", workflow)
                self.assertIn("tools/package_source.py", workflow)
                self.assertIn("tools/package_runtime.py", workflow)
                self.assertIn("build/offline", workflow)
                self.assertNotIn("--native-acceptance", workflow)


if __name__ == "__main__": unittest.main()
