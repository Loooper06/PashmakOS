#include "WebDashboard.h"
#include "../app/PashmakApp.h"
#include <WiFi.h>
#include <esp_system.h>
#include <Preferences.h>
#include <cstring>
namespace
{
    constexpr char DEFAULT_WIFI_AP_SSID[] = "PashmakOS";
    constexpr char DEFAULT_WIFI_AP_PASSWORD[] = "pashmak123";
    constexpr uint8_t WIFI_AP_CONFIG_VERSION = 1;
    constexpr char WIFI_AP_CONFIG_NAMESPACE[] = "pashmak_wifi";
    constexpr char WIFI_AP_CONFIG_KEY[] = "apCfg";
    constexpr size_t WIFI_AP_SSID_CAPACITY = 33;
    constexpr size_t WIFI_AP_PASSWORD_CAPACITY = 64;
    struct StoredAccessPointConfig
    {
        uint8_t version;
        char ssid[WIFI_AP_SSID_CAPACITY];
        char password[WIFI_AP_PASSWORD_CAPACITY];
    };
    bool isValidAccessPointConfig(const StoredAccessPointConfig &config)
    {
        const size_t ssidLength = strnlen(config.ssid, sizeof(config.ssid));
        const size_t passwordLength = strnlen(config.password, sizeof(config.password));
        if (config.version != WIFI_AP_CONFIG_VERSION || ssidLength == 0 || ssidLength > 32 ||
            passwordLength < 8 || passwordLength > 63 ||
            config.ssid[ssidLength] != '\0' || config.password[passwordLength] != '\0')
            return false;
        for (size_t i = 0; i < ssidLength; ++i)
        {
            const unsigned char ch = static_cast<unsigned char>(config.ssid[i]);
            if (ch < 0x20 || ch > 0x7e || ch == '"' || ch == '\\')
                return false;
        }
        for (size_t i = 0; i < passwordLength; ++i)
        {
            const unsigned char ch = static_cast<unsigned char>(config.password[i]);
            if (ch < 0x20 || ch > 0x7e)
                return false;
        }
        return true;
    }
    void loadAccessPointConfig(char (&ssid)[WIFI_AP_SSID_CAPACITY],
                               char (&password)[WIFI_AP_PASSWORD_CAPACITY])
    {
        strlcpy(ssid, DEFAULT_WIFI_AP_SSID, sizeof(ssid));
        strlcpy(password, DEFAULT_WIFI_AP_PASSWORD, sizeof(password));
        Preferences preferences;
        if (!preferences.begin(WIFI_AP_CONFIG_NAMESPACE, true))
            return;
        StoredAccessPointConfig stored{};
        const size_t storedSize = preferences.getBytes(WIFI_AP_CONFIG_KEY, &stored, sizeof(stored));
        preferences.end();
        if (storedSize == sizeof(stored) && isValidAccessPointConfig(stored))
        {
            strlcpy(ssid, stored.ssid, sizeof(ssid));
            strlcpy(password, stored.password, sizeof(password));
        }
    }
    bool saveAccessPointConfig(const char *ssid, const char *password)
    {
        StoredAccessPointConfig stored{};
        stored.version = WIFI_AP_CONFIG_VERSION;
        strlcpy(stored.ssid, ssid, sizeof(stored.ssid));
        strlcpy(stored.password, password, sizeof(stored.password));
        if (!isValidAccessPointConfig(stored))
            return false;
        Preferences preferences;
        if (!preferences.begin(WIFI_AP_CONFIG_NAMESPACE, false))
            return false;
        const size_t written = preferences.putBytes(WIFI_AP_CONFIG_KEY, &stored, sizeof(stored));
        preferences.end();
        return written == sizeof(stored);
    }

    constexpr char EVENT_LOG_NAMESPACE[] = "pashmak_log";
    constexpr char EVENT_LOG_KEY[] = "evtlog";
    constexpr uint8_t EVENT_LOG_VERSION = 1;
    constexpr size_t EVENT_LOG_CAPACITY = 24;
    enum EventCode : uint8_t { EVENT_BOOT=1, EVENT_AP_START_FAILED=2, EVENT_PET_SELECTED=3, EVENT_TIMER_SETTINGS_SAVED=4, EVENT_QUIET_HOURS_SAVED=5, EVENT_REMINDERS_SAVED=6, EVENT_WIFI_SETTINGS_SAVED=7 };
    struct EventRecord { uint32_t sequence; uint32_t uptimeSeconds; uint8_t code; uint8_t detail; uint16_t reserved; };
    struct StoredEventLog { uint8_t version; uint8_t nextIndex; uint8_t count; uint8_t reserved; uint32_t nextSequence; EventRecord records[EVENT_LOG_CAPACITY]; };
    bool readEventLog(StoredEventLog &log) {
        memset(&log, 0, sizeof(log)); log.version=EVENT_LOG_VERSION; log.nextSequence=1;
        Preferences preferences;
        if (!preferences.begin(EVENT_LOG_NAMESPACE, true)) return false;
        const size_t size=preferences.getBytes(EVENT_LOG_KEY,&log,sizeof(log)); preferences.end();
        if(size!=sizeof(log)||log.version!=EVENT_LOG_VERSION||log.nextIndex>=EVENT_LOG_CAPACITY||log.count>EVENT_LOG_CAPACITY||log.nextSequence==0){
            memset(&log,0,sizeof(log));log.version=EVENT_LOG_VERSION;log.nextSequence=1;return false;
        }
        return true;
    }
    bool appendEvent(uint8_t code,uint8_t detail=0) {
        StoredEventLog log{}; readEventLog(log);
        EventRecord &record=log.records[log.nextIndex];record.sequence=log.nextSequence++;
        if(log.nextSequence==0)log.nextSequence=1;
        record.uptimeSeconds=millis()/1000UL;record.code=code;record.detail=detail;record.reserved=0;
        log.nextIndex=static_cast<uint8_t>((log.nextIndex+1U)%EVENT_LOG_CAPACITY);
        if(log.count<EVENT_LOG_CAPACITY)++log.count;
        Preferences preferences;if(!preferences.begin(EVENT_LOG_NAMESPACE,false))return false;
        const size_t written=preferences.putBytes(EVENT_LOG_KEY,&log,sizeof(log));preferences.end();return written==sizeof(log);
    }

    const char DASHBOARD_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="theme-color" content="#171923">
<title>PashmakOS Dashboard</title>
<style>
:root{color-scheme:dark;--bg:#11131b;--card:#1b1f2b;--muted:#9ca3b5;--text:#f4f5f8;--accent:#b9a0ff;--line:#303545}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--text);font:16px/1.5 system-ui,-apple-system,Segoe UI,sans-serif}
main{max-width:1050px;margin:auto;padding:22px 16px 40px}header{display:flex;align-items:center;justify-content:space-between;gap:12px;flex-wrap:wrap;margin-bottom:20px}
h1{font-size:clamp(1.45rem,4vw,2rem);margin:0}h2{font-size:1.05rem;margin:0 0 14px}.muted{color:var(--muted)}.status{font-size:.85rem;color:var(--accent)}.status[data-state="stale"],.status[data-state="error"]{color:#e7a35d}.status[data-state="connected"]{color:#78d8ad}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(145px,1fr));gap:12px;margin-bottom:16px}
.card{background:var(--card);border:1px solid var(--line);border-radius:16px;padding:16px;min-width:0}
.metric{font-size:clamp(1.25rem,3vw,1.8rem);font-weight:700;margin-top:8px;overflow-wrap:anywhere}.label{font-size:.88rem;color:var(--muted)}
.section{margin-top:16px}.table-wrap{overflow-x:auto}table{border-collapse:collapse;width:100%;white-space:nowrap;font-size:.9rem}th,td{text-align:left;padding:10px 12px;border-bottom:1px solid var(--line)}th{color:var(--muted);font-weight:600}
button{border:1px solid var(--line);background:#292e3e;color:var(--text);padding:9px 13px;border-radius:10px;font:inherit;cursor:pointer}button:hover{border-color:var(--accent)}
.pills{display:flex;gap:8px;flex-wrap:wrap;margin-bottom:12px}.empty{color:var(--muted);padding:12px 0}.trend-list{display:grid;gap:10px}.trend-row{display:grid;grid-template-columns:92px minmax(0,1fr);align-items:center;gap:12px}.trend-date{font-size:.82rem;color:var(--muted)}.trend-bars{display:grid;gap:4px}.trend-track{height:8px;background:var(--line);border-radius:99px;overflow:hidden}.trend-fill{height:100%;width:0;border-radius:99px;transition:width .25s}.trend-work{background:#b9a0ff}.trend-play{background:#8fe0d0}.trend-nap{background:#ffbf86}.trend-legend{display:flex;gap:14px;flex-wrap:wrap;margin-bottom:12px;font-size:.82rem;color:var(--muted)}.legend-dot{display:inline-block;width:8px;height:8px;border-radius:50%;margin-right:5px}.chart-empty{color:var(--muted);padding:8px 0}button:disabled{opacity:.45;cursor:not-allowed}
.pet-picker{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:10px;margin-bottom:12px}.pet-option{display:flex;align-items:center;gap:12px;text-align:left;width:100%;padding:12px;background:#202432;border:1px solid var(--line);border-radius:14px;transition:border-color .18s,transform .18s,background .18s}.pet-option:hover{transform:translateY(-1px)}.pet-option[aria-pressed=true]{border-color:var(--pet-color);background:color-mix(in srgb,var(--pet-color) 14%,var(--card));box-shadow:inset 0 0 0 1px var(--pet-color)}.pet-avatar{display:grid;place-items:center;flex:0 0 44px;height:44px;border-radius:14px;background:var(--pet-tint);font-size:1.65rem}.pet-option-copy{min-width:0;flex:1}.pet-option-name{display:block;font-weight:700}.pet-option-meta{display:block;color:var(--muted);font-size:.78rem}.pet-option-state{display:block;margin-top:4px;color:var(--muted);font-size:.74rem;overflow-wrap:anywhere}.pet-option-energy{display:block;margin-top:6px;color:var(--muted);font-size:.72rem}.pet-option-energy-track{display:block;height:4px;margin-top:4px;overflow:hidden;border-radius:99px;background:var(--line)}.pet-option-energy-fill{display:block;height:100%;border-radius:99px;background:var(--pet-color);transition:width .2s}.pet-option-bond{display:block;margin-top:5px;color:var(--pet-color);font-size:.74rem;font-weight:650}.pet-option-bond-track{display:block;height:4px;margin-top:5px;overflow:hidden;border-radius:99px;background:var(--line)}.pet-option-bond-fill{display:block;height:100%;border-radius:99px;background:var(--pet-color);transition:width .2s}.pet-selected{color:var(--pet-color);font-size:.8rem;font-weight:700}
.settings-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(190px,1fr));gap:12px}.settings-grid label{display:grid;gap:6px;color:var(--muted);font-size:.88rem}.settings-grid input{width:100%;min-width:0;border:1px solid var(--line);background:#11131b;color:var(--text);border-radius:10px;padding:10px;font:inherit}.settings-grid input:focus{outline:2px solid var(--accent);outline-offset:1px}
.goal-layout{display:grid;grid-template-columns:minmax(0,1fr) auto;align-items:center;gap:16px}.goal-progress{height:12px;background:var(--line);border-radius:99px;overflow:hidden;margin-top:12px}.goal-progress-fill{height:100%;width:0;background:linear-gradient(90deg,#8fe0d0,#b9a0ff);border-radius:99px;transition:width .25s}.goal-number{width:92px;border:1px solid var(--line);background:#11131b;color:var(--text);border-radius:10px;padding:10px;font:inherit}.goal-status{margin-top:10px;font-size:.9rem;color:var(--muted)}@media(max-width:480px){.goal-layout{grid-template-columns:1fr}.goal-number{width:100%}}
.insight-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(165px,1fr));gap:10px}.insight-card{background:#202432;border:1px solid var(--line);border-radius:13px;padding:13px;min-width:0}.insight-value{font-size:1.35rem;font-weight:700;margin-top:6px;overflow-wrap:anywhere}.insight-caption{font-size:.8rem;color:var(--muted);margin-top:4px}.insight-summary{margin:0 0 12px;color:var(--muted)}
.achievement-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(190px,1fr));gap:10px}.achievement{display:flex;gap:12px;align-items:center;border:1px solid var(--line);background:#202432;border-radius:14px;padding:12px;min-width:0}.achievement-icon{display:grid;place-items:center;flex:0 0 44px;height:44px;border-radius:13px;background:#302b43;font-size:1.45rem}.achievement.unlocked .achievement-icon{background:#3d365b}.achievement.locked{opacity:.58}.achievement-copy{min-width:0}.achievement-title{font-weight:700}.achievement-description{color:var(--muted);font-size:.8rem}.achievement-state{font-size:.72rem;color:#8fe0d0;font-weight:700;margin-top:3px}.achievement.locked .achievement-state{color:var(--muted)}.bond-row{display:flex;align-items:center;gap:12px;flex-wrap:wrap}.bond-stage{display:flex;align-items:center;justify-content:space-between;gap:10px;flex-wrap:wrap;margin-top:8px}.bond-stage-name{font-weight:700;color:#d7caff}.bond-stage-next{font-size:.82rem;color:var(--muted)}.bond-track{height:9px;min-width:100px;flex:1;background:var(--line);border-radius:99px;overflow:hidden}.bond-fill{height:100%;width:0;background:linear-gradient(90deg,#8fe0d0,#b9a0ff);transition:width .25s}.bond-actions{display:flex;align-items:center;gap:10px;flex-wrap:wrap}.bond-actions button{background:#3b3156;border-color:#65518e}.bond-actions button:disabled{opacity:.55;cursor:wait}
@media(max-width:600px){main{padding:14px 10px 28px}header{align-items:flex-start;gap:10px;margin-bottom:14px}header>div:last-child{display:grid;grid-template-columns:1fr 1fr;align-items:center;gap:8px;width:100%}header .status{grid-column:1/-1}.grid>article[style*="grid-column:span 2"]{grid-column:1/-1!important}.card{padding:13px;border-radius:13px}.pet-picker{grid-template-columns:minmax(0,1fr);gap:8px}.pet-option{padding:11px;gap:10px}.trend-row{grid-template-columns:76px minmax(0,1fr);gap:8px}.settings-grid{grid-template-columns:minmax(0,1fr)}.bond-track{min-width:80px}button{min-height:44px}.table-wrap{max-width:100%}footer{overflow-wrap:anywhere}}@media(max-width:380px){h2{font-size:1rem}.metric{font-size:1.35rem}.pet-avatar{flex-basis:40px;height:40px}.trend-date{font-size:.75rem}.bond-actions{align-items:stretch;flex-direction:column}.bond-actions button{width:100%}}
footer{margin-top:18px;color:var(--muted);font-size:.82rem}
</style>
</head>
<body><main>
<header><div><h1>PashmakOS</h1><div class="muted">Pet activity dashboard</div></div><div><span id="connection" class="status">Connecting…</span> <button id="export-json" type="button" disabled>Export JSON</button> <button id="refresh">Refresh</button></div></header>
<section class="card section"><h2>Choose your pet</h2><div id="pet-picker" class="pet-picker" role="group" aria-label="Choose active pet"></div><div class="muted">Choose a companion to view its current energy, mood and message. Each pet has an independent behavior state.</div></section>
<section class="card section" id="pet-status"><h2 id="pet-heading">Pet status</h2><div class="grid">
 <article><div class="label">Energy</div><div id="pet-energy" class="metric">—</div><div style="height:8px;background:var(--line);border-radius:99px;overflow:hidden;margin-top:10px"><div id="pet-energy-bar" style="height:100%;width:0%;background:var(--accent);transition:width .25s"></div></div></article>
 <article><div class="label">Mood</div><div id="pet-mood" class="metric">—</div></article>
 <article style="grid-column:span 2"><div class="label">Pet says</div><div id="pet-message" style="font-size:1.05rem;margin-top:8px">Waiting for data…</div></article>
 <article style="grid-column:span 2"><div class="label">Bond with your companion</div><div class="bond-row" style="margin-top:8px"><strong id="pet-bond">0 / 100</strong><div class="bond-track" role="progressbar" aria-label="Bond level" aria-valuemin="0" aria-valuemax="100" aria-valuenow="0" id="pet-bond-progress"><div id="pet-bond-fill" class="bond-fill"></div></div></div><div class="bond-stage"><strong id="pet-bond-stage" class="bond-stage-name">New companion</strong><span id="pet-bond-next" class="bond-stage-next">25 points to Familiar friend</span></div><div class="bond-actions" style="margin-top:12px"><button id="pet-interact" type="button">Give attention</button><span id="interaction-status" class="muted" role="status" aria-live="polite">Interact to build a lasting bond.</span></div></article>
</div></section>
<div class="grid">
 <article class="card"><div class="label">Lifetime work</div><div id="total-work" class="metric">—</div></article>
 <article class="card"><div class="label">Lifetime play</div><div id="total-play" class="metric">—</div></article>
 <article class="card"><div class="label">Lifetime nap</div><div id="total-nap" class="metric">—</div></article>
 <article class="card"><div class="label">Completed work sessions</div><div id="total-sessions" class="metric">—</div></article>
</div>
<section class="card section" aria-labelledby="backup-settings-heading">
<h2 id="backup-settings-heading">Settings backup &amp; restore</h2>
<p class="muted">Export or restore timer, quiet-hours, and reminder settings. Wi-Fi credentials and activity history are deliberately excluded from this file.</p>
<div style="display:flex;align-items:center;gap:12px;flex-wrap:wrap">
<button id="backup-download" type="button">Download settings backup</button>
<label style="display:grid;gap:6px;min-width:220px">Restore from JSON file<input id="backup-file" type="file" accept=".json,application/json"></label>
<button id="backup-restore" type="button" disabled>Validate &amp; restore</button>
</div>
<p id="backup-status" class="muted" role="status" aria-live="polite">No backup file selected.</p>
</section>
<section class="card section" aria-labelledby="timer-settings-heading">
<h2 id="timer-settings-heading">Focus cycle settings</h2>
<p class="muted">Customize the automatic work and rest cycle. Changes apply immediately and are saved on the ESP32.</p>
<div class="settings-grid">
<label>Work session (minutes)<input id="setting-work" type="number" min="1" max="180" value="25" inputmode="numeric"></label>
<label>Play break (minutes)<input id="setting-play" type="number" min="1" max="60" value="2" inputmode="numeric"></label>
<label>Long rest (minutes)<input id="setting-nap" type="number" min="1" max="120" value="5" inputmode="numeric"></label>
<label>Work sessions before long rest<input id="setting-cycles" type="number" min="1" max="8" value="3" inputmode="numeric"></label>
</div>
<div style="display:flex;align-items:center;gap:12px;flex-wrap:wrap;margin-top:14px"><button id="settings-save" type="button">Save settings</button><span id="settings-status" class="muted" role="status" aria-live="polite">Loading saved settings…</span></div>
</section>
<section class="card section" aria-labelledby="quiet-hours-heading">
<h2 id="quiet-hours-heading">Quiet hours</h2>
<p class="muted">Pause spontaneous pet messages and RTC reminders during your chosen hours. Uses the device's DS3231 clock.</p>
<div class="settings-grid">
<label style="display:flex;align-items:center;gap:10px"><input id="quiet-enabled" type="checkbox" checked> Enable quiet hours</label>
<label>Start hour (0–23)<input id="quiet-start" type="number" min="0" max="23" value="21" inputmode="numeric"></label>
<label>End hour (0–23)<input id="quiet-end" type="number" min="0" max="23" value="9" inputmode="numeric"></label>
</div>
<p class="muted">Hours are whole-hour boundaries; for example, 21 to 9 means 9 PM through 9 AM, including midnight.</p>
<div style="display:flex;align-items:center;gap:12px;flex-wrap:wrap;margin-top:14px"><button id="quiet-save" type="button">Save quiet hours</button><span id="quiet-status" class="muted" role="status" aria-live="polite">Loading saved settings…</span></div>
</section>
<section class="card section" aria-labelledby="reminder-settings-heading">
<h2 id="reminder-settings-heading">Reminder schedule</h2>
<p class="muted">Choose how often the DS3231 sends water and movement reminders outside quiet hours. Set an interval to 0 to disable that reminder.</p>
<div class="settings-grid">
<label>Drink-water reminder (minutes; 0 disables)<input id="reminder-water" type="number" min="0" max="240" value="60" inputmode="numeric"></label>
<label>Movement reminder (minutes; 0 disables)<input id="reminder-movement" type="number" min="0" max="180" value="45" inputmode="numeric"></label>
</div>
<p class="muted">Allowed intervals: water 15–240 minutes; movement 15–180 minutes, or 0 to disable.</p>
<div style="display:flex;align-items:center;gap:12px;flex-wrap:wrap;margin-top:14px"><button id="reminder-save" type="button">Save reminder schedule</button><span id="reminder-status" class="muted" role="status" aria-live="polite">Loading saved settings…</span></div>
</section>
<section class="card section" aria-labelledby="event-log-heading">
<h2 id="event-log-heading">Recent event log</h2>
<p class="muted">The latest 24 important events are retained across reboots. Frequent pet interactions are not logged to reduce flash writes.</p>
<div style="display:flex;align-items:center;gap:12px;flex-wrap:wrap"><button id="event-log-refresh" type="button">Refresh event log</button><span id="event-log-status" class="muted" role="status" aria-live="polite">Loading events…</span></div>
<div id="event-log-list" style="display:grid;gap:8px;margin-top:12px"><p class="muted">Loading…</p></div>
</section>
<section class="card section" aria-labelledby="diagnostics-heading">
<h2 id="diagnostics-heading">System diagnostics</h2>
<p class="muted">Live device health information. Values are read-only and are not stored in flash.</p>
<div class="grid">
<div class="card"><div class="label">Uptime</div><div id="diag-uptime" class="metric">—</div></div>
<div class="card"><div class="label">Free heap</div><div id="diag-heap" class="metric">—</div></div>
<div class="card"><div class="label">Minimum free heap</div><div id="diag-min-heap" class="metric">—</div></div>
<div class="card"><div class="label">Largest heap block</div><div id="diag-max-block" class="metric">—</div></div>
<div class="card"><div class="label">Free PSRAM</div><div id="diag-psram" class="metric">—</div></div>
<div class="card"><div class="label">Connected clients</div><div id="diag-clients" class="metric">—</div></div>
<div class="card"><div class="label">CPU frequency</div><div id="diag-cpu" class="metric">—</div></div>
<div class="card"><div class="label">Reset reason code</div><div id="diag-reset" class="metric">—</div></div>
</div>
<div style="display:flex;align-items:center;gap:12px;flex-wrap:wrap"><button id="diagnostics-refresh" type="button">Refresh diagnostics</button><span id="diagnostics-status" class="muted" role="status" aria-live="polite">Loading device health…</span></div>
</section>
<section class="card section" aria-labelledby="wifi-settings-heading">
<h2 id="wifi-settings-heading">Wi-Fi access point</h2>
<p class="muted">Set the network name and password for this device. Saving changes restarts the access point and disconnects this browser. Reconnect using the new credentials. Leave the password blank to keep the current password.</p>
<div class="settings-grid">
<label>Network name (SSID; 1–32 ASCII characters)<input id="wifi-ssid" type="text" minlength="1" maxlength="32" autocomplete="off" spellcheck="false" value="PashmakOS"></label>
<label>New password (8–63 characters; blank keeps current)<input id="wifi-password" type="password" minlength="8" maxlength="63" autocomplete="new-password" placeholder="Leave blank to keep current"></label>
</div>
<div style="display:flex;align-items:center;gap:12px;flex-wrap:wrap;margin-top:14px"><button id="wifi-save" type="button">Save and restart Wi-Fi</button><span id="wifi-status" class="muted" role="status" aria-live="polite">Loading access-point settings…</span></div>
</section>
<section class="card section" aria-labelledby="daily-goal-heading">
<h2 id="daily-goal-heading">Daily focus goal</h2>
<div class="goal-layout"><div><div id="daily-goal-count" class="metric">— / 4 sessions</div><div class="goal-progress" role="progressbar" aria-label="Daily focus goal progress" aria-valuemin="0" aria-valuemax="4" aria-valuenow="0" id="daily-goal-progress"><div id="daily-goal-fill" class="goal-progress-fill"></div></div><div id="daily-goal-status" class="goal-status" role="status" aria-live="polite">Waiting for activity data…</div></div><label for="daily-goal-target" class="label">Target sessions<br><input id="daily-goal-target" class="goal-number" type="number" min="1" max="20" value="4" inputmode="numeric"></label></div>
</section>
<section class="card section" aria-labelledby="focus-insights-heading">
<h2 id="focus-insights-heading">Focus insights</h2>
<p id="focus-insights-summary" class="insight-summary">Insights use the latest seven retained daily records on this device.</p>
<div class="insight-grid">
<article class="insight-card"><div class="label">Average daily focus</div><div id="insight-average" class="insight-value">—</div><div class="insight-caption">Across retained days in this period</div></article>
<article class="insight-card"><div class="label">Most focused day</div><div id="insight-best-day" class="insight-value">—</div><div id="insight-best-day-detail" class="insight-caption">No activity data yet</div></article>
<article class="insight-card"><div class="label">Days with focus</div><div id="insight-active-days" class="insight-value">—</div><div class="insight-caption">Days with at least one millisecond of work time</div></article>
<article class="insight-card"><div class="label">Sessions completed</div><div id="insight-sessions" class="insight-value">—</div><div class="insight-caption">Across the same period</div></article>
</div>
</section>
<section class="card section" aria-labelledby="goal-streak-heading">
<h2 id="goal-streak-heading">Daily goal streak</h2>
<div class="goal-layout">
<div><div id="goal-streak-count" class="metric">— days</div><div id="goal-streak-status" class="goal-status" role="status" aria-live="polite">Waiting for activity data…</div></div>
<div class="insight-card" style="min-width:150px"><div class="label">Daily target</div><div id="goal-streak-target" class="insight-value">4 sessions</div><div class="insight-caption">Uses your configured daily goal</div></div>
</div>
<p class="muted">Counts consecutive retained calendar days, ending at the latest recorded day, that met your session target. Up to 14 days are available.</p>
</section>
<section class="card section" aria-labelledby="achievements-heading"><h2 id="achievements-heading">Achievements</h2><p id="achievement-summary" class="muted">Achievements unlock as you build your focus habit.</p><div id="achievement-grid" class="achievement-grid" aria-live="polite"></div></section>
<section class="card section"><h2>Recent activity</h2><div class="pills"><button data-days="1">Today / latest day</button><button data-days="7">7 days</button><button data-days="14">14 days</button></div>
<div class="grid">
 <article><div class="label">Work</div><div id="recent-work" class="metric">—</div></article>
 <article><div class="label">Play</div><div id="recent-play" class="metric">—</div></article>
 <article><div class="label">Nap</div><div id="recent-nap" class="metric">—</div></article>
 <article><div class="label">Completed sessions</div><div id="recent-sessions" class="metric">—</div></article>
</div><div id="recent-note" class="muted"></div></section>
<section class="card section"><h2>Activity trend</h2><div class="trend-legend"><span><i class="legend-dot trend-work"></i>Work</span><span><i class="legend-dot trend-play"></i>Play</span><span><i class="legend-dot trend-nap"></i>Nap</span></div><div id="activity-trend" class="trend-list" role="region" aria-label="Daily activity trend for work, play, and nap"><div class="chart-empty">Waiting for activity data…</div></div></section>
<section class="card section"><div style="display:flex;align-items:center;justify-content:space-between;gap:12px;flex-wrap:wrap"><h2>Daily history</h2><button id="export-daily" type="button">Export CSV</button></div><div class="table-wrap"><table><thead><tr><th>Date</th><th>Work</th><th>Play</th><th>Nap</th><th>Sessions</th></tr></thead><tbody id="daily"></tbody></table></div></section>
<section class="card section"><div style="display:flex;align-items:center;justify-content:space-between;gap:12px;flex-wrap:wrap"><h2>Monthly history</h2><button id="export-monthly" type="button">Export CSV</button></div><div class="table-wrap"><table><thead><tr><th>Month</th><th>Work</th><th>Play</th><th>Nap</th><th>Sessions</th></tr></thead><tbody id="monthly"></tbody></table></div></section>
<footer>Data is read from the ESP32's local statistics API. Keep this page open while connected to the PashmakOS Wi-Fi network.</footer>
</main>
<script>
const $=id=>document.getElementById(id);let snapshot=null;let timerSettingsSnapshot=null;let quietHoursSnapshot=null;let reminderSettingsSnapshot=null;let selectedDays=7;let refreshInFlight=false;let lastUpdatedAt=0;let dailySessionGoal=4;try{const savedGoal=Number(localStorage.getItem('pashmakDailySessionGoal'));if(Number.isInteger(savedGoal)&&savedGoal>=1&&savedGoal<=20)dailySessionGoal=savedGoal}catch(e){}
const duration=ms=>{const mins=Math.floor(Number(ms||0)/60000);const d=Math.floor(mins/1440),h=Math.floor((mins%1440)/60),m=mins%60;return d?d+'d '+h+'h':h?h+'h '+m+'m':m+'m'};
const dateLabel=n=>{const s=String(n);return s.length===8?s.slice(0,4)+'-'+s.slice(4,6)+'-'+s.slice(6,8):s};
const monthLabel=n=>{const s=String(n);return s.length===6?s.slice(0,4)+'-'+s.slice(4,6):s};
function metric(id,value){$(id).textContent=value}
function renderAchievements(data){const totals=data.totals||{};const sessions=Number(totals.completedWorkSessions||0);const records=Array.isArray(data.dailyHistory)?data.dailyHistory:[];const latest=records.length?records[records.length-1]:null;const todaySessions=latest?Number(latest.completedWorkSessions||0):0;const goalReached=todaySessions>=dailySessionGoal;const work=Number(totals.workMs||0),play=Number(totals.playMs||0),nap=Number(totals.napMs||0);const definitions=[{id:'first-session',icon:'🌱',title:'First Focus',description:'Complete your first work session.',unlocked:sessions>=1,progress:sessions>=1?'1 / 1':'0 / 1'},{id:'five-sessions',icon:'🧩',title:'Finding Your Rhythm',description:'Complete 5 work sessions.',unlocked:sessions>=5,progress:Math.min(sessions,5)+' / 5'},{id:'twenty-five-sessions',icon:'🔥',title:'Focus Builder',description:'Complete 25 work sessions.',unlocked:sessions>=25,progress:Math.min(sessions,25)+' / 25'},{id:'hundred-sessions',icon:'🏆',title:'Focus Master',description:'Complete 100 work sessions.',unlocked:sessions>=100,progress:Math.min(sessions,100)+' / 100'},{id:'daily-goal',icon:'🎯',title:'Goal Getter',description:'Reach your daily session goal.',unlocked:goalReached,progress:goalReached?dailySessionGoal+' / '+dailySessionGoal:Math.min(todaySessions,dailySessionGoal)+' / '+dailySessionGoal},{id:'balanced-day',icon:'🌈',title:'Balanced Companion',description:'Record work, play, and nap time.',unlocked:work>0&&play>0&&nap>0,progress:work>0&&play>0&&nap>0?'All activity types recorded':'Keep exploring each activity'}];const grid=$('achievement-grid');grid.replaceChildren();definitions.forEach(a=>{const card=document.createElement('article');card.className='achievement '+(a.unlocked?'unlocked':'locked');const icon=document.createElement('div');icon.className='achievement-icon';icon.textContent=a.icon;icon.setAttribute('aria-hidden','true');const copy=document.createElement('div');copy.className='achievement-copy';const title=document.createElement('div');title.className='achievement-title';title.textContent=a.title;const description=document.createElement('div');description.className='achievement-description';description.textContent=a.description;const state=document.createElement('div');state.className='achievement-state';state.textContent=a.unlocked?'UNLOCKED':a.progress;copy.append(title,description,state);card.append(icon,copy);grid.appendChild(card)});const unlocked=definitions.filter(a=>a.unlocked).length;metric('achievement-summary',unlocked+' of '+definitions.length+' achievements unlocked. Keep going to collect them all.')}
function renderDailyGoal(data){const records=Array.isArray(data.dailyHistory)?data.dailyHistory:[];const latest=records.length?records[records.length-1]:null;const completed=latest?Number(latest.completedWorkSessions||0):0;const goal=dailySessionGoal;const progress=Math.min(100,Math.round(completed/goal*100));$('daily-goal-target').value=String(goal);metric('daily-goal-count',completed+' / '+goal+' sessions');$('daily-goal-fill').style.width=progress+'%';const bar=$('daily-goal-progress');bar.setAttribute('aria-valuemax',String(goal));bar.setAttribute('aria-valuenow',String(Math.min(completed,goal)));if(!latest){$('daily-goal-status').textContent='No daily activity recorded yet.';return}const date=dateLabel(latest.dateKey);$('daily-goal-status').textContent=completed>=goal?'Goal completed for '+date+' — great work!':(goal-completed)+' more completed session'+(goal-completed===1?'':'s')+' to reach the goal for '+date+'.'}
function saveDailyGoal(){const value=Number($('daily-goal-target').value);if(!Number.isInteger(value)||value<1||value>20){$('daily-goal-status').textContent='Choose a goal between 1 and 20 sessions.';$('daily-goal-target').value=String(dailySessionGoal);return}dailySessionGoal=value;try{localStorage.setItem('pashmakDailySessionGoal',String(value))}catch(e){}if(snapshot)renderDailyGoal(snapshot);else $('daily-goal-status').textContent='Goal set. Waiting for activity data.'}
function downloadCsv(filename,headers,records){const quote=value=>{let text=String(value??'');if(/^[\s\u0000-\u001f]*[=+@-]/.test(text))text="'"+text;return '"'+text.replace(/"/g,'""')+'"'};const csv=[headers,...records].map(record=>record.map(quote).join(',')).join('\r\n');const blob=new Blob(['\ufeff',csv],{type:'text/csv;charset=utf-8;'});const url=URL.createObjectURL(blob);const link=document.createElement('a');link.href=url;link.download=filename;document.body.appendChild(link);link.click();link.remove();setTimeout(()=>URL.revokeObjectURL(url),1000)}
function downloadJsonExport(){if(!snapshot)return;const payload={schemaVersion:1,exportedAt:new Date().toISOString(),source:'PashmakOS local dashboard',dailySessionGoal,settings:{timer:timerSettingsSnapshot,quietHours:quietHoursSnapshot,reminders:reminderSettingsSnapshot},statistics:snapshot};const blob=new Blob([JSON.stringify(payload,null,2)],{type:'application/json;charset=utf-8'});const url=URL.createObjectURL(blob);const link=document.createElement('a');link.href=url;link.download='pashmakos-export-'+new Date().toISOString().replace(/[:.]/g,'-')+'.json';document.body.appendChild(link);link.click();link.remove();setTimeout(()=>URL.revokeObjectURL(url),1000)}
function row(values){const tr=document.createElement('tr');values.forEach(v=>{const td=document.createElement('td');td.textContent=v;tr.appendChild(td)});return tr}
const petProfiles={0:{emoji:'🐶',tag:'Classic companion',color:'#c6a7ff',tint:'#4b3b67'},1:{emoji:'🐾',tag:'Soft & cozy',color:'#8fe0d0',tint:'#244d4b'},2:{emoji:'✨',tag:'Pixel buddy',color:'#ffbf86',tint:'#5b4030'}};
function renderGoalStreak(data){
 const records=Array.isArray(data.dailyHistory)?data.dailyHistory:[];
 const goal=dailySessionGoal;
 let streak=0;
 let previousDay=null;
 for(let i=records.length-1;i>=0;i--){
  const record=records[i];
  const key=String(record.dateKey);
  const date=new Date(Date.UTC(Number(key.slice(0,4)),Number(key.slice(4,6))-1,Number(key.slice(6,8))));
  const day=date.getTime();
  if(previousDay!==null&&previousDay-day!==86400000)break;
  if(Number(record.completedWorkSessions||0)<goal)break;
  streak++;
  previousDay=day;
 }
 metric('goal-streak-count',streak+' day'+(streak===1?'':'s'));
 metric('goal-streak-target',goal+' session'+(goal===1?'':'s'));
 const latest=records.length?records[records.length-1]:null;
 if(!latest){
  $('goal-streak-status').textContent='No daily history recorded yet.';
 }else if(streak>0){
  $('goal-streak-status').textContent='Goal met on '+streak+' consecutive recorded day'+(streak===1?'':'s')+', ending '+dateLabel(latest.dateKey)+'.';
 }else{
  $('goal-streak-status').textContent='The latest recorded day has not met the '+goal+'-session target yet.';
 }
}
function renderFocusInsights(data){
 const records=(Array.isArray(data.dailyHistory)?data.dailyHistory:[]).slice(-7);
 const totalWork=records.reduce((sum,d)=>sum+Number(d.workMs||0),0);
 const totalSessions=records.reduce((sum,d)=>sum+Number(d.completedWorkSessions||0),0);
 const activeDays=records.filter(d=>Number(d.workMs||0)>0).length;
 const best=records.reduce((winner,d)=>!winner||Number(d.workMs||0)>Number(winner.workMs||0)?d:winner,null);
 $('insight-average').textContent=records.length?duration(Math.round(totalWork/records.length)):'—';
 $('insight-best-day').textContent=best&&Number(best.workMs)>0?dateLabel(best.dateKey):'—';
 $('insight-best-day-detail').textContent=best&&Number(best.workMs)>0?duration(best.workMs)+' of focus':'No focus recorded yet';
 $('insight-active-days').textContent=records.length?activeDays+' / '+records.length:'—';
 $('insight-sessions').textContent=records.length?totalSessions.toLocaleString():'—';
 $('focus-insights-summary').textContent=records.length?'Based on '+records.length+' latest retained day(s). Insights are calculated locally from device history.': 'No daily history is available yet. Insights will appear after activity is recorded.';
}
function bondTier(value){const bond=Math.max(0,Math.min(100,Number(value)||0));if(bond>=75)return {name:'Best friend',next:null,remaining:0};if(bond>=50)return {name:'Close companion',next:'Best friend',remaining:75-bond};if(bond>=25)return {name:'Familiar friend',next:'Close companion',remaining:50-bond};return {name:'New companion',next:'Familiar friend',remaining:25-bond}}
function render(data){snapshot=data;$('export-json').disabled=false;renderDailyGoal(data);renderGoalStreak(data);renderAchievements(data);renderFocusInsights(data);const pets=Array.isArray(data.pets)?data.pets:[];const picker=$('pet-picker');picker.replaceChildren();pets.forEach(p=>{const id=Number(p.id),profile=petProfiles[id]||{emoji:'🐾',tag:'Companion',color:'#b9a0ff',tint:'#39314d'},active=Number(p.id)===Number(data.activePetId);const b=document.createElement('button');b.type='button';b.className='pet-option';b.style.setProperty('--pet-color',profile.color);b.style.setProperty('--pet-tint',profile.tint);b.setAttribute('aria-pressed',String(active));b.setAttribute('aria-label','Select '+p.name+(active?', currently active':''));const avatar=document.createElement('span');avatar.className='pet-avatar';avatar.setAttribute('aria-hidden','true');avatar.textContent=profile.emoji;const copy=document.createElement('span');copy.className='pet-option-copy';const name=document.createElement('span');name.className='pet-option-name';name.textContent=p.name;const tag=document.createElement('span');tag.className='pet-option-meta';tag.textContent=profile.tag;const energy=Math.max(0,Math.min(100,Number(p.energy)||0));const mood=String(p.mood||'unknown').replace(/\b\w/g,ch=>ch.toUpperCase());const stateLine=document.createElement('span');stateLine.className='pet-option-state';stateLine.textContent='Mood · '+mood;const energyLabel=document.createElement('span');energyLabel.className='pet-option-energy';energyLabel.textContent='Energy · '+energy+'%';const energyTrack=document.createElement('span');energyTrack.className='pet-option-energy-track';energyTrack.setAttribute('aria-hidden','true');const energyFill=document.createElement('span');energyFill.className='pet-option-energy-fill';energyFill.style.width=energy+'%';energyTrack.appendChild(energyFill);const bond=Math.max(0,Math.min(100,Number(p.bond)||0));const tier=bondTier(bond);const bondLabel=document.createElement('span');bondLabel.className='pet-option-bond';bondLabel.textContent=tier.name+' · '+bond+'/100';const bondTrack=document.createElement('span');bondTrack.className='pet-option-bond-track';bondTrack.setAttribute('aria-hidden','true');const bondFill=document.createElement('span');bondFill.className='pet-option-bond-fill';bondFill.style.width=bond+'%';bondTrack.appendChild(bondFill);copy.append(name,tag,stateLine,energyLabel,energyTrack,bondLabel,bondTrack);b.setAttribute('aria-label','Select '+p.name+(active?', currently active':'')+' · energy '+energy+' percent · mood '+mood+' · '+tier.name+' · bond '+bond+' of 100');const state=document.createElement('span');state.className='pet-selected';state.textContent=active?'ACTIVE':'SELECT';b.append(avatar,copy,state);b.addEventListener('click',()=>selectPet(id));picker.appendChild(b)});const active=pets.find(p=>Number(p.id)===Number(data.activePetId));$('pet-heading').textContent='Pet status'+(active?' · '+active.name:'');const pet=data.pet||{};const energy=Math.max(0,Math.min(100,Number(pet.energy||0)));metric('pet-energy',energy+'%');metric('pet-mood',String(pet.mood||'unknown').replace(/\b\w/g,ch=>ch.toUpperCase()));metric('pet-message',pet.message||'No message yet.');$('pet-energy-bar').style.width=energy+'%';const bond=Math.max(0,Math.min(100,Number(pet.bond||0)));metric('pet-bond',bond+' / 100');$('pet-bond-fill').style.width=bond+'%';$('pet-bond-progress').setAttribute('aria-valuenow',String(bond));const tier=bondTier(bond);$('pet-bond-stage').textContent=tier.name;$('pet-bond-next').textContent=tier.next?(tier.remaining+' point'+(tier.remaining===1?'':'s')+' to '+tier.next):'Maximum bond reached';const t=data.totals||{};metric('total-work',duration(t.workMs));metric('total-play',duration(t.playMs));metric('total-nap',duration(t.napMs));metric('total-sessions',Number(t.completedWorkSessions||0).toLocaleString());
const days=(data.dailyHistory||[]).slice(-selectedDays);const recent=days.reduce((a,d)=>{a.workMs+=Number(d.workMs||0);a.playMs+=Number(d.playMs||0);a.napMs+=Number(d.napMs||0);a.completedWorkSessions+=Number(d.completedWorkSessions||0);return a},{workMs:0,playMs:0,napMs:0,completedWorkSessions:0});
metric('recent-work',duration(recent.workMs));metric('recent-play',duration(recent.playMs));metric('recent-nap',duration(recent.napMs));metric('recent-sessions',recent.completedWorkSessions.toLocaleString());$('recent-note').textContent='Showing '+days.length+' retained day(s).';
const trend=$('activity-trend');trend.replaceChildren();const maxActivity=Math.max(1,...days.map(d=>Math.max(Number(d.workMs||0),Number(d.playMs||0),Number(d.napMs||0))));if(!days.length){const empty=document.createElement('div');empty.className='chart-empty';empty.textContent='No activity recorded for this period yet.';trend.appendChild(empty)}else{days.forEach(d=>{const item=document.createElement('div');item.className='trend-row';const date=document.createElement('div');date.className='trend-date';date.textContent=dateLabel(d.dateKey);const bars=document.createElement('div');bars.className='trend-bars';[['workMs','trend-work','Work'],['playMs','trend-play','Play'],['napMs','trend-nap','Nap']].forEach(([key,cls,label])=>{const track=document.createElement('div');track.className='trend-track';track.title=label+': '+duration(d[key]);track.setAttribute('aria-label',label+': '+duration(d[key]));const fill=document.createElement('div');fill.className='trend-fill '+cls;fill.style.width=Math.max(0,Math.min(100,Number(d[key]||0)/maxActivity*100))+'%';track.appendChild(fill);bars.appendChild(track)});item.append(date,bars);trend.appendChild(item)})}
const daily=$('daily');daily.replaceChildren();(data.dailyHistory||[]).slice().reverse().forEach(d=>daily.appendChild(row([dateLabel(d.dateKey),duration(d.workMs),duration(d.playMs),duration(d.napMs),d.completedWorkSessions])));
if(!daily.children.length)daily.appendChild(row(['No daily records yet','—','—','—','—']));
const monthly=$('monthly');monthly.replaceChildren();(data.monthlyHistory||[]).slice().reverse().forEach(m=>monthly.appendChild(row([monthLabel(m.monthKey),duration(m.workMs),duration(m.playMs),duration(m.napMs),m.completedWorkSessions])));
if(!monthly.children.length)monthly.appendChild(row(['No monthly records yet','—','—','—','—']));
$('export-daily').disabled=!(data.dailyHistory||[]).length;$('export-monthly').disabled=!(data.monthlyHistory||[]).length;}
async function loadQuietHours(){try{const response=await fetch('/api/quiet-hours',{cache:'no-store'});if(!response.ok)throw new Error('HTTP '+response.status);const settings=await response.json();if(typeof settings.enabled!=='boolean'||!Number.isInteger(settings.startHour)||settings.startHour<0||settings.startHour>23||!Number.isInteger(settings.endHour)||settings.endHour<0||settings.endHour>23||settings.startHour===settings.endHour)throw new Error('Invalid quiet-hours payload');quietHoursSnapshot={enabled:settings.enabled,startHour:settings.startHour,endHour:settings.endHour};$('quiet-enabled').checked=settings.enabled;$('quiet-start').value=settings.startHour;$('quiet-end').value=settings.endHour;$('quiet-status').textContent='Loaded from device.'}catch(e){$('quiet-status').textContent='Could not load quiet hours · '+(e.message||'connection error')}}
async function saveQuietHours(){const values={enabled:$('quiet-enabled').checked?1:0,startHour:Number($('quiet-start').value),endHour:Number($('quiet-end').value)};if(!Number.isInteger(values.startHour)||values.startHour<0||values.startHour>23||!Number.isInteger(values.endHour)||values.endHour<0||values.endHour>23||values.startHour===values.endHour){$('quiet-status').textContent='Choose different whole hours between 0 and 23.';return}const button=$('quiet-save');button.disabled=true;$('quiet-status').textContent='Saving…';try{const response=await fetch('/api/quiet-hours',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(values)});if(!response.ok)throw new Error('HTTP '+response.status);quietHoursSnapshot={enabled:Boolean(values.enabled),startHour:values.startHour,endHour:values.endHour};$('quiet-status').textContent=values.enabled?'Saved · quiet hours are active immediately.':'Saved · quiet hours are disabled.'}catch(e){$('quiet-status').textContent='Save failed · '+(e.message||'connection error')}finally{button.disabled=false}}
let selectedBackupFile=null;
function validBackupSettings(data){if(!data||typeof data!=='object'||data.schemaVersion!==1||!data.settings||typeof data.settings!=='object')return false;const s=data.settings,t=s.timer,q=s.quietHours,r=s.reminders;return !!t&&Number.isInteger(t.workMinutes)&&t.workMinutes>=1&&t.workMinutes<=180&&Number.isInteger(t.playMinutes)&&t.playMinutes>=1&&t.playMinutes<=60&&Number.isInteger(t.napMinutes)&&t.napMinutes>=1&&t.napMinutes<=120&&Number.isInteger(t.workCyclesBeforeLongRest)&&t.workCyclesBeforeLongRest>=1&&t.workCyclesBeforeLongRest<=8&&!!q&&typeof q.enabled==='boolean'&&Number.isInteger(q.startHour)&&q.startHour>=0&&q.startHour<=23&&Number.isInteger(q.endHour)&&q.endHour>=0&&q.endHour<=23&&q.startHour!==q.endHour&&!!r&&Number.isInteger(r.waterMinutes)&&(r.waterMinutes===0||(r.waterMinutes>=15&&r.waterMinutes<=240))&&Number.isInteger(r.movementMinutes)&&(r.movementMinutes===0||(r.movementMinutes>=15&&r.movementMinutes<=180))}
async function fetchSettings(path){const response=await fetch(path,{cache:'no-store'});if(!response.ok)throw new Error(path+' returned HTTP '+response.status);return response.json()}
async function postSettings(path,values){const response=await fetch(path,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(values)});if(!response.ok)throw new Error(path+' returned HTTP '+response.status)}
async function downloadSettingsBackup(){const button=$('backup-download');button.disabled=true;$('backup-status').textContent='Collecting saved settings…';try{const [timer,quietHours,reminders]=await Promise.all([fetchSettings('/api/timer-settings'),fetchSettings('/api/quiet-hours'),fetchSettings('/api/reminder-settings')]);const backup={format:'PashmakOS-settings-backup',schemaVersion:1,createdAt:new Date().toISOString(),settings:{timer,quietHours,reminders}};if(!validBackupSettings({schemaVersion:backup.schemaVersion,settings:backup.settings}))throw new Error('Device returned invalid settings');const blob=new Blob([JSON.stringify(backup,null,2)],{type:'application/json'});const url=URL.createObjectURL(blob);const link=document.createElement('a');link.href=url;link.download='pashmakos-settings-backup.json';document.body.appendChild(link);link.click();link.remove();URL.revokeObjectURL(url);$('backup-status').textContent='Backup downloaded. Wi-Fi credentials and activity history are not included.'}catch(e){$('backup-status').textContent='Backup failed · '+(e.message||'connection error')}finally{button.disabled=false}}
async function restoreSettingsBackup(){if(!selectedBackupFile)return;const button=$('backup-restore');button.disabled=true;$('backup-status').textContent='Validating backup file…';try{if(selectedBackupFile.size>16384)throw new Error('File is too large (maximum 16 KB)');const backup=JSON.parse(await selectedBackupFile.text());if(backup.format!=='PashmakOS-settings-backup'||!validBackupSettings(backup))throw new Error('Unsupported backup or invalid setting values');if(!window.confirm('Restore timer, quiet-hours, and reminder settings from this file? Existing settings will be overwritten.'))return;const s=backup.settings;$('backup-status').textContent='Restoring settings…';await postSettings('/api/timer-settings',s.timer);await postSettings('/api/quiet-hours',{enabled:s.quietHours.enabled?1:0,startHour:s.quietHours.startHour,endHour:s.quietHours.endHour});await postSettings('/api/reminder-settings',s.reminders);await Promise.all([loadTimerSettings(),loadQuietHours(),loadReminderSettings()]);$('backup-status').textContent='Restore complete. Timer, quiet-hours, and reminder settings were saved.'}catch(e){$('backup-status').textContent='Restore failed; some earlier settings may already have been restored. '+(e.message||'invalid backup')}finally{button.disabled=!selectedBackupFile}}
const EVENT_NAMES={1:'Device boot',2:'Wi-Fi access point failed to start',3:'Pet selected',4:'Timer settings saved',5:'Quiet hours saved',6:'Reminder settings saved',7:'Wi-Fi settings saved'};
async function loadEventLog(){const button=$('event-log-refresh');button.disabled=true;$('event-log-status').textContent='Loading event history…';try{const response=await fetch('/api/events',{cache:'no-store'});if(!response.ok)throw new Error('HTTP '+response.status);const data=await response.json();if(!Array.isArray(data.events))throw new Error('Invalid event log response');const list=$('event-log-list');list.replaceChildren();if(data.events.length===0){const empty=document.createElement('p');empty.className='muted';empty.textContent='No events recorded yet.';list.appendChild(empty)}for(const item of data.events){if(!Number.isInteger(item.sequence)||!Number.isInteger(item.uptimeSeconds)||!Number.isInteger(item.code)||!Number.isInteger(item.detail))continue;const row=document.createElement('div');row.className='card';row.style.padding='10px 12px';const title=document.createElement('strong');title.textContent=EVENT_NAMES[item.code]||'Unknown event';const meta=document.createElement('div');meta.className='muted';meta.textContent='#'+item.sequence+' · uptime '+formatUptime(item.uptimeSeconds)+(item.code===3?' · pet '+item.detail:'');row.append(title,meta);list.appendChild(row)}$('event-log-status').textContent=data.events.length+' event(s) · newest first'}catch(e){$('event-log-status').textContent='Event log unavailable · '+(e.message||'connection error')}finally{button.disabled=false}}
function formatBytes(value){if(!Number.isFinite(value)||value<0)return '—';if(value<1024)return value+' B';return (value/1024).toFixed(1)+' KB'}
function formatUptime(seconds){if(!Number.isFinite(seconds)||seconds<0)return '—';const days=Math.floor(seconds/86400);const hours=Math.floor(seconds%86400/3600);const minutes=Math.floor(seconds%3600/60);const secs=Math.floor(seconds%60);return (days?days+'d ':'')+(hours?hours+'h ':'')+(minutes?minutes+'m ':'')+secs+'s'}
async function loadDiagnostics(){const button=$('diagnostics-refresh');button.disabled=true;$('diagnostics-status').textContent='Reading device health…';try{const response=await fetch('/api/diagnostics',{cache:'no-store'});if(!response.ok)throw new Error('HTTP '+response.status);const d=await response.json();for(const key of ['uptimeSeconds','freeHeapBytes','minFreeHeapBytes','maxAllocHeapBytes','freePsramBytes','wifiClients','cpuMHz','resetReasonCode'])if(!Number.isFinite(d[key])||d[key]<0)throw new Error('Invalid diagnostics payload');$('diag-uptime').textContent=formatUptime(d.uptimeSeconds);$('diag-heap').textContent=formatBytes(d.freeHeapBytes);$('diag-min-heap').textContent=formatBytes(d.minFreeHeapBytes);$('diag-max-block').textContent=formatBytes(d.maxAllocHeapBytes);$('diag-psram').textContent=formatBytes(d.freePsramBytes);$('diag-clients').textContent=String(d.wifiClients);$('diag-cpu').textContent=d.cpuMHz+' MHz';$('diag-reset').textContent=String(d.resetReasonCode);$('diagnostics-status').textContent='Updated just now.'}catch(e){$('diagnostics-status').textContent='Diagnostics unavailable · '+(e.message||'connection error')}finally{button.disabled=false}}
async function loadWifiSettings(){try{const response=await fetch('/api/wifi-settings',{cache:'no-store'});if(!response.ok)throw new Error('HTTP '+response.status);const settings=await response.json();if(typeof settings.ssid!=='string'||settings.ssid.length<1||settings.ssid.length>32||settings.passwordConfigured!==true)throw new Error('Invalid Wi-Fi settings payload');$('wifi-ssid').value=settings.ssid;$('wifi-status').textContent='Loaded from device. Password is not exposed by the API.'}catch(e){$('wifi-status').textContent='Could not load Wi-Fi settings · '+(e.message||'connection error')}}
async function saveWifiSettings(){const ssid=$('wifi-ssid').value;const password=$('wifi-password').value;if(ssid.length<1||ssid.length>32||/[^\x20-\x7e]|["\\]/.test(ssid)){$('wifi-status').textContent='SSID must use 1–32 printable ASCII characters and cannot contain quotes or backslashes.';return}if(password.length>0&&(password.length<8||password.length>63||/[^\x20-\x7e]/.test(password))){$('wifi-status').textContent='Password must be 8–63 printable ASCII characters, or blank to keep the current password.';return}const button=$('wifi-save');button.disabled=true;$('wifi-status').textContent='Saving settings and restarting the access point…';try{const response=await fetch('/api/wifi-settings',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({ssid,password})});if(!response.ok){const result=await response.json().catch(()=>({}));throw new Error(result.error||('HTTP '+response.status))}$('wifi-password').value='';$('wifi-status').textContent='Saved. Wi-Fi is restarting; reconnect using the configured SSID and password.'}catch(e){$('wifi-status').textContent='Save may have disconnected the browser. Reconnect to the configured network and reload if needed. '+(e.message||'')}finally{button.disabled=false}}
async function loadReminderSettings(){try{const response=await fetch('/api/reminder-settings',{cache:'no-store'});if(!response.ok)throw new Error('HTTP '+response.status);const settings=await response.json();if(!Number.isInteger(settings.waterMinutes)||(settings.waterMinutes!==0&&(settings.waterMinutes<15||settings.waterMinutes>240))||!Number.isInteger(settings.movementMinutes)||(settings.movementMinutes!==0&&(settings.movementMinutes<15||settings.movementMinutes>180)))throw new Error('Invalid reminder settings payload');reminderSettingsSnapshot={waterMinutes:settings.waterMinutes,movementMinutes:settings.movementMinutes};$('reminder-water').value=settings.waterMinutes;$('reminder-movement').value=settings.movementMinutes;$('reminder-status').textContent='Loaded from device.'}catch(e){$('reminder-status').textContent='Could not load reminder settings · '+(e.message||'connection error')}}
async function saveReminderSettings(){const values={waterMinutes:Number($('reminder-water').value),movementMinutes:Number($('reminder-movement').value)};if(!Number.isInteger(values.waterMinutes)||(values.waterMinutes!==0&&(values.waterMinutes<15||values.waterMinutes>240))||!Number.isInteger(values.movementMinutes)||(values.movementMinutes!==0&&(values.movementMinutes<15||values.movementMinutes>180))){$('reminder-status').textContent='Water: 0 or 15–240 min. Movement: 0 or 15–180 min.';return}const button=$('reminder-save');button.disabled=true;$('reminder-status').textContent='Saving…';try{const response=await fetch('/api/reminder-settings',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(values)});if(!response.ok)throw new Error('HTTP '+response.status);reminderSettingsSnapshot={...values};$('reminder-status').textContent='Saved · new intervals are active immediately.'}catch(e){$('reminder-status').textContent='Save failed · '+(e.message||'connection error')}finally{button.disabled=false}}
async function loadTimerSettings(){try{const response=await fetch('/api/timer-settings',{cache:'no-store'});if(!response.ok)throw new Error('HTTP '+response.status);const settings=await response.json();if(!Number.isInteger(settings.workMinutes)||settings.workMinutes<1||settings.workMinutes>180||!Number.isInteger(settings.playMinutes)||settings.playMinutes<1||settings.playMinutes>60||!Number.isInteger(settings.napMinutes)||settings.napMinutes<1||settings.napMinutes>120||!Number.isInteger(settings.workCyclesBeforeLongRest)||settings.workCyclesBeforeLongRest<1||settings.workCyclesBeforeLongRest>8)throw new Error('Invalid settings payload');timerSettingsSnapshot={workMinutes:settings.workMinutes,playMinutes:settings.playMinutes,napMinutes:settings.napMinutes,workCyclesBeforeLongRest:settings.workCyclesBeforeLongRest};$('setting-work').value=settings.workMinutes;$('setting-play').value=settings.playMinutes;$('setting-nap').value=settings.napMinutes;$('setting-cycles').value=settings.workCyclesBeforeLongRest;$('settings-status').textContent='Loaded from device.'}catch(e){$('settings-status').textContent='Could not load settings. Defaults are shown; try refresh.'}}
async function saveTimerSettings(){const values={workMinutes:Number($('setting-work').value),playMinutes:Number($('setting-play').value),napMinutes:Number($('setting-nap').value),workCyclesBeforeLongRest:Number($('setting-cycles').value)};if(!Number.isInteger(values.workMinutes)||values.workMinutes<1||values.workMinutes>180||!Number.isInteger(values.playMinutes)||values.playMinutes<1||values.playMinutes>60||!Number.isInteger(values.napMinutes)||values.napMinutes<1||values.napMinutes>120||!Number.isInteger(values.workCyclesBeforeLongRest)||values.workCyclesBeforeLongRest<1||values.workCyclesBeforeLongRest>8){$('settings-status').textContent='Check the allowed ranges before saving.';return}const button=$('settings-save');button.disabled=true;$('settings-status').textContent='Saving…';try{const response=await fetch('/api/timer-settings',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(values)});if(!response.ok)throw new Error('HTTP '+response.status);timerSettingsSnapshot={...values};$('settings-status').textContent='Saved. New durations are active immediately.';await refresh()}catch(e){$('settings-status').textContent='Save failed · '+(e.message||'connection error')}finally{button.disabled=false}}
function isCounter(value){return value!==null&&value!==undefined&&value!==''&&Number.isFinite(Number(value))&&Number(value)>=0}
function isValidDateKey(value){const text=String(value);if(!/^\d{8}$/.test(text))return false;const y=Number(text.slice(0,4)),m=Number(text.slice(4,6)),d=Number(text.slice(6,8));const date=new Date(Date.UTC(y,m-1,d));return date.getUTCFullYear()===y&&date.getUTCMonth()===m-1&&date.getUTCDate()===d}
function isValidMonthKey(value){const text=String(value);return /^\d{6}$/.test(text)&&Number(text.slice(0,4))>=2000&&Number(text.slice(4,6))>=1&&Number(text.slice(4,6))<=12}
function isRecord(record,key,validator){return !!record&&validator(record[key])&&isCounter(record.workMs)&&isCounter(record.playMs)&&isCounter(record.napMs)&&isCounter(record.completedWorkSessions)}
function isValidStats(data){
 if(!data||typeof data!=='object'||data.version!==1)return false;
 if(!Number.isInteger(data.activePetId)||data.activePetId<0||data.activePetId>2)return false;
 if(!Array.isArray(data.pets)||data.pets.length!==3)return false;
 if(!data.pets.every((p,i)=>p&&Number(p.id)===i&&typeof p.name==='string'&&typeof p.mood==='string'&&typeof p.message==='string'&&Number.isFinite(Number(p.energy))&&Number(p.energy)>=0&&Number(p.energy)<=100&&Number.isInteger(Number(p.bond))&&Number(p.bond)>=0&&Number(p.bond)<=100))return false;
 const selected=data.pets[data.activePetId];
 if(!data.pet||typeof data.pet.mood!=='string'||typeof data.pet.message!=='string'||!Number.isFinite(Number(data.pet.energy))||Number(data.pet.energy)<0||Number(data.pet.energy)>100||!Number.isInteger(Number(data.pet.bond))||Number(data.pet.bond)<0||Number(data.pet.bond)>100)return false;
 if(data.pet.mood!==selected.mood||data.pet.message!==selected.message||Number(data.pet.energy)!==Number(selected.energy)||Number(data.pet.bond)!==Number(selected.bond))return false;
 if(!data.totals||!['workMs','playMs','napMs','completedWorkSessions'].every(k=>isCounter(data.totals[k])))return false;
 if(!data.recent||!['requestedDays','daysIncluded','workMs','playMs','napMs','completedWorkSessions'].every(k=>isCounter(data.recent[k]))||data.recent.daysIncluded>data.recent.requestedDays)return false;
 if(!Array.isArray(data.dailyHistory)||data.dailyHistory.length>14||!data.dailyHistory.every(d=>isRecord(d,'dateKey',isValidDateKey)))return false;
 if(!Array.isArray(data.monthlyHistory)||data.monthlyHistory.length>12||!data.monthlyHistory.every(m=>isRecord(m,'monthKey',isValidMonthKey)))return false;
 return true;
}
let retryDelay=10000;let refreshTimer=null;let refreshRequested=false;
function scheduleRefresh(delay){if(refreshTimer!==null)clearTimeout(refreshTimer);refreshTimer=setTimeout(()=>{refreshTimer=null;refresh()},delay)}
async function interactWithPet(){const button=$('pet-interact');button.disabled=true;$('interaction-status').textContent='Waiting for your companion…';try{const response=await fetch('/api/pet/interact',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'action=interact'});const result=await response.json();if(response.status===429){$('interaction-status').textContent='Your pet needs a few seconds before another interaction.';return}if(!response.ok)throw new Error(result.error||('HTTP '+response.status));const bond=Math.max(0,Math.min(100,Number(result.bond)||0));const tier=bondTier(bond);$('interaction-status').textContent='Bond: '+bond+'/100 · '+tier.name+(tier.next?' · '+tier.remaining+' more to '+tier.next:' · maximum bond')+' · reaction shown on the TFT.';await refresh()}catch(e){$('interaction-status').textContent='Interaction failed · '+(e.message||'connection error')}finally{button.disabled=false}}
async function selectPet(id){try{const response=await fetch('/api/pet/select',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'id='+encodeURIComponent(id)});if(!response.ok)throw new Error('HTTP '+response.status);await refresh()}catch(e){$('connection').dataset.state=snapshot?'stale':'error';$('connection').textContent=snapshot?'Pet selection failed · showing last valid data':'Pet selection failed';}}
async function refresh(){if(refreshInFlight){refreshRequested=true;return}refreshInFlight=true;if(refreshTimer!==null){clearTimeout(refreshTimer);refreshTimer=null}$('refresh').disabled=true;const controller=new AbortController();const timeout=setTimeout(()=>controller.abort(),5000);try{const response=await fetch('/api/stats',{cache:'no-store',signal:controller.signal});if(!response.ok)throw new Error('HTTP '+response.status);const data=await response.json();if(!isValidStats(data))throw new Error('Invalid statistics payload');render(data);$('connection').dataset.state='connected';$('connection').textContent='Connected · updated '+new Date().toLocaleTimeString();lastUpdatedAt=Date.now();retryDelay=10000}catch(e){$('connection').dataset.state=snapshot?'stale':'error';$('connection').textContent=snapshot?'Stale data · last valid update '+new Date(lastUpdatedAt).toLocaleTimeString():'Connection unavailable · retrying';retryDelay=Math.min(retryDelay*2,60000)}finally{clearTimeout(timeout);refreshInFlight=false;$('refresh').disabled=false;const nextDelay=refreshRequested?250:retryDelay;refreshRequested=false;scheduleRefresh(nextDelay)}}
$('refresh').addEventListener('click',refresh);
$('backup-download').addEventListener('click',downloadSettingsBackup);
$('backup-file').addEventListener('change',event=>{selectedBackupFile=event.target.files&&event.target.files[0]||null;$('backup-restore').disabled=!selectedBackupFile;$('backup-status').textContent=selectedBackupFile?'Selected '+selectedBackupFile.name+'. Validate before restoring.':'No backup file selected.'});
$('backup-restore').addEventListener('click',restoreSettingsBackup);
$('export-json').addEventListener('click',downloadJsonExport);
$('pet-interact').addEventListener('click',interactWithPet);
$('settings-save').addEventListener('click',saveTimerSettings);
$('quiet-save').addEventListener('click',saveQuietHours);$('reminder-save').addEventListener('click',saveReminderSettings);
$('wifi-save').addEventListener('click',saveWifiSettings);
$('diagnostics-refresh').addEventListener('click',loadDiagnostics);
$('event-log-refresh').addEventListener('click',loadEventLog);
loadEventLog();
loadDiagnostics();
$('daily-goal-target').value=String(dailySessionGoal);$('daily-goal-target').addEventListener('change',saveDailyGoal);$('daily-goal-target').addEventListener('keydown',event=>{if(event.key==='Enter'){event.preventDefault();saveDailyGoal()}});
loadTimerSettings();
loadQuietHours();
loadReminderSettings();
loadWifiSettings();
$('export-daily').disabled=true;$('export-monthly').disabled=true;$('export-daily').addEventListener('click',()=>{if(!snapshot?.dailyHistory?.length)return;const records=(snapshot?.dailyHistory||[]).map(d=>[dateLabel(d.dateKey),d.workMs,d.playMs,d.napMs,d.completedWorkSessions]);downloadCsv('pashmak-daily-history.csv',['Date','Work ms','Play ms','Nap ms','Completed sessions'],records)});
$('export-monthly').addEventListener('click',()=>{if(!snapshot?.monthlyHistory?.length)return;const records=(snapshot?.monthlyHistory||[]).map(m=>[monthLabel(m.monthKey),m.workMs,m.playMs,m.napMs,m.completedWorkSessions]);downloadCsv('pashmak-monthly-history.csv',['Month','Work ms','Play ms','Nap ms','Completed sessions'],records)});
document.querySelectorAll('[data-days]').forEach(b=>b.addEventListener('click',()=>{selectedDays=Number(b.dataset.days);if(snapshot)render(snapshot)}));refresh();
</script></body></html>
)HTML";
}
void WebDashboard::begin(PashmakApp &app)
{
    _app = &app;
    // The TFT and ESP32 share USB power. Reduce Wi-Fi peak demand first;
    // this does not touch the display or pet-rendering code.
    WiFi.mode(WIFI_AP);
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    WiFi.setSleep(true);
    char apSsid[WIFI_AP_SSID_CAPACITY]{};
    char apPassword[WIFI_AP_PASSWORD_CAPACITY]{};
    loadAccessPointConfig(apSsid, apPassword);
    const bool apStarted = WiFi.softAP(apSsid, apPassword);
    if (!apStarted)
    {
        Serial.println("[WEB] Failed to start Wi-Fi access point.");
        appendEvent(EVENT_AP_START_FAILED);
        return;
    }
    appendEvent(EVENT_BOOT);
    _server.on("/", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/api/stats", HTTP_GET, [this]() { handleStats(); });
    _server.on("/api/pet/select", HTTP_POST, [this]() { handlePetSelection(); });
    _server.on("/api/pet/interact", HTTP_POST, [this]() { handlePetInteraction(); });
    _server.on("/api/timer-settings", HTTP_GET, [this]() { handleTimerSettingsGet(); });
    _server.on("/api/timer-settings", HTTP_POST, [this]() { handleTimerSettingsPost(); });
    _server.on("/api/quiet-hours", HTTP_GET, [this]() { handleQuietHoursGet(); });
    _server.on("/api/quiet-hours", HTTP_POST, [this]() { handleQuietHoursPost(); });
    _server.on("/api/reminder-settings", HTTP_GET, [this]() { handleReminderSettingsGet(); });
    _server.on("/api/reminder-settings", HTTP_POST, [this]() { handleReminderSettingsPost(); });
    _server.on("/api/events", HTTP_GET, [this]() { handleEventLog(); });
    _server.on("/api/diagnostics", HTTP_GET, [this]() { handleDiagnostics(); });
    _server.on("/api/wifi-settings", HTTP_GET, [this]() { handleWifiSettingsGet(); });
    _server.on("/api/wifi-settings", HTTP_POST, [this]() { handleWifiSettingsPost(); });
    _server.onNotFound([this]() { handleNotFound(); });
    _server.begin();
    _started = true;
    Serial.printf("[WEB] Wi-Fi SSID: %s\n", apSsid);
    Serial.printf("[WEB] Dashboard: http://%s/\n", WiFi.softAPIP().toString().c_str());
}
void WebDashboard::update()
{
    if (_started)
        _server.handleClient();
}
void WebDashboard::handleRoot()
{
    _server.send_P(200, "text/html; charset=utf-8", DASHBOARD_HTML);
}
void WebDashboard::handleStats()
{
    if (_app == nullptr)
    {
        _server.send(503, "application/json", "{\"error\":\"statistics unavailable\"}");
        return;
    }
    _server.sendHeader("Cache-Control", "no-store");
    _server.send(200, "application/json; charset=utf-8", _app->statisticsToJson(14));
}
void WebDashboard::handlePetSelection()
{
    if (_app == nullptr || !_server.hasArg("id"))
    {
        _server.send(400, "application/json", "{\"error\":\"missing pet id\"}");
        return;
    }
    const String value = _server.arg("id");
    if (value.length() != 1 || value[0] < '0' || value[0] > '2' ||
        !_app->setActivePet(static_cast<uint8_t>(value[0] - '0')))
    {
        _server.send(400, "application/json", "{\"error\":\"invalid pet id\"}");
        return;
    }
    appendEvent(EVENT_PET_SELECTED, _app->activePetId());
    _server.send(200, "application/json", "{\"ok\":true}");
}
void WebDashboard::handlePetInteraction()
{
    if (_app == nullptr)
    {
        _server.send(503, "application/json", "{\"error\":\"pet interaction unavailable\"}");
        return;
    }
    if (!_app->interactWithActivePet(millis()))
    {
        _server.sendHeader("Retry-After", "8");
        _server.send(429, "application/json", "{\"error\":\"interaction cooldown active\"}");
        return;
    }
    char response[64];
    snprintf(response, sizeof(response), "{\"ok\":true,\"bond\":%u}",
             _app->petBond(_app->activePetId()));
    _server.sendHeader("Cache-Control", "no-store");
    _server.send(200, "application/json", response);
}
void WebDashboard::handleTimerSettingsGet()
{
    if (_app == nullptr)
    {
        _server.send(503, "application/json", "{\"error\":\"settings unavailable\"}");
        return;
    }
    const PashmakApp::TimerSettings settings = _app->timerSettings();
    char json[128];
    snprintf(json, sizeof(json),
             "{\"workMinutes\":%u,\"playMinutes\":%u,\"napMinutes\":%u,\"workCyclesBeforeLongRest\":%u}",
             settings.workMinutes, settings.playMinutes, settings.napMinutes,
             settings.workCyclesBeforeLongRest);
    _server.sendHeader("Cache-Control", "no-store");
    _server.send(200, "application/json; charset=utf-8", json);
}
void WebDashboard::handleTimerSettingsPost()
{
    if (_app == nullptr)
    {
        _server.send(503, "application/json", "{\"error\":\"settings unavailable\"}");
        return;
    }
    const char *keys[] = {"workMinutes", "playMinutes", "napMinutes", "workCyclesBeforeLongRest"};
    uint32_t values[4]{};
    for (uint8_t i = 0; i < 4; ++i)
    {
        if (!_server.hasArg(keys[i]))
        {
            _server.send(400, "application/json", "{\"error\":\"missing setting\"}");
            return;
        }
        const String raw = _server.arg(keys[i]);
        if (raw.length() == 0 || raw.length() > 3)
        {
            _server.send(400, "application/json", "{\"error\":\"invalid setting\"}");
            return;
        }
        uint32_t parsed = 0;
        for (size_t j = 0; j < raw.length(); ++j)
        {
            if (raw[j] < '0' || raw[j] > '9')
            {
                _server.send(400, "application/json", "{\"error\":\"invalid setting\"}");
                return;
            }
            parsed = parsed * 10U + static_cast<uint32_t>(raw[j] - '0');
        }
        values[i] = parsed;
    }
    if (values[0] < 1 || values[0] > 180 ||
        values[1] < 1 || values[1] > 60 ||
        values[2] < 1 || values[2] > 120 ||
        values[3] < 1 || values[3] > 8)
    {
        _server.send(400, "application/json", "{\"error\":\"setting out of range\"}");
        return;
    }
    PashmakApp::TimerSettings settings;
    settings.workMinutes = static_cast<uint16_t>(values[0]);
    settings.playMinutes = static_cast<uint16_t>(values[1]);
    settings.napMinutes = static_cast<uint16_t>(values[2]);
    settings.workCyclesBeforeLongRest = static_cast<uint8_t>(values[3]);
    if (!_app->setTimerSettings(settings))
    {
        _server.send(500, "application/json", "{\"error\":\"could not persist settings\"}");
        return;
    }
    _server.sendHeader("Cache-Control", "no-store");
    appendEvent(EVENT_TIMER_SETTINGS_SAVED);
    _server.send(200, "application/json", "{\"ok\":true}");
}
void WebDashboard::handleQuietHoursGet()
{
    if (_app == nullptr)
    {
        _server.send(503, "application/json", "{\"error\":\"settings unavailable\"}");
        return;
    }
    const PashmakApp::QuietHoursSettings settings = _app->quietHoursSettings();
    char json[80];
    snprintf(json, sizeof(json), "{\"enabled\":%s,\"startHour\":%u,\"endHour\":%u}",
             settings.enabled ? "true" : "false", settings.startHour, settings.endHour);
    _server.sendHeader("Cache-Control", "no-store");
    _server.send(200, "application/json; charset=utf-8", json);
}
void WebDashboard::handleQuietHoursPost()
{
    if (_app == nullptr)
    {
        _server.send(503, "application/json", "{\"error\":\"settings unavailable\"}");
        return;
    }
    if (!_server.hasArg("enabled") || !_server.hasArg("startHour") || !_server.hasArg("endHour"))
    {
        _server.send(400, "application/json", "{\"error\":\"missing setting\"}");
        return;
    }
    const String enabled = _server.arg("enabled");
    const String start = _server.arg("startHour");
    const String end = _server.arg("endHour");
    if ((enabled != "0" && enabled != "1") || start.length() == 0 || start.length() > 2 ||
        end.length() == 0 || end.length() > 2)
    {
        _server.send(400, "application/json", "{\"error\":\"invalid setting\"}");
        return;
    }
    uint8_t startHour = 0;
    uint8_t endHour = 0;
    for (size_t i = 0; i < start.length(); ++i)
    {
        if (start[i] < '0' || start[i] > '9')
        {
            _server.send(400, "application/json", "{\"error\":\"invalid hour\"}");
            return;
        }
        startHour = static_cast<uint8_t>(startHour * 10U + start[i] - '0');
    }
    for (size_t i = 0; i < end.length(); ++i)
    {
        if (end[i] < '0' || end[i] > '9')
        {
            _server.send(400, "application/json", "{\"error\":\"invalid hour\"}");
            return;
        }
        endHour = static_cast<uint8_t>(endHour * 10U + end[i] - '0');
    }
    if (startHour > 23 || endHour > 23 || startHour == endHour)
    {
        _server.send(400, "application/json", "{\"error\":\"hours must differ and be between 0 and 23\"}");
        return;
    }
    PashmakApp::QuietHoursSettings settings;
    settings.enabled = enabled == "1";
    settings.startHour = startHour;
    settings.endHour = endHour;
    if (!_app->setQuietHoursSettings(settings))
    {
        _server.send(500, "application/json", "{\"error\":\"could not persist settings\"}");
        return;
    }
    _server.sendHeader("Cache-Control", "no-store");
    appendEvent(EVENT_QUIET_HOURS_SAVED);
    _server.send(200, "application/json", "{\"ok\":true}");
}
void WebDashboard::handleReminderSettingsGet()
{
    if (_app == nullptr)
    {
        _server.send(503, "application/json", "{\"error\":\"settings unavailable\"}");
        return;
    }
    const PashmakApp::ReminderSettings settings = _app->reminderSettings();
    char json[80];
    snprintf(json, sizeof(json), "{\"waterMinutes\":%u,\"movementMinutes\":%u}",
             settings.waterMinutes, settings.movementMinutes);
    _server.sendHeader("Cache-Control", "no-store");
    _server.send(200, "application/json; charset=utf-8", json);
}
void WebDashboard::handleReminderSettingsPost()
{
    if (_app == nullptr)
    {
        _server.send(503, "application/json", "{\"error\":\"settings unavailable\"}");
        return;
    }
    const char *keys[] = {"waterMinutes", "movementMinutes"};
    uint16_t values[2]{};
    for (uint8_t i = 0; i < 2; ++i)
    {
        if (!_server.hasArg(keys[i]))
        {
            _server.send(400, "application/json", "{\"error\":\"missing setting\"}");
            return;
        }
        const String raw = _server.arg(keys[i]);
        if (raw.length() == 0 || raw.length() > 3)
        {
            _server.send(400, "application/json", "{\"error\":\"invalid setting\"}");
            return;
        }
        uint16_t parsed = 0;
        for (size_t j = 0; j < raw.length(); ++j)
        {
            if (raw[j] < '0' || raw[j] > '9')
            {
                _server.send(400, "application/json", "{\"error\":\"invalid setting\"}");
                return;
            }
            parsed = static_cast<uint16_t>(parsed * 10U + static_cast<uint16_t>(raw[j] - '0'));
        }
        values[i] = parsed;
    }
    if ((values[0] != 0 && (values[0] < 15 || values[0] > 240)) ||
        (values[1] != 0 && (values[1] < 15 || values[1] > 180)))
    {
        _server.send(400, "application/json", "{\"error\":\"setting out of range\"}");
        return;
    }
    PashmakApp::ReminderSettings settings;
    settings.waterMinutes = values[0];
    settings.movementMinutes = values[1];
    if (!_app->setReminderSettings(settings))
    {
        _server.send(500, "application/json", "{\"error\":\"could not persist settings\"}");
        return;
    }
    _server.sendHeader("Cache-Control", "no-store");
    appendEvent(EVENT_REMINDERS_SAVED);
    _server.send(200, "application/json", "{\"ok\":true}");
}
void WebDashboard::handleEventLog()
{
    StoredEventLog log{}; readEventLog(log);
    String json; json.reserve(2200); json = "{\"events\":[";
    bool first=true;
    for(uint8_t i=0;i<log.count;++i){
        const uint8_t index=static_cast<uint8_t>((log.nextIndex+EVENT_LOG_CAPACITY-1U-i)%EVENT_LOG_CAPACITY);
        const EventRecord &event=log.records[index];if(event.sequence==0||event.code==0)continue;
        if(!first)json+=',';first=false;
        char item[128];
        snprintf(item,sizeof(item),"{\"sequence\":%lu,\"uptimeSeconds\":%lu,\"code\":%u,\"detail\":%u}",
          static_cast<unsigned long>(event.sequence),static_cast<unsigned long>(event.uptimeSeconds),
          static_cast<unsigned>(event.code),static_cast<unsigned>(event.detail));
        json+=item;
    }
    json+="]}";_server.sendHeader("Cache-Control","no-store");
    _server.send(200,"application/json; charset=utf-8",json);
}

void WebDashboard::handleDiagnostics()
{
    char response[256];
    snprintf(response, sizeof(response),
             "{\"uptimeSeconds\":%lu,\"freeHeapBytes\":%u,\"minFreeHeapBytes\":%u,\"maxAllocHeapBytes\":%u,\"freePsramBytes\":%u,\"wifiClients\":%u,\"cpuMHz\":%u,\"resetReasonCode\":%u}",
             static_cast<unsigned long>(millis() / 1000UL),
             static_cast<unsigned>(ESP.getFreeHeap()),
             static_cast<unsigned>(ESP.getMinFreeHeap()),
             static_cast<unsigned>(ESP.getMaxAllocHeap()),
             static_cast<unsigned>(ESP.getFreePsram()),
             static_cast<unsigned>(WiFi.softAPgetStationNum()),
             static_cast<unsigned>(ESP.getCpuFreqMHz()),
             static_cast<unsigned>(esp_reset_reason()));
    _server.sendHeader("Cache-Control", "no-store");
    _server.send(200, "application/json; charset=utf-8", response);
}

void WebDashboard::handleWifiSettingsGet()
{
    char ssid[WIFI_AP_SSID_CAPACITY]{};
    char password[WIFI_AP_PASSWORD_CAPACITY]{};
    loadAccessPointConfig(ssid, password);

    char response[96];
    snprintf(response, sizeof(response), "{\"ssid\":\"%s\",\"passwordConfigured\":true}", ssid);
    _server.sendHeader("Cache-Control", "no-store");
    _server.send(200, "application/json; charset=utf-8", response);
}

void WebDashboard::handleWifiSettingsPost()
{
    if (!_server.hasArg("ssid") || !_server.hasArg("password"))
    {
        _server.send(400, "application/json", "{\"error\":\"missing setting\"}");
        return;
    }

    const String ssidValue = _server.arg("ssid");
    const String newPassword = _server.arg("password");
    if (ssidValue.length() == 0 || ssidValue.length() > 32)
    {
        _server.send(400, "application/json", "{\"error\":\"SSID must contain 1 to 32 characters\"}");
        return;
    }
    for (size_t i = 0; i < ssidValue.length(); ++i)
    {
        const unsigned char ch = static_cast<unsigned char>(ssidValue[i]);
        if (ch < 0x20 || ch > 0x7e || ch == '"' || ch == '\\')
        {
            _server.send(400, "application/json", "{\"error\":\"SSID contains unsupported characters\"}");
            return;
        }
    }

    char currentSsid[WIFI_AP_SSID_CAPACITY]{};
    char currentPassword[WIFI_AP_PASSWORD_CAPACITY]{};
    loadAccessPointConfig(currentSsid, currentPassword);
    String passwordValue = newPassword.length() == 0 ? String(currentPassword) : newPassword;
    if (passwordValue.length() < 8 || passwordValue.length() > 63)
    {
        _server.send(400, "application/json", "{\"error\":\"password must contain 8 to 63 characters\"}");
        return;
    }
    for (size_t i = 0; i < passwordValue.length(); ++i)
    {
        const unsigned char ch = static_cast<unsigned char>(passwordValue[i]);
        if (ch < 0x20 || ch > 0x7e)
        {
            _server.send(400, "application/json", "{\"error\":\"password contains unsupported characters\"}");
            return;
        }
    }

    const String ssid = ssidValue;
    if (!saveAccessPointConfig(ssid.c_str(), passwordValue.c_str()))
    {
        _server.send(500, "application/json", "{\"error\":\"could not persist Wi-Fi settings\"}");
        return;
    }

    appendEvent(EVENT_WIFI_SETTINGS_SAVED);
    _server.sendHeader("Cache-Control", "no-store");
    _server.send(200, "application/json; charset=utf-8", "{\"ok\":true,\"restarting\":true}");
    delay(700);
    WiFi.softAPdisconnect(true);
    delay(150);
    if (!WiFi.softAP(ssid.c_str(), passwordValue.c_str()))
        Serial.println("[WEB] Failed to restart access point with updated settings.");
    else
        Serial.printf("[WEB] Wi-Fi settings updated; SSID: %s\n", ssid.c_str());
}

void WebDashboard::handleNotFound()
{
    _server.send(404, "text/plain; charset=utf-8", "Not found");
}