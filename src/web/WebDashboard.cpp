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
.settings-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(190px,1fr));gap:12px}.settings-grid label{display:grid;gap:6px;color:var(--muted);font-size:.88rem}.settings-grid input{width:100%;min-width:0;border:1px solid var(--line);background:#11131b;color:var(--text);border-radius:10px;padding:10px;font:inherit}.settings-grid input:focus{outline:2px solid var(--accent);outline-offset:1px}
.goal-layout{display:grid;grid-template-columns:minmax(0,1fr) auto;align-items:center;gap:16px}.goal-progress{height:12px;background:var(--line);border-radius:99px;overflow:hidden;margin-top:12px}.goal-progress-fill{height:100%;width:0;background:linear-gradient(90deg,#8fe0d0,#b9a0ff);border-radius:99px;transition:width .25s}.goal-number{width:92px;border:1px solid var(--line);background:#11131b;color:var(--text);border-radius:10px;padding:10px;font:inherit}.goal-status{margin-top:10px;font-size:.9rem;color:var(--muted)}@media(max-width:480px){.goal-layout{grid-template-columns:1fr}.goal-number{width:100%}}
.achievement-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(190px,1fr));gap:10px}.achievement{display:flex;gap:12px;align-items:center;border:1px solid var(--line);background:#202432;border-radius:14px;padding:12px;min-width:0}.achievement-icon{display:grid;place-items:center;flex:0 0 44px;height:44px;border-radius:13px;background:#302b43;font-size:1.45rem}.achievement.unlocked .achievement-icon{background:#3d365b}.achievement.locked{opacity:.58}.achievement-copy{min-width:0}.achievement-title{font-weight:700}.achievement-description{color:var(--muted);font-size:.8rem}.achievement-state{font-size:.72rem;color:#8fe0d0;font-weight:700;margin-top:3px}.achievement.locked .achievement-state{color:var(--muted)}.bond-row{display:flex;align-items:center;gap:12px;flex-wrap:wrap}.bond-track{height:9px;min-width:100px;flex:1;background:var(--line);border-radius:99px;overflow:hidden}.bond-fill{height:100%;width:0;background:linear-gradient(90deg,#8fe0d0,#b9a0ff);transition:width .25s}.bond-actions{display:flex;align-items:center;gap:10px;flex-wrap:wrap}.bond-actions button{background:#3b3156;border-color:#65518e}.bond-actions button:disabled{opacity:.55;cursor:wait}
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
 <article style="grid-column:span 2"><div class="label">Bond with your companion</div><div class="bond-row" style="margin-top:8px"><strong id="pet-bond">0 / 100</strong><div class="bond-track" role="progressbar" aria-label="Bond level" aria-valuemin="0" aria-valuemax="100" aria-valuenow="0" id="pet-bond-progress"><div id="pet-bond-fill" class="bond-fill"></div></div></div><div class="bond-actions" style="margin-top:12px"><button id="pet-interact" type="button">Give attention</button><span id="interaction-status" class="muted" role="status" aria-live="polite">Interact to build a lasting bond.</span></div></article>
</div></section>
<div class="grid">
 <article class="card"><div class="label">Lifetime work</div><div id="total-work" class="metric">—</div></article>
 <article class="card"><div class="label">Lifetime play</div><div id="total-play" class="metric">—</div></article>
 <article class="card"><div class="label">Lifetime nap</div><div id="total-nap" class="metric">—</div></article>
 <article class="card"><div class="label">Completed work sessions</div><div id="total-sessions" class="metric">—</div></article>
</div>
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
<section class="card section" aria-labelledby="daily-goal-heading">
<h2 id="daily-goal-heading">Daily focus goal</h2>
<div class="goal-layout"><div><div id="daily-goal-count" class="metric">— / 4 sessions</div><div class="goal-progress" role="progressbar" aria-label="Daily focus goal progress" aria-valuemin="0" aria-valuemax="4" aria-valuenow="0" id="daily-goal-progress"><div id="daily-goal-fill" class="goal-progress-fill"></div></div><div id="daily-goal-status" class="goal-status" role="status" aria-live="polite">Waiting for activity data…</div></div><label for="daily-goal-target" class="label">Target sessions<br><input id="daily-goal-target" class="goal-number" type="number" min="1" max="20" value="4" inputmode="numeric"></label></div>
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
const $=id=>document.getElementById(id);let snapshot=null;let selectedDays=7;let refreshInFlight=false;let lastUpdatedAt=0;let dailySessionGoal=4;try{const savedGoal=Number(localStorage.getItem('pashmakDailySessionGoal'));if(Number.isInteger(savedGoal)&&savedGoal>=1&&savedGoal<=20)dailySessionGoal=savedGoal}catch(e){}
const duration=ms=>{const mins=Math.floor(Number(ms||0)/60000);const d=Math.floor(mins/1440),h=Math.floor((mins%1440)/60),m=mins%60;return d?d+'d '+h+'h':h?h+'h '+m+'m':m+'m'};
const dateLabel=n=>{const s=String(n);return s.length===8?s.slice(0,4)+'-'+s.slice(4,6)+'-'+s.slice(6,8):s};
const monthLabel=n=>{const s=String(n);return s.length===6?s.slice(0,4)+'-'+s.slice(4,6):s};
function metric(id,value){$(id).textContent=value}
function renderAchievements(data){const totals=data.totals||{};const sessions=Number(totals.completedWorkSessions||0);const records=Array.isArray(data.dailyHistory)?data.dailyHistory:[];const latest=records.length?records[records.length-1]:null;const todaySessions=latest?Number(latest.completedWorkSessions||0):0;const goalReached=todaySessions>=dailySessionGoal;const work=Number(totals.workMs||0),play=Number(totals.playMs||0),nap=Number(totals.napMs||0);const definitions=[{id:'first-session',icon:'🌱',title:'First Focus',description:'Complete your first work session.',unlocked:sessions>=1,progress:sessions>=1?'1 / 1':'0 / 1'},{id:'five-sessions',icon:'🧩',title:'Finding Your Rhythm',description:'Complete 5 work sessions.',unlocked:sessions>=5,progress:Math.min(sessions,5)+' / 5'},{id:'twenty-five-sessions',icon:'🔥',title:'Focus Builder',description:'Complete 25 work sessions.',unlocked:sessions>=25,progress:Math.min(sessions,25)+' / 25'},{id:'hundred-sessions',icon:'🏆',title:'Focus Master',description:'Complete 100 work sessions.',unlocked:sessions>=100,progress:Math.min(sessions,100)+' / 100'},{id:'daily-goal',icon:'🎯',title:'Goal Getter',description:'Reach your daily session goal.',unlocked:goalReached,progress:goalReached?dailySessionGoal+' / '+dailySessionGoal:Math.min(todaySessions,dailySessionGoal)+' / '+dailySessionGoal},{id:'balanced-day',icon:'🌈',title:'Balanced Companion',description:'Record work, play, and nap time.',unlocked:work>0&&play>0&&nap>0,progress:work>0&&play>0&&nap>0?'All activity types recorded':'Keep exploring each activity'}];const grid=$('achievement-grid');grid.replaceChildren();definitions.forEach(a=>{const card=document.createElement('article');card.className='achievement '+(a.unlocked?'unlocked':'locked');const icon=document.createElement('div');icon.className='achievement-icon';icon.textContent=a.icon;icon.setAttribute('aria-hidden','true');const copy=document.createElement('div');copy.className='achievement-copy';const title=document.createElement('div');title.className='achievement-title';title.textContent=a.title;const description=document.createElement('div');description.className='achievement-description';description.textContent=a.description;const state=document.createElement('div');state.className='achievement-state';state.textContent=a.unlocked?'UNLOCKED':a.progress;copy.append(title,description,state);card.append(icon,copy);grid.appendChild(card)});const unlocked=definitions.filter(a=>a.unlocked).length;metric('achievement-summary',unlocked+' of '+definitions.length+' achievements unlocked. Keep going to collect them all.')}
function renderDailyGoal(data){const records=Array.isArray(data.dailyHistory)?data.dailyHistory:[];const latest=records.length?records[records.length-1]:null;const completed=latest?Number(latest.completedWorkSessions||0):0;const goal=dailySessionGoal;const progress=Math.min(100,Math.round(completed/goal*100));$('daily-goal-target').value=String(goal);metric('daily-goal-count',completed+' / '+goal+' sessions');$('daily-goal-fill').style.width=progress+'%';const bar=$('daily-goal-progress');bar.setAttribute('aria-valuemax',String(goal));bar.setAttribute('aria-valuenow',String(Math.min(completed,goal)));if(!latest){$('daily-goal-status').textContent='No daily activity recorded yet.';return}const date=dateLabel(latest.dateKey);$('daily-goal-status').textContent=completed>=goal?'Goal completed for '+date+' — great work!':(goal-completed)+' more completed session'+(goal-completed===1?'':'s')+' to reach the goal for '+date+'.'}
function saveDailyGoal(){const value=Number($('daily-goal-target').value);if(!Number.isInteger(value)||value<1||value>20){$('daily-goal-status').textContent='Choose a goal between 1 and 20 sessions.';$('daily-goal-target').value=String(dailySessionGoal);return}dailySessionGoal=value;try{localStorage.setItem('pashmakDailySessionGoal',String(value))}catch(e){}if(snapshot)renderDailyGoal(snapshot);else $('daily-goal-status').textContent='Goal set. Waiting for activity data.'}
function downloadCsv(filename,headers,records){const quote=value=>{let text=String(value??'');if(/^[\s\u0000-\u001f]*[=+@-]/.test(text))text="'"+text;return '"'+text.replace(/"/g,'""')+'"'};const csv=[headers,...records].map(record=>record.map(quote).join(',')).join('\r\n');const blob=new Blob(['\ufeff',csv],{type:'text/csv;charset=utf-8;'});const url=URL.createObjectURL(blob);const link=document.createElement('a');link.href=url;link.download=filename;document.body.appendChild(link);link.click();link.remove();setTimeout(()=>URL.revokeObjectURL(url),1000)}
function row(values){const tr=document.createElement('tr');values.forEach(v=>{const td=document.createElement('td');td.textContent=v;tr.appendChild(td)});return tr}
const petProfiles={0:{emoji:'🐶',tag:'Classic companion',color:'#c6a7ff',tint:'#4b3b67'},1:{emoji:'🐾',tag:'Soft & cozy',color:'#8fe0d0',tint:'#244d4b'},2:{emoji:'✨',tag:'Pixel buddy',color:'#ffbf86',tint:'#5b4030'}};
function render(data){snapshot=data;renderDailyGoal(data);renderAchievements(data);const pets=Array.isArray(data.pets)?data.pets:[];const picker=$('pet-picker');picker.replaceChildren();pets.forEach(p=>{const id=Number(p.id),profile=petProfiles[id]||{emoji:'🐾',tag:'Companion',color:'#b9a0ff',tint:'#39314d'},active=Number(p.id)===Number(data.activePetId);const b=document.createElement('button');b.type='button';b.className='pet-option';b.style.setProperty('--pet-color',profile.color);b.style.setProperty('--pet-tint',profile.tint);b.setAttribute('aria-pressed',String(active));b.setAttribute('aria-label','Select '+p.name+(active?', currently active':''));const avatar=document.createElement('span');avatar.className='pet-avatar';avatar.setAttribute('aria-hidden','true');avatar.textContent=profile.emoji;const copy=document.createElement('span');copy.className='pet-option-copy';const name=document.createElement('span');name.className='pet-option-name';name.textContent=p.name;const tag=document.createElement('span');tag.className='pet-option-meta';tag.textContent=profile.tag;copy.append(name,tag);const state=document.createElement('span');state.className='pet-selected';state.textContent=active?'ACTIVE':'SELECT';b.append(avatar,copy,state);b.addEventListener('click',()=>selectPet(id));picker.appendChild(b)});const active=pets.find(p=>Number(p.id)===Number(data.activePetId));$('pet-heading').textContent='Pet status'+(active?' · '+active.name:'');const pet=data.pet||{};const energy=Math.max(0,Math.min(100,Number(pet.energy||0)));metric('pet-energy',energy+'%');metric('pet-mood',String(pet.mood||'unknown').replace(/\b\w/g,ch=>ch.toUpperCase()));metric('pet-message',pet.message||'No message yet.');$('pet-energy-bar').style.width=energy+'%';const bond=Math.max(0,Math.min(100,Number(pet.bond||0)));metric('pet-bond',bond+' / 100');$('pet-bond-fill').style.width=bond+'%';$('pet-bond-progress').setAttribute('aria-valuenow',String(bond));const t=data.totals||{};metric('total-work',duration(t.workMs));metric('total-play',duration(t.playMs));metric('total-nap',duration(t.napMs));metric('total-sessions',Number(t.completedWorkSessions||0).toLocaleString());
const days=(data.dailyHistory||[]).slice(-selectedDays);const recent=days.reduce((a,d)=>{a.workMs+=Number(d.workMs||0);a.playMs+=Number(d.playMs||0);a.napMs+=Number(d.napMs||0);a.completedWorkSessions+=Number(d.completedWorkSessions||0);return a},{workMs:0,playMs:0,napMs:0,completedWorkSessions:0});
metric('recent-work',duration(recent.workMs));metric('recent-play',duration(recent.playMs));metric('recent-nap',duration(recent.napMs));metric('recent-sessions',recent.completedWorkSessions.toLocaleString());$('recent-note').textContent='Showing '+days.length+' retained day(s).';
const trend=$('activity-trend');trend.replaceChildren();const maxActivity=Math.max(1,...days.map(d=>Math.max(Number(d.workMs||0),Number(d.playMs||0),Number(d.napMs||0))));if(!days.length){const empty=document.createElement('div');empty.className='chart-empty';empty.textContent='No activity recorded for this period yet.';trend.appendChild(empty)}else{days.forEach(d=>{const item=document.createElement('div');item.className='trend-row';const date=document.createElement('div');date.className='trend-date';date.textContent=dateLabel(d.dateKey);const bars=document.createElement('div');bars.className='trend-bars';[['workMs','trend-work','Work'],['playMs','trend-play','Play'],['napMs','trend-nap','Nap']].forEach(([key,cls,label])=>{const track=document.createElement('div');track.className='trend-track';track.title=label+': '+duration(d[key]);track.setAttribute('aria-label',label+': '+duration(d[key]));const fill=document.createElement('div');fill.className='trend-fill '+cls;fill.style.width=Math.max(0,Math.min(100,Number(d[key]||0)/maxActivity*100))+'%';track.appendChild(fill);bars.appendChild(track)});item.append(date,bars);trend.appendChild(item)})}
const daily=$('daily');daily.replaceChildren();(data.dailyHistory||[]).slice().reverse().forEach(d=>daily.appendChild(row([dateLabel(d.dateKey),duration(d.workMs),duration(d.playMs),duration(d.napMs),d.completedWorkSessions])));
if(!daily.children.length)daily.appendChild(row(['No daily records yet','—','—','—','—']));
const monthly=$('monthly');monthly.replaceChildren();(data.monthlyHistory||[]).slice().reverse().forEach(m=>monthly.appendChild(row([monthLabel(m.monthKey),duration(m.workMs),duration(m.playMs),duration(m.napMs),m.completedWorkSessions])));
if(!monthly.children.length)monthly.appendChild(row(['No monthly records yet','—','—','—','—']));
$('export-daily').disabled=!(data.dailyHistory||[]).length;$('export-monthly').disabled=!(data.monthlyHistory||[]).length;}
async function loadTimerSettings(){try{const response=await fetch('/api/timer-settings',{cache:'no-store'});if(!response.ok)throw new Error('HTTP '+response.status);const settings=await response.json();if(!Number.isInteger(settings.workMinutes)||settings.workMinutes<1||settings.workMinutes>180||!Number.isInteger(settings.playMinutes)||settings.playMinutes<1||settings.playMinutes>60||!Number.isInteger(settings.napMinutes)||settings.napMinutes<1||settings.napMinutes>120||!Number.isInteger(settings.workCyclesBeforeLongRest)||settings.workCyclesBeforeLongRest<1||settings.workCyclesBeforeLongRest>8)throw new Error('Invalid settings payload');$('setting-work').value=settings.workMinutes;$('setting-play').value=settings.playMinutes;$('setting-nap').value=settings.napMinutes;$('setting-cycles').value=settings.workCyclesBeforeLongRest;$('settings-status').textContent='Loaded from device.'}catch(e){$('settings-status').textContent='Could not load settings. Defaults are shown; try refresh.'}}
async function saveTimerSettings(){const values={workMinutes:Number($('setting-work').value),playMinutes:Number($('setting-play').value),napMinutes:Number($('setting-nap').value),workCyclesBeforeLongRest:Number($('setting-cycles').value)};if(!Number.isInteger(values.workMinutes)||values.workMinutes<1||values.workMinutes>180||!Number.isInteger(values.playMinutes)||values.playMinutes<1||values.playMinutes>60||!Number.isInteger(values.napMinutes)||values.napMinutes<1||values.napMinutes>120||!Number.isInteger(values.workCyclesBeforeLongRest)||values.workCyclesBeforeLongRest<1||values.workCyclesBeforeLongRest>8){$('settings-status').textContent='Check the allowed ranges before saving.';return}const button=$('settings-save');button.disabled=true;$('settings-status').textContent='Saving…';try{const response=await fetch('/api/timer-settings',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(values)});if(!response.ok)throw new Error('HTTP '+response.status);$('settings-status').textContent='Saved. New durations are active immediately.';await refresh()}catch(e){$('settings-status').textContent='Save failed · '+(e.message||'connection error')}finally{button.disabled=false}}
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
async function interactWithPet(){const button=$('pet-interact');button.disabled=true;$('interaction-status').textContent='Waiting for your companion…';try{const response=await fetch('/api/pet/interact',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'action=interact'});const result=await response.json();if(response.status===429){$('interaction-status').textContent='Your pet needs a few seconds before another interaction.';return}if(!response.ok)throw new Error(result.error||('HTTP '+response.status));$('interaction-status').textContent='Bond increased to '+result.bond+'/100 · reaction shown on the TFT.';await refresh()}catch(e){$('interaction-status').textContent='Interaction failed · '+(e.message||'connection error')}finally{button.disabled=false}}
async function selectPet(id){try{const response=await fetch('/api/pet/select',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'id='+encodeURIComponent(id)});if(!response.ok)throw new Error('HTTP '+response.status);await refresh()}catch(e){$('connection').dataset.state=snapshot?'stale':'error';$('connection').textContent=snapshot?'Pet selection failed · showing last valid data':'Pet selection failed';}}
async function refresh(){if(refreshInFlight){refreshRequested=true;return}refreshInFlight=true;if(refreshTimer!==null){clearTimeout(refreshTimer);refreshTimer=null}$('refresh').disabled=true;const controller=new AbortController();const timeout=setTimeout(()=>controller.abort(),5000);try{const response=await fetch('/api/stats',{cache:'no-store',signal:controller.signal});if(!response.ok)throw new Error('HTTP '+response.status);const data=await response.json();if(!isValidStats(data))throw new Error('Invalid statistics payload');render(data);$('connection').dataset.state='connected';$('connection').textContent='Connected · updated '+new Date().toLocaleTimeString();lastUpdatedAt=Date.now();retryDelay=10000}catch(e){$('connection').dataset.state=snapshot?'stale':'error';$('connection').textContent=snapshot?'Stale data · last valid update '+new Date(lastUpdatedAt).toLocaleTimeString():'Connection unavailable · retrying';retryDelay=Math.min(retryDelay*2,60000)}finally{clearTimeout(timeout);refreshInFlight=false;$('refresh').disabled=false;const nextDelay=refreshRequested?250:retryDelay;refreshRequested=false;scheduleRefresh(nextDelay)}}
$('refresh').addEventListener('click',refresh);
$('pet-interact').addEventListener('click',interactWithPet);
$('settings-save').addEventListener('click',saveTimerSettings);
$('daily-goal-target').value=String(dailySessionGoal);$('daily-goal-target').addEventListener('change',saveDailyGoal);$('daily-goal-target').addEventListener('keydown',event=>{if(event.key==='Enter'){event.preventDefault();saveDailyGoal()}});
loadTimerSettings();
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
    _server.on("/api/pet/interact", HTTP_POST, [this]() { handlePetInteraction(); });
    _server.on("/api/timer-settings", HTTP_GET, [this]() { handleTimerSettingsGet(); });
    _server.on("/api/timer-settings", HTTP_POST, [this]() { handleTimerSettingsPost(); });
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
    _server.send(200, "application/json", "{\"ok\":true}");
}

void WebDashboard::handleNotFound()
{
    _server.send(404, "text/plain; charset=utf-8", "Not found");
}
