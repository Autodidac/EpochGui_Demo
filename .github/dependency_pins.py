import json
import os
import re
import sys
from pathlib import Path

composition = sys.argv[1]
selected = ["Gui"] + (["Platform", "Audio", "Media"] if composition == "full" else [composition.title()] if composition != "gui" else [])
pins = json.loads(Path("dependencies.json").read_text())["source_revisions"]
with open(os.environ["GITHUB_OUTPUT"], "a") as output:
    for owner in selected:
        revision = pins.get(owner)
        if not isinstance(revision, str) or not re.fullmatch("[0-9a-f]{40}", revision):
            raise SystemExit("Missing accepted source revision: " + owner)
        output.write(owner.lower() + "=" + revision + "\n")
