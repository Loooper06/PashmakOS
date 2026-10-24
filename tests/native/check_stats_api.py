#!/usr/bin/env python3
"""Regression checks for the statistics JSON API assembled in PashmakApp.cpp."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = (root / "src" / "app" / "PashmakApp.cpp").read_text(encoding="utf-8")
start = source.find("String PashmakApp::statisticsToJson(")
end = source.find("\\nvoid PashmakApp::begin()", start)
if start < 0 or end < 0:
    raise SystemExit("statisticsToJson() implementation not found")

method = source[start:end]

# Check property names without depending on C++ string-literal escaping.
required_keys = (
    "version", "activePetId", "pets", "pet", "totals", "recent",
    "dailyHistory", "monthlyHistory", "requestedDays", "daysIncluded",
    "dateKey", "monthKey", "completedWorkSessions", "energy", "mood",
    "message", "workMs", "playMs", "napMs",
)
for key in required_keys:
    if key not in method:
        raise SystemExit(f"Statistics API regression: missing key/property {key}")

# Dynamic strings must always pass through the JSON string escaper.
for expression in ("petNames[i]", "profileMood", "profile.message", "mood", "pet.message"):
    if f"appendJsonString(json, {expression})" not in method:
        raise SystemExit(f"Statistics API regression: {expression} is not JSON-escaped")

if "StatsAggregation::summarizeRecentDays" not in source:
    raise SystemExit("Recent history aggregation helper is not wired into the app")
if "StatsAggregation::summarizeMonth" not in source:
    raise SystemExit("Monthly history aggregation helper is not wired into the app")

print("Statistics JSON API contract: PASS")
