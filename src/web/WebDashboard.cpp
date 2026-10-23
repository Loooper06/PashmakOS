#include "WebDashboard.h"
#include "../app/PashmakApp.h"
#include <WiFi.h>

namespace
{
    // Local access-point defaults. Change these before deployment if desired.
    constexpr char WIFI_AP_SSID[] = "PashmakOS";
    constexpr char WIFI_AP_PASSWORD[] = "pashmak123";

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
h1{font-size:clamp(1.45rem,4vw,2rem);margin:0}h2{font-size:1.05rem;margin:0 0 14px}.muted{color:var(--muted)}.status{font-size:.85rem;color:var(--accent)}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(145px,1fr));gap:12px;margin-bottom:16px}
.card{background:var(--card);border:1px solid var(--line);border-radius:16px;padding:16px;min-width:0}
.metric{font-size:clamp(1.25rem,3vw,1.8rem);font-weight:700;margin-top:8px;overflow-wrap:anywhere}.label{font-size:.88rem;color:var(--muted)}
.section{margin-top:16px}.table-wrap{overflow-x:auto}table{border-collapse:collapse;width:100%;white-space:nowrap;font-size:.9rem}th,td{text-align:left;padding:10px 12px;border-bottom:1px solid var(--line)}th{color:var(--muted);font-weight:600}
button{border:1px solid var(--line);background:#292e3e;color:var(--text);padding:9px 13px;border-radius:10px;font:inherit;cursor:pointer}button:hover{border-color:var(--accent)}
.pills{display:flex;gap:8px;flex-wrap:wrap;margin-bottom:12px}.empty{color:var(--muted);padding:12px 0}
footer{margin-top:18px;color:var(--muted);font-size:.82rem}
</style>
</head>
<body><main>
<header><div><h1>PashmakOS</h1><div class="muted">Pet activity dashboard</div></div><div><span id="connection" class="status">Connecting…</span> <button id="refresh">Refresh</button></div></header>
<div class="grid">
 <article class="card"><div class="label">Lifetime work</div><div id="total-work" class="metric">—</div></article>
 <article class="card"><div class="label">Lifetime play</div><div id="total-play" class="metric">—</div></article>
 <article class="card"><div class="label">Lifetime nap</div><div id="total-nap" class="metric">—</div></article>
 <article class="card"><div class="label">Completed work sessions</div><div id="total-sessions" class="metric">—</div></article>
</div>
<section class="card section"><h2>Recent activity</h2><div class="pills"><button data-days="1">Today / latest day</button><button data-days="7">7 days</button><button data-days="14">14 days</button></div>
<div class="grid">
 <article><div class="label">Work</div><div id="recent-work" class="metric">—</div></article>
 <article><div class="label">Play</div><div id="recent-play" class="metric">—</div></article>
 <article><div class="label">Nap</div><div id="recent-nap" class="metric">—</div></article>
 <article><div class="label">Completed sessions</div><div id="recent-sessions" class="metric">—</div></article>
</div><div id="recent-note" class="muted"></div></section>
<section class="card section"><h2>Daily history</h2><div class="table-wrap"><table><thead><tr><th>Date</th><th>Work</th><th>Play</th><th>Nap</th><th>Sessions</th></tr></thead><tbody id="daily"></tbody></table></div></section>
<section class="card section"><h2>Monthly history</h2><div class="table-wrap"><table><thead><tr><th>Month</th><th>Work</th><th>Play</th><th>Nap</th><th>Sessions</th></tr></thead><tbody id="monthly"></tbody></table></div></section>
<footer>Data is read from the ESP32's local statistics API. Keep this page open while connected to the PashmakOS Wi-Fi network.</footer>
</main>
<script>
const $=id=>document.getElementById(id);let snapshot=null;let selectedDays=7;
const duration=ms=>{const mins=Math.floor(Number(ms||0)/60000);const d=Math.floor(mins/1440),h=Math.floor((mins%1440)/60),m=mins%60;return d?d+'d '+h+'h':h?h+'h '+m+'m':m+'m'};
const dateLabel=n=>{const s=String(n);return s.length===8?s.slice(0,4)+'-'+s.slice(4,6)+'-'+s.slice(6,8):s};
const monthLabel=n=>{const s=String(n);return s.length===6?s.slice(0,4)+'-'+s.slice(4,6):s};
function metric(id,value){$(id).textContent=value}
function row(values){const tr=document.createElement('tr');values.forEach(v=>{const td=document.createElement('td');td.textContent=v;tr.appendChild(td)});return tr}
function render(data){snapshot=data;const t=data.totals||{};metric('total-work',duration(t.workMs));metric('total-play',duration(t.playMs));metric('total-nap',duration(t.napMs));metric('total-sessions',Number(t.completedWorkSessions||0).toLocaleString());
const days=(data.dailyHistory||[]).slice(-selectedDays);const recent=days.reduce((a,d)=>{a.workMs+=Number(d.workMs||0);a.playMs+=Number(d.playMs||0);a.napMs+=Number(d.napMs||0);a.completedWorkSessions+=Number(d.completedWorkSessions||0);return a},{workMs:0,playMs:0,napMs:0,completedWorkSessions:0});
metric('recent-work',duration(recent.workMs));metric('recent-play',duration(recent.playMs));metric('recent-nap',duration(recent.napMs));metric('recent-sessions',recent.completedWorkSessions.toLocaleString());$('recent-note').textContent='Showing '+days.length+' retained day(s).';
const daily=$('daily');daily.replaceChildren();(data.dailyHistory||[]).slice().reverse().forEach(d=>daily.appendChild(row([dateLabel(d.dateKey),duration(d.workMs),duration(d.playMs),duration(d.napMs),d.completedWorkSessions])));
if(!daily.children.length)daily.appendChild(row(['No daily records yet','—','—','—','—']));
const monthly=$('monthly');monthly.replaceChildren();(data.monthlyHistory||[]).slice().reverse().forEach(m=>monthly.appendChild(row([monthLabel(m.monthKey),duration(m.workMs),duration(m.playMs),duration(m.napMs),m.completedWorkSessions])));
if(!monthly.children.length)monthly.appendChild(row(['No monthly records yet','—','—','—','—']));
$('connection').textContent='Connected · auto-refresh 10s';}
async function refresh(){try{const response=await fetch('/api/stats',{cache:'no-store'});if(!response.ok)throw new Error('HTTP '+response.status);render(await response.json())}catch(e){$('connection').textContent='Connection lost · retrying';}}
$('refresh').addEventListener('click',refresh);document.querySelectorAll('[data-days]').forEach(b=>b.addEventListener('click',()=>{selectedDays=Number(b.dataset.days);if(snapshot)render(snapshot)}));refresh();setInterval(refresh,10000);
</script></body></html>
)HTML";
}

void WebDashboard::begin(PashmakApp &app)
{
    _app = &app;
    WiFi.mode(WIFI_AP);
    WiFi.setSleep(false);
    const bool apStarted = WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
    if (!apStarted)
    {
        Serial.println("[WEB] Failed to start Wi-Fi access point.");
        return;
    }

    _server.on("/", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/api/stats", HTTP_GET, [this]() { handleStats(); });
    _server.onNotFound([this]() { handleNotFound(); });
    _server.begin();
    _started = true;

    Serial.printf("[WEB] Wi-Fi SSID: %s\n", WIFI_AP_SSID);
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

void WebDashboard::handleNotFound()
{
    _server.send(404, "text/plain; charset=utf-8", "Not found");
}
