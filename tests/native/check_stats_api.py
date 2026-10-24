#!/usr/bin/env python3
"""Compile and validate the portable statistics JSON serializer used by firmware."""
import json
import subprocess
import tempfile
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = (root / "src" / "app" / "PashmakApp.cpp").read_text(encoding="utf-8")
header = (root / "src" / "core" / "StatsJson.h").read_text(encoding="utf-8")
method_start = source.find("String PashmakApp::statisticsToJson(")
method_end = source.find("\nvoid PashmakApp::begin()", method_start)
if method_start < 0 or method_end < 0:
    raise SystemExit("statisticsToJson() implementation not found")
method = source[method_start:method_end]

if '#include "../core/StatsJson.h"' not in source:
    raise SystemExit("PashmakApp.cpp does not include the portable serializer")
if "StatsJson::appendStatisticsJson(json, payload)" not in method:
    raise SystemExit("statisticsToJson() does not use the portable serializer")
if "void appendStatisticsJson" not in header:
    raise SystemExit("portable statistics serializer implementation is missing")

fixture = root / "tests" / "native" / "StatsJsonFixture.cpp"
with tempfile.TemporaryDirectory(prefix="pashmak-stats-json-") as temp_dir:
    binary = Path(temp_dir) / "stats-json-fixture"
    compile_result = subprocess.run(
        [
            "c++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
            "-I", str(root / "src" / "core"),
            str(fixture), "-o", str(binary),
        ],
        text=True,
        capture_output=True,
        check=False,
    )
    if compile_result.returncode != 0:
        raise SystemExit(
            "Statistics JSON fixture compilation failed:\n"
            + compile_result.stdout + compile_result.stderr
        )
    run_result = subprocess.run(
        [str(binary)], text=True, capture_output=True, check=False
    )
    if run_result.returncode != 0:
        raise SystemExit("Statistics JSON fixture execution failed: " + run_result.stderr)

try:
    data = json.loads(run_result.stdout)
except json.JSONDecodeError as exc:
    raise SystemExit(f"Statistics serializer produced invalid JSON: {exc}")

assert data["version"] == 1
assert data["activePetId"] == 1
assert len(data["pets"]) == 3
assert data["pets"][0] == {
    "id": 0, "name": "Pashmak", "energy": 91, "mood": "happy",
    "message": 'hello "friend"\\path\nnext',
}
assert data["pets"][1]["message"] == "quiet\tmode"
assert data["pets"][2]["message"] == "control\x01byte"
assert data["pet"] == {
    "energy": 64, "mood": "sleepy", "message": "quiet\tmode"
}
assert data["totals"] == {
    "workMs": 9000000000, "playMs": 2000000000, "napMs": 300000000,
    "completedWorkSessions": 1234,
}
assert data["recent"] == {
    "requestedDays": 7, "daysIncluded": 2, "workMs": 5000,
    "playMs": 600, "napMs": 70, "completedWorkSessions": 3,
}
assert len(data["dailyHistory"]) == 2
assert data["dailyHistory"][0]["workMs"] == 4294967000
assert data["dailyHistory"][1]["dateKey"] == 20261024
assert len(data["monthlyHistory"]) == 2
assert data["monthlyHistory"][0]["workMs"] == 5000000000
assert data["monthlyHistory"][1]["monthKey"] == 202610

print("Statistics JSON API serialization: PASS")
