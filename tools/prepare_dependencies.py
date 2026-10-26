#!/usr/bin/env python3
"""Explicitly fetch the demo's pinned public libraries, or verify a bundled copy."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
RECEIPT = "epoch-demo-source.json"
OWNERS = {"Gui": "EpochGui", "Platform": "EpochPlatformEngine",
          "Audio": "EpochAudioEngine", "Media": "EpochMediaEngine"}


def digest(path):
    result = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            result.update(block)
    return result.hexdigest()


def git(root, *args):
    environment = dict(os.environ, GIT_TERMINAL_PROMPT="0")
    return subprocess.check_output(["git", "-C", str(root), *args],
                                   env=environment, text=True).strip()


def verify_bundle(root, revision):
    receipt = json.loads((root / RECEIPT).read_text(encoding="utf-8"))
    if receipt.get("schema") != "epoch.demo-dependency-source.v1" or receipt.get("revision") != revision:
        raise ValueError("Dependency receipt does not match its exact source pin: " + str(root))
    records = receipt.get("files")
    if not isinstance(records, list) or not records:
        raise ValueError("Dependency receipt has no source inventory")
    seen = set()
    for record in records:
        name = record["path"]
        relative = Path(name)
        path = root / relative
        if (relative.is_absolute() or ".." in relative.parts or name in seen
                or not path.resolve().is_relative_to(root.resolve()) or path.is_symlink()
                or not path.is_file() or digest(path) != record["sha256"]):
            raise ValueError("Dependency source differs from its bundled receipt: " + name)
        seen.add(name)
    actual = {p.relative_to(root).as_posix() for p in root.rglob("*") if p.is_file()}
    if actual != seen | {RECEIPT}:
        raise ValueError("Dependency bundle contains unlisted files: " + str(root))


def prepare(root, owner, revision, check_only=False):
    if not re.fullmatch("[0-9a-f]{40}", revision):
        raise ValueError("Missing exact public dependency revision: " + owner)
    project = OWNERS[owner]
    destination = root / project
    url = f"https://github.com/Autodidac/{project}.git"
    if destination.exists() and not (destination / ".git").is_dir():
        verify_bundle(destination, revision)
        return destination
    if not destination.exists():
        if check_only:
            raise ValueError("Dependency missing; run prepare_dependencies.py first: " + project)
        destination.mkdir(parents=True, exist_ok=False)
        git(destination, "init")
        git(destination, "remote", "add", "origin", url)
        git(destination, "-c", "credential.helper=", "fetch", "--depth", "1", "origin", revision)
        git(destination, "checkout", "--detach", "FETCH_HEAD")
    if (git(destination, "remote", "get-url", "origin").rstrip("/").removesuffix(".git") != url.removesuffix(".git")
            or git(destination, "rev-parse", "HEAD") != revision):
        raise ValueError("Existing dependency has a different identity; it was left untouched: " + project)
    subprocess.run(["git", "-C", str(destination), "diff", "--exit-code", "HEAD", "--"], check=True)
    extras = set(filter(None, git(destination, "ls-files", "--others", "--exclude-standard", "-z").split("\0")))
    if extras - {RECEIPT}:
        raise ValueError("Dependency has untracked source; it was left untouched: " + project)
    # Keep Git checkouts clean. package_source.py writes byte inventories only
    # into its gitless export; ordinary dependency setup creates no loose files.
    return destination


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--composition", choices=("gui", "platform", "audio", "media", "full"), default="full")
    parser.add_argument("--check", action="store_true", help="Verify only; never fetch or replace a library")
    args = parser.parse_args()
    selected = list(OWNERS) if args.composition == "full" else ["Gui"]
    if args.composition not in ("gui", "full"):
        selected.append(args.composition.title())
    pins = json.loads((ROOT / "dependencies.json").read_text(encoding="utf-8"))["source_revisions"]
    try:
        for owner in selected:
            path = prepare(ROOT / "dependencies", owner, pins[owner], args.check)
            print(f"{owner}: {pins[owner]} {path}")
    except (OSError, ValueError, KeyError, subprocess.SubprocessError) as error:
        parser.exit(1, f"Dependency setup stopped: {error}\nExisting folders were not replaced.\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
