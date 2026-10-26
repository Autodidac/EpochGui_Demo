"""Audit tracked public source only; never upload build products or credentials."""
from pathlib import Path
import hashlib
import json
import subprocess

root = Path(__file__).resolve().parent.parent
files = subprocess.check_output(["git", "-C", str(root), "ls-files", "--cached",
                                 "--others", "--exclude-standard", "-z"]).decode().split("\0")
for required in ("LICENSE",):
    if not (root / required).is_file():
        raise SystemExit("Missing license: " + required)
forbidden = {".env", ".codex", ".aws", ".ssh", "credentials.json", "auth.json", "EpochSelfCoding"}
records = []
for name in sorted(set(filter(None, files))):
    if name == "source-audit.json":
        raise SystemExit("Obsolete root source-audit.json must not be tracked or shipped as current proof")
    path = Path(name)
    if any(part in forbidden or part.startswith(".env.") for part in path.parts):
        raise SystemExit("Private file in public source: " + name)
    if path.suffix.lower() in {".exe", ".dll", ".pdb", ".obj", ".gguf", ".safetensors", ".zip", ".pem", ".pfx"}:
        raise SystemExit("Non-source payload in public source: " + name)
    records.append({"path": name, "sha256": hashlib.sha256((root / path).read_bytes()).hexdigest()})
revision = subprocess.check_output(["git", "-C", str(root), "rev-parse", "HEAD"]).decode().strip()
clean = not subprocess.check_output(["git", "-C", str(root), "status", "--porcelain"]).strip()
receipt = {"schema": "epoch.demo-source-audit.v2", "base_revision": revision,
           "source_revision": revision if clean else None, "working_tree_clean": clean,
           "license": "MIT", "files": records,
           "scope": "Source path/license and working-byte inventory only; no build or native runtime acceptance"}
destination = root / "build/audits/source-audit.json"
destination.parent.mkdir(parents=True, exist_ok=True)
destination.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
print("Source audit: " + str(destination))
