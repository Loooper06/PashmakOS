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
h1{font-size:clamp(1.45rem,4vw,2rem);margin:0}h2{font-size:1.05rem;margin:0 0 14px}.muted{color:var(--muted)}.status{font-size:.85rem;color:var(--accent)}.status[data-state="stale"],.status[data-state="error"]{color:#e7a35d}.status[data-state="connected"]{color:#78d8ad}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(145px,1fr));gap:12px;margin-bottom:16px}
.card{background:var(--card);border:1px solid var(--line);border-radius:16px;padding:16px;min-width:0}
.metric{font-size:clamp(1.25rem,3vw,1.8rem);font-weight:700;margin-top:8px;overflow-wrap:anywhere}.label{font-size:.88rem;color:var(--muted)}
.section{margin-top:16px}.table-wrap{overflow-x:auto}table{border-collapse:collapse;width:100%;white-space:nowrap;font-size:.9rem}th,td{text-align:left;padding:10px 12px;border-bottom:1px solid var(--line)}th{color:var(--muted);font-weight:600}
button{border:1px solid var(--line);background:#292e3e;color:var(--text);padding:9px 13px;border-radius:10px;font:inherit;cursor:pointer}button:hover{border-color:var(--accent)}
.pills{display:flex;gap:8px;flex-wrap:wrap;margin-bottom:12px}.empty{color:var(--muted);padding:12px 0}.trend-list{display:grid;gap:10px}.trend-row{display:grid;grid-template-columns:92px minmax(0,1fr);align-items:center;gap:12px}.trend-date{font-size:.82rem;color:var(--muted)}.trend-bars{display:grid;gap:4px}.trend-track{height:8px;background:var(--line);border-radius:99px;overflow:hidden}.trend-fill{height:100%;width:0;border-radius:99px;transition:width .25s}.trend-work{background:#b9a0ff}.trend-play{background:#8fe0d0}.trend-nap{background:#ffbf86}.trend-legend{display:flex;gap:14px;flex-wrap:wrap;margin-bottom:12px;font-size:.82rem;color:var(--muted)}.legend-dot{display:inline-block;width:8px;height:8px;border-radius:50%;margin-right:5px}.chart-empty{color:var(--muted);padding:8px 0}button:disabled{opacity:.45;cursor:not-allowed}
.pet-picker{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:10px;margin-bottom:12px}.pet-option{display:flex;align-items:center;gap:12px;text-align:left;width:100%;padding:12px;background:#202432;border:1px solid var(--line);border-radius:14px;transition:border-color .18s,transform .18s,background .18s}.pet-option:hover{transform:translateY(-1px)}.pet-option[aria-pressed=true]{border-color:var(--pet-color);background:color-mix(in srgb,var(--pet-color) 14%,var(--card));box-shadow:inset 0 0 0 1px var(--pet-color)}.pet-avatar{display:grid;place-items:center;flex:0 0 44px;height:44px;border-radius:14px;background:var(--pet-tint);font-size:1.65rem}.pet-option-copy{min-width:0;flex:1}.pet-option-name{display:block;font-weight:700}.pet-option-meta{display:block;color:var(--muted);font-size:.78rem}.pet-selected{color:var(--pet-color);font-size:.8rem;font-weight:700}
footer{margin-top:18px;color:var(--muted);font-size:.82rem}
</style>
</head>
<body><main>
<header><div><h1>PashmakOS</h1><div class="muted">Pet activity dashboard</div></div><div><span id="connection" class="status">Connecting…</span> <button id="refresh">Refresh</button></div></header>
<section class="card section"><h2>Choose your pet</h2><div id="pet-picker" class="pet-picker" role="group" aria-label="Choose active pet"></div><div class="muted">Choose a companion to view its current energy, mood and message. Each pet has an independent behavior state.</div></section>
<section class="card section" id="pet-status"><h2 id="pet-heading">Pet status</h2><div class="grid">
 <article><div class="label">Energy</div><div id="pet-energy" class="metric">—</div><div style="height:8px;background:var(--line);border-radius:99px;overflow:hidden;margin-top:10px"><div id="pet-energy-bar" style="height:100%;width:0%;background:var(--accent);transition:width .25s"></div></div></article>
 <article><div class="label">Mood</div><div id="pet-mood" class="metric">—</div></article>
 <article style="grid-column:span 2"><div class="label">Pet says</div><div id="pet-message" style="font-size:1.05rem;margin-top:8px">Waiting for data…</div></article>
</div></section>
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
<section class="card section"><h2>Activity trend</h2><div class="trend-legend"><span><i class="legend-dot trend-work"></i>Work</span><span><i class="legend-dot trend-play"></i>Play</span><span><i class="legend-dot trend-nap"></i>Nap</span></div><div id="activity-trend" class="trend-list"><div class="chart-empty">Waiting for activity data…</div></div></section>
<section class="card section"><div style="display:flex;align-items:center;justify-content:space-between;gap:12px;flex-wrap:wrap"><h2>Daily history</h2><button id="export-daily" type="button">Export CSV</button></div><div class="table-wrap"><table><thead><tr><th>Date</th><th>Work</th><th>Play</th><th>Nap</th><th>Sessions</th></tr></thead><tbody id="daily"></tbody></table></div></section>
<section class="card section"><div style="display:flex;align-items:center;justify-content:space-between;gap:12px;flex-wrap:wrap"><h2>Monthly history</h2><button id="export-monthly" type="button">Export CSV</button></div><div class="table-wrap"><table><thead><tr><th>Month</th><th>Work</th><th>Play</th><th>Nap</th><th>Sessions</th></tr></thead><tbody id="monthly"></tbody></table></div></section>
<footer>Data is read from the ESP32's local statistics API. Keep this page open while connected to the PashmakOS Wi-Fi network.</footer>
</main>
<script>
const $=id=>document.getElementById(id);let snapshot=null;let selectedDays=7;let refreshInFlight=false;let lastUpdatedAt=0;
const duration=ms=>{const mins=Math.floor(Number(ms||0)/60000);const d=Math.floor(mins/1440),h=Math.floor((mins%1440)/60),m=mins%60;return d?d+'d '+h+'h':h?h+'h '+m+'m':m+'m'};
const dateLabel=n=>{const s=String(n);return s.length===8?s.slice(0,4)+'-'+s.slice(4,6)+'-'+s.slice(6,8):s};
const monthLabel=n=>{const s=String(n);return s.length===6?s.slice(0,4)+'-'+s.slice(4,6):s};
function metric(id,value){$(id).textContent=value}
function downloadCsv(filename,headers,records){const quote=value=>{let text=String(value??'');if(/^[\s\u0000-\u001f]*[=+@-]/.test(text))text="'"+text;return '"'+text.replace(/"/g,'""')+'"'};const csv=[headers,...records].map(record=>record.map(quote).join(',')).join('\\r\\n');const blob=new Blob(['\\ufeff',csv],{type:'text/csv;charset=utf-8;'});const url=URL.createObjectURL(blob);const link=document.createElement('a');link.href=url;link.download=filename;document.body.appendChild(link);link.click();link.remove();setTimeout(()=>URL.revokeObjectURL(url),1000)}
function row(values){const tr=document.createElement('tr');values.forEach(v=>{const td=document.createElement('td');td.textContent=v;tr.appendChild(td)});return tr}
const petProfiles={0:{emoji:'🐶',tag:'Classic companion',color:'#c6a7ff',tint:'#4b3b67'},1:{emoji:'🐾',tag:'Soft & cozy',color:'#8fe0d0',tint:'#244d4b'},2:{emoji:'✨',tag:'Pixel buddy',color:'#ffbf86',tint:'#5b4030'}};
function render(data){snapshot=data;const pets=Array.isArray(data.pets)?data.pets:[];const picker=$('pet-picker');picker.replaceChildren();pets.forEach(p=>{const id=Number(p.id),profile=petProfiles[id]||{emoji:'🐾',tag:'Companion',color:'#b9a0ff',tint:'#39314d'},active=Number(p.id)===Number(data.activePetId);const b=document.createElement('button');b.type='button';b.className='pet-option';b.style.setProperty('--pet-color',profile.color);b.style.setProperty('--pet-tint',profile.tint);b.setAttribute('aria-pressed',String(active));b.setAttribute('aria-label','Select '+p.name+(active?', currently active':''));const avatar=document.createElement('span');avatar.className='pet-avatar';avatar.setAttribute('aria-hidden','true');avatar.textContent=profile.emoji;const copy=document.createElement('span');copy.className='pet-option-copy';const name=document.createElement('span');name.className='pet-option-name';name.textContent=p.name;const tag=document.createElement('span');tag.className='pet-option-meta';tag.textContent=profile.tag;copy.append(name,tag);const state=document.createElement('span');state.className='pet-selected';state.textContent=active?'ACTIVE':'SELECT';b.append(avatar,copy,state);b.addEventListener('click',()=>selectPet(id));picker.appendChild(b)});const active=pets.find(p=>Number(p.id)===Number(data.activePetId));$('pet-heading').textContent='Pet status'+(active?' · '+active.name:'');const pet=data.pet||{};const energy=Math.max(0,Math.min(100,Number(pet.energy||0)));metric('pet-energy',energy+'%');metric('pet-mood',String(pet.mood||'unknown').replace(/\b\w/g,ch=>ch.toUpperCase()));metric('pet-message',pet.message||'No message yet.');$('pet-energy-bar').style.width=energy+'%';const t=data.totals||{};metric('total-work',duration(t.workMs));metric('total-play',duration(t.playMs));metric('total-nap',duration(t.napMs));metric('total-sessions',Number(t.completedWorkSessions||0).toLocaleString());
const days=(data.dailyHistory||[]).slice(-selectedDays);const recent=days.reduce((a,d)=>{a.workMs+=Number(d.workMs||0);a.playMs+=Number(d.playMs||0);a.napMs+=Number(d.napMs||0);a.completedWorkSessions+=Number(d.completedWorkSessions||0);return a},{workMs:0,playMs:0,napMs:0,completedWorkSessions:0});
metric('recent-work',duration(recent.workMs));metric('recent-play',duration(recent.playMs));metric('recent-nap',duration(recent.napMs));metric('recent-sessions',recent.completedWorkSessions.toLocaleString());$('recent-note').textContent='Showing '+days.length+' retained day(s).';
const trend=$('activity-trend');trend.replaceChildren();const maxActivity=Math.max(1,...days.map(d=>Math.max(Number(d.workMs||0),Number(d.playMs||0),Number(d.napMs||0))));if(!days.length){const empty=document.createElement('div');empty.className='chart-empty';empty.textContent='No activity recorded for this period yet.';trend.appendChild(empty)}else{days.forEach(d=>{const item=document.createElement('div');item.className='trend-row';const date=document.createElement('div');date.className='trend-date';date.textContent=dateLabel(d.dateKey);const bars=document.createElement('div');bars.className='trend-bars';[['workMs','trend-work','Work'],['playMs','trend-play','Play'],['napMs','trend-nap','Nap']].forEach(([key,cls,label])=>{const track=document.createElement('div');track.className='trend-track';track.title=label+': '+duration(d[key]);const fill=document.createElement('div');fill.className='trend-fill '+cls;fill.style.width=Math.max(0,Math.min(100,Number(d[key]||0)/maxActivity*100))+'%';track.appendChild(fill);bars.appendChild(track)});item.append(date,bars);trend.appendChild(item)})}
const daily=$('daily');daily.replaceChildren();(data.dailyHistory||[]).slice().reverse().forEach(d=>daily.appendChild(row([dateLabel(d.dateKey),duration(d.workMs),duration(d.playMs),duration(d.napMs),d.completedWorkSessions])));
if(!daily.children.length)daily.appendChild(row(['No daily records yet','—','—','—','—']));
const monthly=$('monthly');monthly.replaceChildren();(data.monthlyHistory||[]).slice().reverse().forEach(m=>monthly.appendChild(row([monthLabel(m.monthKey),duration(m.workMs),duration(m.playMs),duration(m.napMs),m.completedWorkSessions])));
if(!monthly.children.length)monthly.appendChild(row(['No monthly records yet','—','—','—','—']));
$('connection').dataset.state='connected';$('connection').textContent='Connected · updated '+new Date().toLocaleTimeString();lastUpdatedAt=Date.now();$('export-daily').disabled=!(data.dailyHistory||[]).length;$('export-monthly').disabled=!(data.monthlyHistory||[]).length;}
function isCounter(value){return value!==null&&value!==undefined&&value!==''&&Number.isFinite(Number(value))&&Number(value)>=0}
function isValidDateKey(value){const text=String(value);if(!/^\d{8}$/.test(text))return false;const y=Number(text.slice(0,4)),m=Number(text.slice(4,6)),d=Number(text.slice(6,8));const date=new Date(Date.UTC(y,m-1,d));return date.getUTCFullYear()===y&&date.getUTCMonth()===m-1&&date.getUTCDate()===d}
function isValidMonthKey(value){const text=String(value);return /^\d{6}$/.test(text)&&Number(text.slice(0,4))>=2000&&Number(text.slice(4,6))>=1&&Number(text.slice(4,6))<=12}
function isRecord(record,key,validator){return !!record&&validator(record[key])&&isCounter(record.workMs)&&isCounter(record.playMs)&&isCounter(record.napMs)&&isCounter(record.completedWorkSessions)}
function isValidStats(data){
 if(!data||typeof data!=='object'||data.version!==1)return false;
 if(!Number.isInteger(data.activePetId)||data.activePetId<0||data.activePetId>2)return false;
 if(!Array.isArray(data.pets)||data.pets.length!==3)return false;
 if(!data.pets.every((p,i)=>p&&Number(p.id)===i&&typeof p.name==='string'&&typeof p.mood==='string'&&typeof p.message==='string'&&Number.isFinite(Number(p.energy))&&Number(p.energy)>=0&&Number(p.energy)<=100))return false;
 const selected=data.pets[data.activePetId];
 if(!data.pet||typeof data.pet.mood!=='string'||typeof data.pet.message!=='string'||!Number.isFinite(Number(data.pet.energy))||Number(data.pet.energy)<0||Number(data.pet.energy)>100)return false;
 if(data.pet.mood!==selected.mood||data.pet.message!==selected.message||Number(data.pet.energy)!==Number(selected.energy))return false;
 if(!data.totals||!['workMs','playMs','napMs','completedWorkSessions'].every(k=>isCounter(data.totals[k])))return false;
 if(!data.recent||!['requestedDays','daysIncluded','workMs','playMs','napMs','completedWorkSessions'].every(k=>isCounter(data.recent[k]))||data.recent.daysIncluded>data.recent.requestedDays)return false;
 if(!Array.isArray(data.dailyHistory)||data.dailyHistory.length>14||!data.dailyHistory.every(d=>isRecord(d,'dateKey',isValidDateKey)))return false;
 if(!Array.isArray(data.monthlyHistory)||data.monthlyHistory.length>12||!data.monthlyHistory.every(m=>isRecord(m,'monthKey',isValidMonthKey)))return false;
 return true;
}
let retryDelay=10000;let refreshTimer=null;
function scheduleRefresh(delay){if(refreshTimer!==null)clearTimeout(refreshTimer);refreshTimer=setTimeout(()=>{refreshTimer=null;refresh()},delay)}
async function selectPet(id){try{const response=await fetch('/api/pet/select',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'id='+encodeURIComponent(id)});if(!response.ok)throw new Error('HTTP '+response.status);await refresh()}catch(e){$('connection').dataset.state=snapshot?'stale':'error';$('connection').textContent=snapshot?'Pet selection failed · showing last valid data':'Pet selection failed';}}
async function refresh(){if(refreshInFlight)return;refreshInFlight=true;if(refreshTimer!==null){clearTimeout(refreshTimer);refreshTimer=null}$('refresh').disabled=true;const controller=new AbortController();const timeout=setTimeout(()=>controller.abort(),5000);try{const response=await fetch('/api/stats',{cache:'no-store',signal:controller.signal});if(!response.ok)throw new Error('HTTP '+response.status);const data=await response.json();if(!isValidStats(data))throw new Error('Invalid statistics payload');render(data);retryDelay=10000}catch(e){$('connection').dataset.state=snapshot?'stale':'error';$('connection').textContent=snapshot?'Stale data · last valid update '+new Date(lastUpdatedAt).toLocaleTimeString():'Connection unavailable · retrying';retryDelay=Math.min(retryDelay*2,60000)}finally{clearTimeout(timeout);refreshInFlight=false;$('refresh').disabled=false;scheduleRefresh(retryDelay)}}
$('refresh').addEventListener('click',refresh);
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
    const bool apStarted = WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
    if (!apStarted)
    {
        Serial.println("[WEB] Failed to start Wi-Fi access point.");
        return;
    }

    _server.on("/", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/api/stats", HTTP_GET, [this]() { handleStats(); });
    _server.on("/api/pet/select", HTTP_POST, [this]() { handlePetSelection(); });
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

    _server.send(200, "application/json", "{\"ok\":true}");
}

void WebDashboard::handleNotFound()
{
    _server.send(404, "text/plain; charset=utf-8", "Not found");
}
