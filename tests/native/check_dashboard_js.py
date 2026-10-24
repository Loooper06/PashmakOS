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

style_match = re.search(r"<style>\s*(.*?)\s*</style>", source, flags=re.DOTALL)
if not style_match:
    raise SystemExit("Dashboard style block not found in WebDashboard.cpp")

required_css_fragments = (
    "@media(max-width:600px)",
    "header>div:last-child{display:grid",
    '.grid>article[style*="grid-column:span 2"]{grid-column:1/-1!important}',
    "button{min-height:44px}",
    "@media(max-width:380px)",
)
for fragment in required_css_fragments:
    if fragment not in style_match.group(1):
        raise SystemExit(f"Dashboard responsive CSS check failed: missing {fragment!r}")

required_fragments = (
    r"if(/^[\s\u0000-\u001f]*[=+@-]/.test(text))",
    r"join('\r\n')",
    r"new Blob(['\ufeff',csv]",
    "function isValidDateKey",
    "refreshRequested?250:retryDelay",
    "pet-option-state",
    "pet-option-energy-track",
    "pet-happiness",
    "pet-happiness-bar",
    "globalAchievementMask",
    "achievementMask",
    "renderAchievements(data)",
    "pet-option-energy-fill",
    "pet-happiness-bar",
    "data.version!==3",
    "Mood · '+mood",
    "energyFill.style.width=energy+'%'",
)
for fragment in required_fragments:
    if fragment not in match.group(1):
        raise SystemExit(f"Dashboard regression check failed: missing {fragment!r}")

with tempfile.NamedTemporaryFile("w", suffix=".js", encoding="utf-8") as script:
    script.write(match.group(1))
    script.flush()
    result = subprocess.run(["node", "--check", script.name], check=False)
    if result.returncode:
        raise SystemExit(result.returncode)

print("Dashboard JavaScript syntax: PASS")
