#!/usr/bin/env python3
"""Extract and syntax-check the dashboard JavaScript embedded in WebDashboard.cpp."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / "src" / "web" / "WebDashboard.cpp").read_text(encoding="utf-8")
match = re.search(r"<script>\s*(.*?)\s*</script>", source, flags=re.DOTALL)
if not match:
    raise SystemExit("Dashboard script block not found in WebDashboard.cpp")

with tempfile.NamedTemporaryFile("w", suffix=".js", encoding="utf-8") as script:
    script.write(match.group(1))
    script.flush()
    result = subprocess.run(["node", "--check", script.name], check=False)
    if result.returncode:
        raise SystemExit(result.returncode)

print("Dashboard JavaScript syntax: PASS")
