#!/usr/bin/env python3
"""Package clean committed public demo/library sources; never publish a release."""
import argparse
import hashlib
import importlib.util
import io
import json
from pathlib import Path, PurePosixPath
import re
import subprocess
import tarfile
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
OWNERS = {"Gui": "EpochGui", "Platform": "EpochPlatformEngine",
          "Audio": "EpochAudioEngine", "Media": "EpochMediaEngine"}
FORBIDDEN_PARTS = {".git", ".codex", ".aws", ".ssh", "EpochSelfCoding"}
FORBIDDEN_SUFFIXES = {".exe", ".dll", ".pdb", ".obj", ".gguf", ".safetensors", ".zip", ".pem", ".pfx"}


def git(root, *args):
    return subprocess.check_output(["git", "-C", str(root), *args])


def source_tree(root, project, expected_revision=None):
    root = root.resolve(strict=True)
    if git(root, "status", "--porcelain").strip():
        raise ValueError("Commit or preserve pending source first; packaging requires a clean checkout: " + project)
    remote = git(root, "remote", "get-url", "origin").decode().strip().rstrip("/").removesuffix(".git")
    if remote != f"https://github.com/Autodidac/{project}":
        raise ValueError("Unexpected public source repository: " + project)
    revision = git(root, "rev-parse", "HEAD").decode().strip()
    if expected_revision is not None and revision != expected_revision:
        raise ValueError("Dependency revision differs from the demo pin: " + project)
    result = {}
    with tarfile.open(fileobj=io.BytesIO(git(root, "archive", "--format=tar", revision))) as archive:
        for member in archive:
            if member.isdir():
                continue
            name = PurePosixPath(member.name)
            if (name.is_absolute() or ".." in name.parts or "\\" in member.name
                    or any(part in FORBIDDEN_PARTS or part.startswith(".env") for part in name.parts)
                    or name.suffix.lower() in FORBIDDEN_SUFFIXES or not member.isfile()
                    or name.parts[0] in {"build", "out", "logs", "cache"}):
                raise ValueError("Non-source or unsafe tracked payload: " + project + "/" + member.name)
            # These generated receipts predate their source commits. A fresh
            # inventory is written below, never shipped as old runtime proof.
            if member.name == "source-audit.json":
                continue
            if member.name in result:
                raise ValueError("Duplicate source path: " + member.name)
            result[member.name] = (archive.extractfile(member).read(), member.mode)
    if "LICENSE" not in result:
        raise ValueError("Source project has no license: " + project)
    return revision, result


def bundle(source_root, dependency_root, output_dir):
    source_root = source_root.resolve(strict=True)
    dependency_root = dependency_root.resolve(strict=True)
    output_dir = output_dir.resolve()
    if not output_dir.is_relative_to(source_root / "build"):
        raise ValueError("Package output must remain under the demo's build directory")
    revision, demo = source_tree(source_root, "EpochGui_Demo")
    version = demo["VERSION"][0].decode("ascii").strip()
    if not re.fullmatch(r"\d+\.\d+\.\d+", version):
        raise ValueError("Invalid committed demo VERSION")
    pins = json.loads(demo["dependencies.json"][0])["source_revisions"]
    files = dict(demo)
    admitted = {}
    for owner, project in OWNERS.items():
        pin = pins.get(owner)
        if not isinstance(pin, str) or not re.fullmatch(r"[0-9a-f]{40}", pin):
            raise ValueError("Dependency needs an exact public revision: " + owner)
        actual, entries = source_tree(dependency_root / project, project, pin)
        admitted[project] = actual
        inventory = [{"path": name, "sha256": hashlib.sha256(data).hexdigest()}
                     for name, (data, _) in sorted(entries.items())]
        receipt = {"schema": "epoch.demo-dependency-source.v1",
                   "repository": f"https://github.com/Autodidac/{project}.git",
                   "revision": actual, "files": inventory}
        entries["epoch-demo-source.json"] = ((json.dumps(receipt, indent=2) + "\n").encode(), 0o644)
        for name, entry in entries.items():
            files[f"dependencies/{project}/{name}"] = entry
    notices = "# Included public source\n\nEpochGui_Demo is MIT.\n\n" + "\n".join(
        f"- {project}: MPL-2.0. Full license: dependencies/{project}/LICENSE. Source: https://github.com/Autodidac/{project}"
        for project in admitted) + "\n"
    files["THIRD_PARTY_NOTICES.md"] = (notices.encode(), 0o644)
    manifest = {"schema": "epoch.demo-source-bundle.v1", "version": version,
                "demo_revision": revision, "dependency_revisions": admitted,
                "scope": "Complete offline public source closure; no build or native runtime acceptance",
                "files": [{"path": name, "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}
                          for name, (data, _) in sorted(files.items())]}
    files["SOURCE_MANIFEST.json"] = ((json.dumps(manifest, indent=2) + "\n").encode(), 0o644)
    output_dir.mkdir(parents=True, exist_ok=True)
    artifact = output_dir / f"EpochGui_Demo-v{version}-source.zip"
    prefix = f"EpochGui_Demo-v{version}-source"
    with zipfile.ZipFile(artifact, "x", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, (data, mode) in sorted(files.items()):
            info = zipfile.ZipInfo(prefix + "/" + name, date_time=(2026, 1, 1, 0, 0, 0))
            info.create_system = 3
            info.external_attr = (0o100000 | mode) << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, data)
    # Validate the actual written ZIP, then use its own restore verifier on each
    # extracted gitless dependency. No network, CMake, executable or model starts.
    verify_parent = source_root / "build/package-verify"
    verify_parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=verify_parent, prefix="source-") as temporary:
        with zipfile.ZipFile(artifact) as archive:
            if set(archive.namelist()) != {prefix + "/" + name for name in files}:
                raise ValueError("Source ZIP inventory differs from the admitted files")
            for name, (data, _) in files.items():
                if archive.read(prefix + "/" + name) != data:
                    raise ValueError("Source ZIP bytes differ from the committed source: " + name)
            archive.extractall(temporary)
        extracted = Path(temporary) / prefix
        spec = importlib.util.spec_from_file_location("packaged_demo_dependencies", extracted / "tools/prepare_dependencies.py")
        verifier = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(verifier)
        for project, pin in admitted.items():
            verifier.verify_bundle(extracted / "dependencies" / project, pin)
    digest = hashlib.sha256(artifact.read_bytes()).hexdigest()
    Path(str(artifact) + ".sha256").write_text(digest + "  " + artifact.name + "\n", encoding="ascii")
    return artifact, manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dependency-root", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    args = parser.parse_args()
    try:
        artifact, manifest = bundle(ROOT, args.dependency_root, args.output_dir)
    except (OSError, ValueError, KeyError, subprocess.SubprocessError) as error:
        parser.exit(1, f"Source candidate refused: {error}\n")
    print(f"Source candidate: {artifact}\nSource revision: {manifest['demo_revision']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
