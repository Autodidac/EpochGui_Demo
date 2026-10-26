#!/usr/bin/env python3
"""Stage a demonstration candidate with its complete offline project; no release writes."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import shutil
import stat
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
NATIVE_CHECKS = ("launch", "controls", "multiline_clip", "windows_docking", "compressed_resize", "normal_exit")


def admit_native_acceptance(path, platform, binary_sha256, source_sha256):
    path = Path(path)
    if path.stat().st_size > 65536:
        raise ValueError("Native acceptance must be a concise native verification receipt")
    data = json.loads(path.read_text(encoding="utf-8"))
    if (data.get("schema") != "epoch.demo-native-acceptance.v1"
            or data.get("platform") != platform or data.get("binary_sha256") != binary_sha256
            or data.get("source_archive_sha256") != source_sha256):
        raise ValueError("Native acceptance does not identify this exact binary and source archive")
    checks = data.get("checks")
    if not isinstance(checks, dict) or any(checks.get(name) != "pass" for name in NATIVE_CHECKS):
        raise ValueError("Native acceptance has missing or failed required checks")
    # Curated verdict only: no local paths, logs or arbitrary receipt fields enter a release.
    return {name: checks[name] for name in NATIVE_CHECKS}


def package(binary, source_zip, platform, output_dir, native_acceptance=None):
    binary, source_zip = binary.resolve(strict=True), source_zip.resolve(strict=True)
    output_dir = output_dir.resolve()
    if not output_dir.is_relative_to(ROOT / "build"):
        raise ValueError("Runtime candidates stay under the demo build directory")
    revision = subprocess.check_output(["git", "-C", str(ROOT), "rev-parse", "HEAD"]).decode().strip()
    if subprocess.check_output(["git", "-C", str(ROOT), "status", "--porcelain"]).strip():
        raise ValueError("Runtime packaging requires clean committed source")
    binary_sha256 = hashlib.sha256(binary.read_bytes()).hexdigest()
    source_sha256 = hashlib.sha256(source_zip.read_bytes()).hexdigest()
    native = admit_native_acceptance(native_acceptance, platform, binary_sha256, source_sha256) if native_acceptance else None
    with zipfile.ZipFile(source_zip) as source:
        manifests = [n for n in source.namelist() if n.endswith("/SOURCE_MANIFEST.json")]
        if len(manifests) != 1: raise ValueError("Missing unique source manifest")
        manifest_path = PurePosixPath(manifests[0])
        prefix = manifest_path.parts[0]
        manifest = json.loads(source.read(str(manifest_path)))
        if manifest.get("schema") != "epoch.demo-source-bundle.v1" or manifest.get("demo_revision") != revision:
            raise ValueError("Source bundle does not match this exact source commit")
        version = manifest["version"]
        records = {r["path"]: r for r in manifest["files"]}
        expected = {prefix + "/" + name for name in records} | {str(manifest_path)}
        if set(source.namelist()) != expected or len(source.infolist()) != len(expected):
            raise ValueError("Source bundle inventory differs")
        for info in source.infolist():
            path = PurePosixPath(info.filename)
            if (path.is_absolute() or ".." in path.parts or "\\" in info.filename
                    or path.parts[0] != prefix or stat.S_ISLNK(info.external_attr >> 16)):
                raise ValueError("Unsafe source bundle path")
            name = "/".join(path.parts[1:])
            if name in records and hashlib.sha256(source.read(info)).hexdigest() != records[name]["sha256"]:
                raise ValueError("Source bundle bytes differ: " + name)
        stage_parent = ROOT / "build/package-runtime"
        stage_parent.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=stage_parent, prefix="candidate-") as temporary:
            stage = Path(temporary)
            source.extractall(stage / "project")
            if platform == "linux-x64":
                for info in source.infolist():
                    (stage / "project" / info.filename).chmod((info.external_attr >> 16) & 0o777)
            project = stage / "project" / prefix
            header = binary.read_bytes()[:20]
            if platform == "windows-x64":
                if header[:2] != b"MZ": raise ValueError("Expected a Windows demonstration executable")
                executable = stage / "EpochGuiDemo.exe"
            else:
                if header[:6] != b"\x7fELF\x02\x01" or header[18:20] != b"\x3e\x00":
                    raise ValueError("Expected a Linux x64 demonstration executable")
                executable = stage / "EpochGuiDemo"
            shutil.copy2(binary, executable)
            shutil.copytree(project / "assets", stage / "assets")
            shutil.copyfile(project / "LICENSE", stage / "LICENSE")
            shutil.copyfile(project / "THIRD_PARTY_NOTICES.md", stage / "THIRD_PARTY_NOTICES.md")
            status = "EpochGui demonstration release: exact binary and source have verified native checks.\n" if native else \
                "EpochGui demonstration candidate: native acceptance has not run.\n"
            gate = "See VALIDATION.json for the accepted checks.\n" if native else \
                "This exact extracted executable must pass native checks before publication.\n"
            (stage / "README.txt").write_text(status +
                "Extract the complete folder before running.\n"
                "The complete project and pinned public libraries are in project/.\n"
                "Windows requires the Microsoft Visual C++2022 x64 runtime.\n"
                + gate, encoding="utf-8")
            receipt = {"schema": "epoch.demo-release-validation.v1" if native else "epoch.demo-runtime-candidate.v1", "version": version,
                       "source_revision": revision, "platform": platform,
                       "binary_sha256": binary_sha256, "source_archive_sha256": source_sha256,
                       "native_acceptance": "native_verified" if native else "not_run",
                       "publication": "manual_owner_gate"}
            if native: receipt["native_checks"] = native
            (stage / ("VALIDATION.json" if native else "candidate.json")).write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
            output_dir.mkdir(parents=True, exist_ok=True)
            artifact = output_dir / f"EpochGui_Demo-v{version}-{platform}.zip"
            with zipfile.ZipFile(artifact, "x", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
                for path in sorted(stage.rglob("*")):
                    if path.is_file(): archive.write(path, path.relative_to(stage))
    with zipfile.ZipFile(artifact) as archive:
        if archive.testzip() is not None: raise ValueError("Candidate ZIP checksum failed")
        if hashlib.sha256(archive.read(executable.name)).hexdigest() != receipt["binary_sha256"]:
            raise ValueError("Candidate executable differs from the built executable")
    digest = hashlib.sha256(artifact.read_bytes()).hexdigest()
    Path(str(artifact) + ".sha256").write_text(digest + "  " + artifact.name + "\n", encoding="ascii")
    return artifact


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", required=True, type=Path)
    parser.add_argument("--source-zip", required=True, type=Path)
    parser.add_argument("--platform", choices=("windows-x64", "linux-x64"), required=True)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--native-acceptance", type=Path,
                        help="Native verification receipt binding all required checks to this exact binary and source ZIP")
    args = parser.parse_args()
    try:
        print(package(args.binary, args.source_zip, args.platform, args.output_dir, args.native_acceptance))
    except (OSError, ValueError, KeyError, subprocess.SubprocessError, zipfile.BadZipFile) as error:
        parser.exit(1, f"Runtime candidate refused: {error}\n")
