#!/usr/bin/env python3
"""Regression checks for the dashboard HTTP/API contract and client recovery rules."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
web = (ROOT / "src" / "web" / "WebDashboard.cpp").read_text(encoding="utf-8")
header = (ROOT / "src" / "web" / "WebDashboard.h").read_text(encoding="utf-8")
app = (ROOT / "src" / "app" / "PashmakApp.cpp").read_text(encoding="utf-8")


def function_body(source: str, signature: str, next_signature: str) -> str:
    start = source.find(signature)
    end = source.find(next_signature, start + len(signature))
    if start < 0 or end < 0:
        raise SystemExit(f"Could not isolate function: {signature}")
    return source[start:end]


stats = function_body(web, "void WebDashboard::handleStats()", "void WebDashboard::handlePetSelection()")
select = function_body(web, "void WebDashboard::handlePetSelection()", "void WebDashboard::handleNotFound()")
setter = function_body(app, "bool PashmakApp::setActivePet(", "uint8_t PashmakApp::activePetId()")
begin = function_body(app, "void PashmakApp::begin()", "void PashmakApp::setMode(")
client = re.search(r"<script>\s*(.*?)\s*</script>", web, flags=re.DOTALL)
if not client:
    raise SystemExit("Embedded dashboard script block not found")
js = client.group(1)

checks = {
    "stats endpoint returns 503 when app is unavailable": '_server.send(503, "application/json"' in stats,
    "stats endpoint disables caching": '"Cache-Control", "no-store"' in stats,
    "pet selection rejects missing ID": '!_server.hasArg("id")' in select,
    "pet selection accepts only a single ASCII digit 0..2": "value.length() != 1" in select and "value[0] < '0' || value[0] > '2'" in select,
    "pet selection delegates range validation to app": "_app->setActivePet(static_cast<uint8_t>(value[0] - '0'))" in select,
    "invalid selection returns HTTP 400": '_server.send(400, "application/json"' in select,
    "successful selection returns JSON acknowledgement": '_server.send(200, "application/json", "{\\\"ok\\\":true}")' in select,
    "app setter rejects out-of-range pet IDs": "if (petId >= PET_COUNT)" in setter and "return false;" in setter,
    "app setter persists valid selection when Preferences are ready": '_statsPreferences.putUChar("activePet", _activePetId)' in setter,
    "stored active-pet ID is loaded with a safe default": '_statsPreferences.getUChar("activePet", 0)' in begin,
    "invalid stored active-pet ID falls back to pet 0 before use": "if (_activePetId >= 3) _activePetId = 0;" in begin and begin.find("if (_activePetId >= 3) _activePetId = 0;") < begin.find("_renderer.draw("),
    "dashboard POSTs selection and checks HTTP status": "fetch('/api/pet/select',{method:'POST'" in js and "if(!response.ok)throw new Error('HTTP '+response.status)" in js,
    "dashboard times out statistics requests": "setTimeout(()=>controller.abort(),5000)" in js,
    "dashboard validates payload before rendering": "if(!isValidStats(data))throw new Error('Invalid statistics payload')" in js,
    "dashboard retains last valid snapshot on failures": "snapshot?'Stale data · last valid update" in js,
    "dashboard uses bounded retry backoff": "Math.min(retryDelay*2,60000)" in js,
    "dashboard avoids overlapping refreshes": "if(refreshInFlight){refreshRequested=true;return}" in js,
    "dashboard requests uncached API data": "fetch('/api/stats',{cache:'no-store'" in js,
}

failed = [name for name, ok in checks.items() if not ok]
for name, ok in checks.items():
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")
if failed:
    raise SystemExit(f"{len(failed)} dashboard API contract check(s) failed")

print(f"Dashboard API contract: PASS ({len(checks)} checks)")
