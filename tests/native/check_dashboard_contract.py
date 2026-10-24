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
select = function_body(web, "void WebDashboard::handlePetSelection()", "void WebDashboard::handlePetInteraction()")
interaction = function_body(web, "void WebDashboard::handlePetInteraction()", "void WebDashboard::handleTimerSettingsGet()")
timer_get = function_body(web, "void WebDashboard::handleTimerSettingsGet()", "void WebDashboard::handleTimerSettingsPost()")
timer_post = function_body(web, "void WebDashboard::handleTimerSettingsPost()", "void WebDashboard::handleQuietHoursGet()")
quiet_post = function_body(web, "void WebDashboard::handleQuietHoursPost()", "void WebDashboard::handleReminderSettingsGet()")
reminder_get = function_body(web, "void WebDashboard::handleReminderSettingsGet()", "void WebDashboard::handleReminderSettingsPost()")
reminder_post = function_body(web, "void WebDashboard::handleReminderSettingsPost()", "void WebDashboard::handleNotFound()")
reminder_setter = function_body(app, "bool PashmakApp::setReminderSettings(", "bool PashmakApp::isQuietHour(")
reminder_load = function_body(app, "void PashmakApp::loadReminderSettings()", "void PashmakApp::loadPetProfiles()")
timer_setter = function_body(app, "bool PashmakApp::setTimerSettings(", "uint8_t PashmakApp::dailyHistoryCount()")
timer_load = function_body(app, "void PashmakApp::loadTimerSettings()", "void PashmakApp::loadQuietHoursSettings()")
quiet_setter = function_body(app, "bool PashmakApp::setQuietHoursSettings(", "PashmakApp::TimerSettings PashmakApp::timerSettings() const")
quiet_logic = function_body(app, "bool PashmakApp::isQuietHour(", "PashmakApp::TimerSettings PashmakApp::timerSettings() const")
quiet_load = function_body(app, "void PashmakApp::loadQuietHoursSettings()", "void PashmakApp::loadPetProfiles()")
automatic_mode = function_body(app, "void PashmakApp::updateAutomaticMode(", "void PashmakApp::loadTimerSettings()")
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
    "pet interaction endpoint is registered": '_server.on("/api/pet/interact", HTTP_POST' in web,
    "pet interaction endpoint applies cooldown and returns 429": "_app->interactWithActivePet(millis())" in interaction and "_server.send(429" in interaction,
    "dashboard exposes bond meter and interaction button": 'id="pet-bond-progress"' in web and 'id="pet-interact"' in web,
    "dashboard shows bond relationship tier and next milestone": 'id="pet-bond-stage"' in web and 'id="pet-bond-next"' in web and "function bondTier(value)" in js,
    "pet picker shows each profile bond tier and progress": "const bond=Math.max(0,Math.min(100,Number(p.bond)||0))" in js and "bondLabel.textContent=tier.name+' · '+bond+'/100'" in js and "pet-option-bond-fill" in js and "bond '+bond+' of 100" in js,
    "interaction acknowledgement reports the current bond tier": "tier.remaining+' more to '+tier.next" in js and "' · maximum bond'" in js,
    "dashboard validates bond range and selected-pet consistency": "Number.isInteger(Number(p.bond))" in js and "Number(data.pet.bond)!==Number(selected.bond)" in js,
    "dashboard POSTs interaction and handles cooldown": "fetch('/api/pet/interact',{method:'POST'" in js and "response.status===429" in js,
    "quiet hours and configured intervals are passed to the RTC reminder path": "update(_mode, now, currentHour, quietHours)" in app and "_rtc.update(" in app and "_reminderSettings.waterMinutes, _reminderSettings.movementMinutes" in app,
    "quiet-hours endpoints are registered": '_server.on("/api/quiet-hours", HTTP_GET' in web and '_server.on("/api/quiet-hours", HTTP_POST' in web,
    "quiet-hours API validates distinct hour bounds": "startHour == endHour" in quiet_post and "startHour > 23 || endHour > 23" in quiet_post,
    "quiet-hours settings are persisted in a versioned blob": '_statsPreferences.putBytes("quietCfg", &stored, sizeof(stored))' in quiet_setter and 'stored.version != 1' in quiet_load,
    "quiet-hours default and rollover-midnight ranges are supported": "_quietHours = QuietHoursSettings{};" in quiet_load and "return hour >= _quietHours.startHour || hour < _quietHours.endHour;" in quiet_logic,
    "dashboard loads, validates, and saves quiet hours": "async function loadQuietHours()" in js and "async function saveQuietHours()" in js and "fetch('/api/quiet-hours',{method:'POST'" in js,
    "dashboard exposes accessible quiet-hours controls": 'id="quiet-enabled"' in web and 'id="quiet-start"' in web and 'id="quiet-end"' in web and 'id="quiet-status"' in web,
    "reminder settings endpoints are registered for GET and POST": '_server.on("/api/reminder-settings", HTTP_GET' in web and '_server.on("/api/reminder-settings", HTTP_POST' in web,
    "reminder settings GET returns both intervals and disables caching": "settings.waterMinutes, settings.movementMinutes" in reminder_get and '"Cache-Control", "no-store"' in reminder_get,
    "reminder settings POST validates ranges and supports zero to disable": 'values[0] < 15 || values[0] > 240' in reminder_post and 'values[1] < 15 || values[1] > 180' in reminder_post and 'values[0] != 0' in reminder_post and 'values[1] != 0' in reminder_post,
    "reminder settings persist in a versioned Preferences blob": '_statsPreferences.putBytes("reminderCfg", &stored, sizeof(stored))' in reminder_setter and 'stored.version != 1' in reminder_load,
    "dashboard loads, validates, and saves reminder settings": "async function loadReminderSettings()" in js and "async function saveReminderSettings()" in js and "fetch('/api/reminder-settings',{method:'POST'" in js,
    "dashboard exposes accessible reminder controls": 'id="reminder-water"' in web and 'id="reminder-movement"' in web and 'id="reminder-status"' in web,
    "focus insights use the latest seven retained daily records": "slice(-7)" in js and "function renderFocusInsights(data)" in js,
    "focus insights derive average, best day, active days, and sessions from local history": "insight-average" in js and "insight-best-day" in js and "insight-active-days" in js and "insight-sessions" in js and "renderFocusInsights(data)" in js,
    "focus insights provide an empty state without daily records": "No daily history is available yet." in js,
    "daily goal streak uses the configured local goal and retained daily history": "function renderGoalStreak(data)" in js and "dailySessionGoal" in js and "id=\"goal-streak-count\"" in web,
    "daily goal streak stops when a day misses the goal or calendar continuity breaks": "previousDay-day!==86400000" in js and "completedWorkSessions||0)<goal" in js,
    "daily goal streak renders an empty-history state and is refreshed with the dashboard": "No daily history recorded yet." in js and "renderGoalStreak(data);" in js,
    "dashboard POSTs selection and checks HTTP status": "fetch('/api/pet/select',{method:'POST'" in js and "if(!response.ok)throw new Error('HTTP '+response.status)" in js,
    "dashboard times out statistics requests": "setTimeout(()=>controller.abort(),5000)" in js,
    "dashboard validates payload before rendering": "if(!isValidStats(data))throw new Error('Invalid statistics payload')" in js,
    "dashboard retains last valid snapshot on failures": "snapshot?'Stale data · last valid update" in js,
    "dashboard uses bounded retry backoff": "Math.min(retryDelay*2,60000)" in js,
    "dashboard avoids overlapping refreshes": "if(refreshInFlight){refreshRequested=true;return}" in js,
    "dashboard requests uncached API data": "fetch('/api/stats',{cache:'no-store'" in js,
    "timer settings endpoints are registered for GET and POST": '_server.on("/api/timer-settings", HTTP_GET' in web and '_server.on("/api/timer-settings", HTTP_POST' in web,
    "timer settings GET disables caching": '"Cache-Control", "no-store"' in timer_get,
    "timer settings POST rejects missing fields": '!_server.hasArg(keys[i])' in timer_post,
    "timer settings POST validates all allowed ranges": 'values[0] > 180' in timer_post and 'values[1] > 60' in timer_post and 'values[2] > 120' in timer_post and 'values[3] > 8' in timer_post,
    "timer settings setter validates ranges before saving": 'settings.workMinutes > 180' in timer_setter and 'settings.playMinutes > 60' in timer_setter and 'settings.napMinutes > 120' in timer_setter and 'settings.workCyclesBeforeLongRest > 8' in timer_setter,
    "timer settings are stored as a versioned Preferences blob": '_statsPreferences.putBytes("timerCfg", &stored, sizeof(stored))' in timer_setter and 'stored.version != 1' in timer_load,
    "invalid saved timer settings fall back to defaults": '_timerSettings = TimerSettings{};' in timer_load and 'Invalid saved settings; using defaults.' in timer_load,
    "automatic cycle uses runtime-configured durations": '_timerSettings.workMinutes' in automatic_mode and '_timerSettings.playMinutes' in automatic_mode and '_timerSettings.napMinutes' in automatic_mode and '_timerSettings.workCyclesBeforeLongRest' in automatic_mode,
    "dashboard exposes settings form and save action": 'id="setting-work"' in web and 'id="setting-cycles"' in web and "async function saveTimerSettings()" in js,
    "dashboard validates and submits timer settings": "Check the allowed ranges before saving." in js and "fetch('/api/timer-settings',{method:'POST'" in js and "loadTimerSettings();" in js,

}

failed = [name for name, ok in checks.items() if not ok]
for name, ok in checks.items():
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")
if failed:
    raise SystemExit(f"{len(failed)} dashboard API contract check(s) failed")

print(f"Dashboard API contract: PASS ({len(checks)} checks)")
