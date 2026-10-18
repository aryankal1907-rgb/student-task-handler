'use strict';
/* FocusMate frontend. Talks to the C++ backend through /api/*.
   Task data lives in the C++ program (tasks.txt etc.); only personal
   extras (theme, streak, XP, focus minutes) are kept in this browser. */
const TYPES = ['Assignment','Lab Work','Tutorial','Quiz Preparation','Exam Preparation','Project','Other'];
const $ = (s, r = document) => r.querySelector(s);
const el = (h) => { const t = document.createElement('template'); t.innerHTML = h.trim(); return t.content.firstChild; };
const esc = (s) => String(s).replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
const pad = (n) => String(n).padStart(2, '0');
const isoOf = (d) => `${d.getFullYear()}-${pad(d.getMonth()+1)}-${pad(d.getDate())}`;
const parseDMY = (s) => { const [d,m,y] = s.split('-').map(Number); return new Date(y, m-1, d); };
const toDMY = (iso) => iso.split('-').reverse().join('-');
const MONTHS = ['January','February','March','April','May','June','July','August','September','October','November','December'];
const THEMES = { sunset:['#ff7a3d','#ffb347'], lavender:['#7357f0','#ff6fa5'], mint:['#1f9e69','#e8b92f'], ocean:['#2a7de1','#21c7bb'] };
const ICON = {
  check:'<svg viewBox="0 0 24 24"><path d="M5 12l5 5 9-10"/></svg>',
  edit:'<svg viewBox="0 0 24 24"><path d="M4 20h4L19 9l-4-4L4 16z"/></svg>',
  trash:'<svg viewBox="0 0 24 24"><path d="M5 7h14M9 7V4h6v3M7 7l1 13h8l1-13"/></svg>'
};

/* ---------- personal data kept in the browser ---------- */
const defaults = { name:'', theme:'sunset', mode:'auto', goal:3, xp:0, streak:0, best:0, lastActive:'', checkin:'', done:{}, focus:{}, sessions:0, totalDone:0, badges:[], notified:'' };
let G = Object.assign({}, defaults, JSON.parse(localStorage.getItem('fm_game') || '{}'));
const saveG = () => localStorage.setItem('fm_game', JSON.stringify(G));
const todayISO = () => isoOf(new Date());
const level = () => Math.floor(Math.sqrt(G.xp / 40)) + 1;
const lvlBase = (l) => 40 * (l-1) * (l-1);

/* ---------- state ---------- */
let S = null;                       // latest state from the C++ server
const UI = { view:'home', filter:'all', subject:'', layout:'list', q:'', calMonth:new Date(), calSel:null, editing:null, focusTask:'' };
const BADGES = [
  { id:'first', e:'🌱', n:'First step',   d:'Complete a task',        ok:() => G.totalDone >= 1 },
  { id:'ten',   e:'🏅', n:'Ten down',     d:'Complete 10 tasks',      ok:() => G.totalDone >= 10 },
  { id:'s3',    e:'🔥', n:'On fire',      d:'3-day streak',           ok:() => G.best >= 3 },
  { id:'s7',    e:'🚀', n:'Unstoppable',  d:'7-day streak',           ok:() => G.best >= 7 },
  { id:'f5',    e:'🎯', n:'Deep focus',   d:'5 focus sessions',       ok:() => G.sessions >= 5 },
  { id:'l5',    e:'👑', n:'Level 5',      d:'Reach level 5',          ok:() => level() >= 5 }
];

/* ---------- api ---------- */
async function api(path, method = 'GET', params) {
  try {
    const opt = { method };
    if (params) { opt.body = new URLSearchParams(params); }
    const r = await fetch(path, opt);
    const j = await r.json();
    if (!r.ok || j.ok === false) { toast(j.error || 'Something went wrong.', { err:true }); return null; }
    if (j.state) { S = j.state; }
    return j;
  } catch (e) {
    toast('Cannot reach the FocusMate server. Is the C++ program still running?', { err:true });
    return null;
  }
}
const byId = (id) => S.tasks.find(t => t.id === id) || S.deleted.find(t => t.id === id);
const pending = () => S.tasks.filter(t => !t.completed);
const ordered = () => S.queue.map(id => S.tasks.find(t => t.id === id)).filter(Boolean);
const whenText = (d) => d < 0 ? 'Overdue' : d === 0 ? 'Due today' : d === 1 ? 'Due tomorrow' : `${d} days left`;
const sub = (s) => esc(s.trim());

/* ---------- gamification ---------- */
function markActive() {
  const t = todayISO();
  if (G.lastActive === t) return;
  const y = new Date(); y.setDate(y.getDate()-1);
  G.streak = G.lastActive === isoOf(y) ? G.streak + 1 : 1;
  G.best = Math.max(G.best, G.streak);
  G.lastActive = t;
}
function gainXP(n) {
  const before = level();
  G.xp += n;
  if (level() > before) { toast(`Level ${level()} reached! 🎉`); burst(); }
  checkBadges(); saveG();
}
function checkBadges() {
  BADGES.forEach(b => { if (!G.badges.includes(b.id) && b.ok()) { G.badges.push(b.id); toast(`${b.e} Badge unlocked: ${b.n}`); } });
}
function streakAlive() {
  const y = new Date(); y.setDate(y.getDate()-1);
  return G.lastActive === todayISO() || G.lastActive === isoOf(y) ? G.streak : 0;
}

/* ---------- actions ---------- */
async function complete(id) {
  const t = byId(id); if (!t || t.completed) return;
  const r = await api('/api/tasks/complete', 'POST', { id });
  if (!r) return;
  markActive(); G.totalDone++;
  G.done[todayISO()] = (G.done[todayISO()] || 0) + 1;
  const goalHit = G.done[todayISO()] === G.goal;
  gainXP(20 + (t.priority === 1 ? 10 : 0));
  burst();
  toast(goalHit ? `Daily goal reached! ${G.goal} of ${G.goal} done 🎯` : `Nice! +${20 + (t.priority===1?10:0)} XP`, { undo:true });
  render();
}
async function remove(id) {
  const t = byId(id);
  if (await api('/api/tasks/delete', 'POST', { ids:id })) { toast(`Deleted “${t.name}”`, { undo:true }); render(); }
}
async function restore(id) { if (await api('/api/tasks/restore', 'POST', { ids:id })) { toast('Task restored'); render(); } }
async function undo() { if (await api('/api/undo', 'POST', { n:1 })) { toast('Undone'); render(); } }
async function redo() { if (await api('/api/redo', 'POST', { n:1 })) { toast('Redone'); render(); } }

/* ---------- toasts, confetti ---------- */
function toast(msg, o = {}) {
  const t = el(`<div class="toast ${o.err ? 'err' : ''}" role="status"><span>${esc(msg)}</span>${o.undo ? '<button>Undo</button>' : ''}</div>`);
  if (o.undo) t.querySelector('button').onclick = () => { t.remove(); undo(); };
  $('#toasts').append(t); setTimeout(() => t.remove(), o.err ? 6000 : 4500);
}
function burst() {
  if (matchMedia('(prefers-reduced-motion:reduce)').matches) return;
  const c = $('#confetti'), x = c.getContext('2d'); c.width = innerWidth; c.height = innerHeight;
  const cols = [...THEMES[G.theme], '#fff', '#ffd166', '#ff6b6b'];
  const P = Array.from({length:110}, () => ({ x:innerWidth/2, y:innerHeight/3, vx:(Math.random()-.5)*16, vy:Math.random()*-14-3, s:4+Math.random()*6, c:cols[Math.random()*cols.length|0], r:Math.random()*6 }));
  let f = 0;
  (function tick() {
    x.clearRect(0,0,c.width,c.height);
    P.forEach(p => { p.vy += .35; p.x += p.vx; p.y += p.vy; p.r += .2; x.save(); x.translate(p.x,p.y); x.rotate(p.r); x.fillStyle = p.c; x.fillRect(-p.s/2,-p.s/2,p.s,p.s*.6); x.restore(); });
    if (++f < 110) requestAnimationFrame(tick); else x.clearRect(0,0,c.width,c.height);
  })();
}

/* ---------- theme ---------- */
function applyTheme() {
  const dark = G.mode === 'dark' || (G.mode === 'auto' && matchMedia('(prefers-color-scheme:dark)').matches);
  document.documentElement.dataset.theme = G.theme;
  document.documentElement.dataset.mode = dark ? 'dark' : 'light';
  $('#modeBtn').innerHTML = dark ? '<svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="4"/><path d="M12 3v2M12 19v2M3 12h2M19 12h2M5.6 5.6L7 7M17 17l1.4 1.4M5.6 18.4L7 17M17 7l1.4-1.4"/></svg>' : '<svg viewBox="0 0 24 24"><path d="M20 14.5A8 8 0 0 1 9.5 4 8 8 0 1 0 20 14.5z"/></svg>';
}
matchMedia('(prefers-color-scheme:dark)').addEventListener('change', applyTheme);

/* ---------- views ---------- */
function taskRow(t, withDays = true) {
  const urgent = !t.completed && t.daysLeft <= 1;
  return `<div class="task ${t.completed ? 'done' : ''}" data-id="${t.id}">
    <button class="chk ${t.completed ? 'on' : ''}" data-act="done" aria-label="Mark ${esc(t.name)} as done">${t.completed ? ICON.check : ''}</button>
    <div class="t"><b>${esc(t.name)}</b><span class="small muted">${sub(t.subject)} · ${esc(t.type)}</span></div>
    ${t.completed ? '<span class="pill done">Done</span>' : `<span class="pill p${t.priority}">${t.priorityName}</span>`}
    ${withDays ? `<span class="when ${urgent ? 'urgent' : ''}">${t.completed ? t.deadline : whenText(t.daysLeft)}</span>` : ''}
    <span class="acts"><button data-act="edit" aria-label="Edit">${ICON.edit}</button><button data-act="del" aria-label="Delete">${ICON.trash}</button></span>
  </div>`;
}
const emptyBox = (big, msg, btn) => `<div class="empty"><big>${big}</big>${msg}${btn || ''}</div>`;

function greeting() {
  const h = new Date().getHours();
  return h < 5 ? 'Still up' : h < 12 ? 'Good morning' : h < 17 ? 'Good afternoon' : h < 21 ? 'Good evening' : 'Late night';
}

function viewHome() {
  const q = ordered(), next = q[0], p = pending();
  const dueSoon = p.filter(t => t.daysLeft <= 1);
  const doneToday = G.done[todayISO()] || 0, pct = Math.min(100, Math.round(doneToday / G.goal * 100));
  const lv = level(), xpIn = G.xp - lvlBase(lv), xpNeed = lvlBase(lv+1) - lvlBase(lv);
  const week = Array.from({length:7}, (_, i) => { const d = new Date(); d.setDate(d.getDate()+i); return d; });
  const subj = Object.entries(S.subjects).map(([k, ids]) => { const ts = ids.map(byId).filter(Boolean); return { k, n:ts.length, d:ts.filter(t => t.completed).length }; });
  const streakRisk = G.lastActive !== todayISO() && streakAlive() > 0;
  const lead = !p.length ? 'Everything is done. Add something new or take a break.' :
    dueSoon.length ? `${dueSoon.length} task${dueSoon.length>1?'s are':' is'} due by tomorrow. Let’s clear ${dueSoon.length>1?'them':'it'} first.` :
    `${p.length} open task${p.length>1?'s':''}, ${S.stats.dueThisWeek} due this week.`;
  return `
  ${streakRisk ? `<div class="alert">🔥 Your ${G.streak}-day streak ends tonight. Finish one task or one focus session to keep it.</div>` : ''}
  <div class="grid-home">
    <div class="stack">
      <section class="card hero">
        <h1>${greeting()}${G.name ? ', ' + esc(G.name) : ''}!</h1>
        <p>${lead}</p>
        ${next ? `<div class="next" data-id="${next.id}">
          <div class="info"><small>Do this next · ${sub(next.subject)}</small><strong>${esc(next.name)}</strong><span class="pill p${next.priority}">${next.priorityName}</span> <small style="display:inline">${whenText(next.daysLeft)}</small></div>
          <div class="btns"><button class="btn-dark" data-act="focus">Start focus</button><button class="btn-soft" data-act="done">Mark done</button></div>
        </div>` : `<div class="next"><div class="info"><strong>Nothing waiting</strong><small>Add a task to get started.</small></div><div class="btns"><button class="btn-dark" data-act="add">Add task</button></div></div>`}
      </section>
      <section class="card">
        <h3>Up next <span class="small muted">sorted by priority, then deadline</span></h3>
        <div class="quick"><input id="quickInput" placeholder="Quick add: type a task and press Enter (due in 7 days)" maxlength="60"><button class="ghost" id="quickAdd">Add</button></div>
        ${q.length ? q.slice(0,6).map(t => taskRow(t)).join('') : emptyBox('🎉', 'You’re all caught up.')}
        ${q.length > 6 ? `<button class="ghost" style="margin-top:10px" data-go="tasks">See all ${q.length} tasks</button>` : ''}
      </section>
      <section class="card"><h3>This week</h3><div class="week">${week.map((d,i) => { const k = `${pad(d.getDate())}-${pad(d.getMonth()+1)}-${d.getFullYear()}`; const n = p.filter(t => t.deadline === k).length;
        return `<button class="day ${i===0?'today':''} ${n?'has':''}" data-day="${isoOf(d)}">${d.toLocaleDateString(undefined,{weekday:'short'})}<b>${d.getDate()}</b>${n ? `<span>${n} due</span>` : '<span>&nbsp;</span>'}</button>`; }).join('')}</div></section>
    </div>
    <div class="stack">
      <section class="card"><h3>Today’s goal</h3>
        <div class="ring-wrap"><div class="ring" style="--v:${pct}"><span>${doneToday}/${G.goal}<small>tasks</small></span></div>
        <div><b>${doneToday >= G.goal ? 'Goal reached!' : `${G.goal - doneToday} to go`}</b><p class="small muted">${streakAlive() ? `🔥 ${streakAlive()}-day streak` : 'Finish a task to start a streak'}<br>Best: ${G.best} days</p></div></div>
        <button class="checkin" id="checkin" ${G.checkin === todayISO() ? 'disabled' : ''}>${G.checkin === todayISO() ? 'Checked in today ✓' : 'Daily check-in · +10 XP'}</button>
      </section>
      <section class="card"><h3>Level</h3><div class="level"><div class="lv">${lv}</div><div style="flex:1"><b>${G.xp} XP</b> <span class="small muted">· ${xpNeed - xpIn} to level ${lv+1}</span><div class="bar"><i style="width:${xpIn/xpNeed*100}%"></i></div></div></div>
        <h3 style="margin-top:18px">Badges</h3><div class="badges">${BADGES.map(b => `<div class="badge ${G.badges.includes(b.id)?'':'locked'}" title="${b.d}"><em>${b.e}</em>${b.n}</div>`).join('')}</div></section>
      <section class="card"><h3>Subjects</h3><div class="subj">${subj.length ? subj.map(s => `<div class="r"><div class="h"><span>${sub(s.k)}</span><span class="muted">${s.d}/${s.n}</span></div><div class="bar"><i style="width:${s.n ? s.d/s.n*100 : 0}%"></i></div></div>`).join('') : '<p class="muted">No subjects yet.</p>'}</div></section>
    </div>
  </div>`;
}

function filtered() {
  const q = UI.q.trim().toLowerCase();
  return [...S.tasks].filter(t =>
    (UI.filter === 'all' || (UI.filter === 'pending' && !t.completed) || (UI.filter === 'done' && t.completed) || (UI.filter === 'high' && !t.completed && t.priority === 1)) &&
    (!UI.subject || t.subject === UI.subject) &&
    (!q || (t.name + ' ' + t.subject + ' ' + t.type + ' ' + t.id).toLowerCase().includes(q)));
}
function viewTasks() {
  const list = filtered(), subjects = Object.keys(S.subjects);
  const chips = [['all','All'],['pending','Pending'],['high','High priority'],['done','Completed']];
  const head = `<div class="bar-row">
    <div class="seg">${chips.map(c => `<button data-filter="${c[0]}" class="${UI.filter===c[0]?'on':''}">${c[1]}</button>`).join('')}</div>
    <select id="subjectSel" aria-label="Filter by subject"><option value="">All subjects</option>${subjects.map(s => `<option ${UI.subject===s?'selected':''} value="${esc(s)}">${sub(s)}</option>`).join('')}</select>
    <span style="flex:1"></span>
    <div class="seg"><button data-layout="list" class="${UI.layout==='list'?'on':''}">List</button><button data-layout="board" class="${UI.layout==='board'?'on':''}">Board</button></div></div>`;
  if (!list.length) return head + `<section class="card">${emptyBox('📝', UI.q || UI.filter !== 'all' ? 'No tasks match these filters.' : 'No tasks yet. Add your first one.', '<br><br><button class="primary" data-act="add">+ New task</button>')}</section>`;
  if (UI.layout === 'list') return head + `<section class="card">${list.map(t => taskRow(t)).join('')}</section>`;
  const col = (title, ts, cls) => `<div class="col"><h4>${title}<span class="muted">${ts.length}</span></h4>${ts.map(t => `<div class="tcard ${cls}" data-id="${t.id}"><div class="meta"><span>${sub(t.subject)}</span><span class="acts"><button data-act="edit" aria-label="Edit">${ICON.edit}</button><button data-act="del" aria-label="Delete">${ICON.trash}</button></span></div><b>${esc(t.name)}</b><div class="meta"><span>${esc(t.type)}</span><span>${t.completed ? '' : whenText(t.daysLeft)}</span></div>${t.completed ? '' : '<button class="ghost" data-act="done">Mark done</button>'}</div>`).join('') || '<p class="small muted" style="padding:8px">Empty</p>'}</div>`;
  const p = (n) => list.filter(t => !t.completed && t.priority === n);
  return head + `<div class="board">${col('🔴 High', p(1), 'p1')}${col('🟠 Medium', p(2), 'p2')}${col('🟢 Low', p(3), 'p3')}${col('✅ Done', list.filter(t => t.completed), 'dn')}</div>`;
}

function viewCalendar() {
  const m = UI.calMonth, y = m.getFullYear(), mo = m.getMonth();
  const first = new Date(y, mo, 1), start = new Date(y, mo, 1 - first.getDay());
  const today = todayISO(); let cells = '';
  for (let i = 0; i < 42; i++) {
    const d = new Date(start); d.setDate(start.getDate() + i);
    const ts = S.tasks.filter(t => t.deadline === `${pad(d.getDate())}-${pad(d.getMonth()+1)}-${d.getFullYear()}`);
    const iso = isoOf(d);
    cells += `<button class="cell ${d.getMonth()!==mo?'out':''} ${iso===today?'today':''} ${iso===UI.calSel?'sel':''}" data-cal="${iso}"><b>${d.getDate()}</b>${ts.slice(0,3).map(t => `<span class="dot ${t.completed?'':'p'+t.priority}" style="${t.completed?'background:#aaa':''}"></span>`).join('')}${ts.length>3?`<span class="muted">+${ts.length-3}</span>`:''}</button>`;
  }
  const sel = UI.calSel ? S.tasks.filter(t => t.deadline === toDMY(UI.calSel)) : [];
  return `<div class="focus-wrap"><section class="card">
    <div class="cal-head"><h2>${MONTHS[mo]} ${y}</h2><div class="bar-row"><button class="ghost" data-cm="-1" aria-label="Previous month">‹</button><button class="ghost" data-cm="0">Today</button><button class="ghost" data-cm="1" aria-label="Next month">›</button></div></div>
    <div class="cal">${['Sun','Mon','Tue','Wed','Thu','Fri','Sat'].map(d => `<div class="dow">${d}</div>`).join('')}${cells}</div></section>
    <section class="card"><h3>${UI.calSel ? parseDMY(toDMY(UI.calSel)).toDateString() : 'Pick a day'}</h3>
      ${UI.calSel ? (sel.length ? sel.map(t => taskRow(t, false)).join('') : emptyBox('🌤️', 'Nothing due this day.', '<br><br><button class="ghost" data-act="add-on">Add a task for this day</button>')) : '<p class="muted">Select a date to see what is due and add tasks to it.</p>'}</section></div>`;
}

/* focus timer (kept outside render so it keeps running) */
const T = { mode:'focus', len:{ focus:25, break:5 }, left:25*60, run:false, id:null, end:0 };
function tfmt(s) { return `${pad(Math.floor(s/60))}:${pad(s%60)}`; }
function viewFocus() {
  const ts = pending();
  const todayMin = G.focus[todayISO()] || 0;
  return `<div class="focus-wrap"><section class="card timer">
    <div class="seg"><button data-tm="focus" class="${T.mode==='focus'?'on':''}">Focus</button><button data-tm="break" class="${T.mode==='break'?'on':''}">Break</button></div>
    <div class="dial" id="dial"><div><strong id="tnum">${tfmt(T.left)}</strong><small>${T.mode==='focus'?'Stay with one task':'Stretch, drink water'}</small></div></div>
    <select id="ftask" aria-label="Task to focus on"><option value="">No specific task</option>${ts.map(t => `<option value="${t.id}" ${String(UI.focusTask)===String(t.id)?'selected':''}>${esc(t.name)}</option>`).join('')}</select>
    <div class="row"><button class="big" id="tgo">${T.run?'Pause':'Start'}</button><button class="btn-soft" style="background:rgba(255,255,255,.25);color:#fff" id="treset">Reset</button></div>
    <div class="row small"><label style="color:#fff">Focus (min) <input id="lenF" type="number" min="1" max="120" value="${T.len.focus}" style="width:70px"></label><label style="color:#fff">Break (min) <input id="lenB" type="number" min="1" max="60" value="${T.len.break}" style="width:70px"></label></div>
  </section><div class="stack"><section class="card"><h3>Today</h3><div class="ring-wrap"><div class="ring" style="--v:${Math.min(100,todayMin/120*100)}"><span>${todayMin}<small>minutes</small></span></div><div><b>${G.sessions} sessions total</b><p class="small muted">Each finished focus session gives +15 XP and keeps your streak alive.</p></div></div></section>
  <section class="card"><h3>Tip</h3><p class="muted">Put your phone out of reach, pick the task above, and work until the ring fills. Short breaks make long sessions possible.</p></section></div></div>`;
}
function tickUI() {
  const n = $('#tnum'); if (n) n.textContent = tfmt(T.left);
  const d = $('#dial'); if (d) d.style.setProperty('--v', (1 - T.left / (T.len[T.mode]*60)) * 100);
  document.title = T.run ? `${tfmt(T.left)} · FocusMate` : 'FocusMate';
}
function tStart() {
  if (T.run) { clearInterval(T.id); T.run = false; }
  else { T.run = true; T.end = Date.now() + T.left * 1000;
    T.id = setInterval(() => { T.left = Math.max(0, Math.round((T.end - Date.now())/1000)); tickUI(); if (T.left <= 0) tDone(); }, 250); }
  if (UI.view === 'focus') { const b = $('#tgo'); if (b) b.textContent = T.run ? 'Pause' : 'Start'; }
}
function tDone() {
  clearInterval(T.id); T.run = false;
  try { const a = new AudioContext(), o = a.createOscillator(); o.connect(a.destination); o.frequency.value = 660; o.start(); o.stop(a.currentTime + .4); } catch(e){}
  if (T.mode === 'focus') {
    markActive(); G.sessions++; G.focus[todayISO()] = (G.focus[todayISO()]||0) + T.len.focus; gainXP(15); burst();
    toast('Focus session complete! Take a break ☕'); notify('Focus session complete', 'Time for a short break.');
    T.mode = 'break';
  } else { toast('Break over. Ready for another round?'); notify('Break over', 'Back to it!'); T.mode = 'focus'; }
  T.left = T.len[T.mode]*60; document.title = 'FocusMate'; render();
}

function viewInsights() {
  const subjects = Object.keys(S.subjects), p = [1,2,3].map(n => pending().filter(t => t.priority === n).length);
  const days = Array.from({length:7}, (_, i) => { const d = new Date(); d.setDate(d.getDate()-6+i); return d; });
  const dn = days.map(d => G.done[isoOf(d)] || 0), mx = Math.max(1, ...dn), mp = Math.max(1, ...p);
  const col = { ADDED:'Added', DELETED:'Deleted', COMPLETED:'Completed', UPDATED:'Updated', RESTORED:'Restored' };
  const hist = [...S.history].reverse().slice(0, 40);
  return `<div class="insights">
    <section class="card"><h3>Completed this week</h3><div class="bars">${dn.map((n,i) => `<div class="${i===6?'td':''}"><span>${n}</span><i style="height:${n/mx*90}px"></i><span>${days[i].toLocaleDateString(undefined,{weekday:'narrow'})}</span></div>`).join('')}</div></section>
    <section class="card"><h3>Open tasks by priority</h3><div class="bars">${['High','Medium','Low'].map((l,i) => `<div><span>${p[i]}</span><i style="height:${p[i]/mp*90}px;background:var(--${['hi','med','lo'][i]})"></i><span>${l}</span></div>`).join('')}</div></section>
    <section class="card"><h3>Find tasks by deadline <span class="small muted">AVL tree range search</span></h3>
      <div class="bar-row"><input type="date" id="rA" aria-label="From"><span class="muted">to</span><input type="date" id="rB" aria-label="To"><button class="ghost" id="rGo">Search</button></div><div id="rOut" class="res"></div></section>
    <section class="card"><h3>Look up by ID <span class="small muted">hash table</span></h3>
      <div class="bar-row"><input type="number" id="hId" min="1" placeholder="Task ID" style="width:120px"><button class="ghost" id="hGo">Find</button></div><div id="hOut" class="res"></div></section>
    <section class="card"><h3>Subject graph <span class="small muted">BFS / DFS</span></h3>
      <div class="bar-row"><select id="gSub" aria-label="Start subject">${subjects.map(s => `<option value="${esc(s)}">${sub(s)}</option>`).join('')}</select><div class="seg"><button data-gm="bfs" class="on">BFS</button><button data-gm="dfs">DFS</button></div><button class="ghost" id="gGo">Traverse</button></div><div id="gOut"></div></section>
    <section class="card"><h3>Activity history <span class="small muted">linked list</span></h3><div class="tl">${hist.length ? hist.map(h => `<div class="ev ${h.action.split(':')[0]}"><em>${esc(col[h.action] || h.action)}</em><span>${esc(h.name)} <span class="muted">#${h.id}</span></span></div>`).join('') : '<p class="muted">No activity yet.</p>'}</div></section></div>`;
}
const miniTask = (t) => `<div class="task ${t.completed?'done':''}"><div class="t"><b>#${t.id} · ${esc(t.name)}</b><span class="small muted">${sub(t.subject)} · due ${t.deadline}</span></div>${t.completed ? '<span class="pill done">Done</span>' : `<span class="pill p${t.priority}">${t.priorityName}</span>`}</div>`;

function viewTrash() {
  return `<section class="card"><h3>Deleted tasks <span class="small muted">${S.deleted.length} item${S.deleted.length===1?'':'s'}</span></h3>${S.deleted.length ? S.deleted.map(t => `<div class="task" data-id="${t.id}"><div class="t"><b>${esc(t.name)}</b><span class="small muted">${sub(t.subject)} · ${esc(t.type)} · due ${t.deadline}</span></div>${!t.completed && t.daysLeft < 0 ? '<span class="pill done">Past deadline</span>' : `<button class="ghost" data-act="restore">Restore</button>`}</div>`).join('') : emptyBox('🗑️', 'Nothing deleted. Overdue tasks land here automatically.')}</section>`;
}

/* ---------- render ---------- */
const VIEWS = { home:viewHome, tasks:viewTasks, calendar:viewCalendar, focus:viewFocus, insights:viewInsights, trash:viewTrash };
function render() {
  if (!S) return;
  document.querySelectorAll('.nav').forEach(n => n.classList.toggle('on', n.dataset.view === UI.view));
  $('#view').innerHTML = VIEWS[UI.view]();
  $('#undoBtn').disabled = !S.undo.count; $('#redoBtn').disabled = !S.redo.count;
  $('#undoBtn').title = S.undo.label ? 'Undo: ' + S.undo.label : 'Nothing to undo';
  $('#redoBtn').title = S.redo.label ? 'Redo: ' + S.redo.label : 'Nothing to redo';
  $('#streakNum').textContent = streakAlive();
  $('#streakChip').style.opacity = streakAlive() ? 1 : .55;
  const tc = $('#trashCount'); tc.hidden = !S.deleted.length; tc.textContent = S.deleted.length;
  $('#subjectList').innerHTML = Object.keys(S.subjects).map(s => `<option value="${esc(s.trim())}">`).join('');
  if (UI.view === 'focus') tickUI();
}
function go(v) { UI.view = v; render(); scrollTo({ top:0 }); }

/* ---------- task dialog ---------- */
function openDlg(t, dateISO) {
  UI.editing = t ? t.id : null;
  $('#dlgTitle').textContent = t ? 'Edit task' : 'New task';
  $('#dlgSave').textContent = t ? 'Save changes' : 'Add task';
  $('#fType').innerHTML = TYPES.map(x => `<option ${t && t.type === x ? 'selected' : ''}>${x}</option>`).join('');
  $('#fSubject').value = t ? t.subject.trim() : (UI.subject || '');
  $('#fName').value = t ? t.name : '';
  const dflt = new Date(); dflt.setDate(dflt.getDate() + 3);
  $('#fDate').min = todayISO();
  $('#fDate').value = t ? toDMY(t.deadline) : (dateISO || isoOf(dflt));
  hint(); $('#taskDlg').showModal(); $('#fName').focus();
}
function hint() {
  const v = $('#fDate').value; if (!v) { $('#fHint').textContent = ''; return; }
  const d = Math.round((new Date(v + 'T12:00') - new Date(todayISO() + 'T12:00')) / 864e5);
  $('#fHint').textContent = d < 0 ? 'That date has already passed.' : `${whenText(d)} → priority will be ${d <= 10 ? 'High' : d <= 20 ? 'Medium' : 'Low'}.`;
}
async function saveDlg(e) {
  e.preventDefault();
  const body = { type:$('#fType').value, subject:$('#fSubject').value, name:$('#fName').value, deadline:toDMY($('#fDate').value) };
  const r = UI.editing ? await api('/api/tasks/update', 'POST', { id:UI.editing, ...body }) : await api('/api/tasks', 'POST', body);
  if (!r) return;
  $('#taskDlg').close();
  if (!UI.editing) { markActive(); gainXP(2); saveG(); }
  toast(UI.editing ? 'Task updated' : 'Task added', { undo:true }); render();
}

/* ---------- notifications ---------- */
function notify(title, body) { if ('Notification' in window && Notification.permission === 'granted') new Notification(title, { body }); }
function deadlineReminder() {
  if (G.notified === todayISO() || !S) return;
  const soon = pending().filter(t => t.daysLeft <= 1);
  if (soon.length) { notify('FocusMate', `${soon.length} task(s) due by tomorrow: ${soon[0].name}${soon.length>1?' and more':''}`); G.notified = todayISO(); saveG(); }
}

/* ---------- events ---------- */
document.addEventListener('click', async (e) => {
  const t = e.target;
  if (!t.closest('#settings') && !t.closest('#settingsBtn')) $('#settings').hidden = true;
  const nav = t.closest('[data-view]'); if (nav) return go(nav.dataset.view);
  const gt = t.closest('[data-go]'); if (gt) return go(gt.dataset.go);
  const day = t.closest('[data-day]'); if (day) { UI.calSel = day.dataset.day; UI.calMonth = new Date(day.dataset.day + 'T12:00'); return go('calendar'); }
  const f = t.closest('[data-filter]'); if (f) { UI.filter = f.dataset.filter; return render(); }
  const l = t.closest('[data-layout]'); if (l) { UI.layout = l.dataset.layout; return render(); }
  const cal = t.closest('[data-cal]'); if (cal) { UI.calSel = cal.dataset.cal; return render(); }
  const cm = t.closest('[data-cm]'); if (cm) { const n = +cm.dataset.cm; UI.calMonth = n ? new Date(UI.calMonth.getFullYear(), UI.calMonth.getMonth() + n, 1) : new Date(); return render(); }
  const tm = t.closest('[data-tm]'); if (tm) { if (T.run) tStart(); T.mode = tm.dataset.tm; T.left = T.len[T.mode]*60; return render(); }
  const gm = t.closest('[data-gm]'); if (gm) { document.querySelectorAll('[data-gm]').forEach(b => b.classList.toggle('on', b === gm)); return; }
  const act = t.closest('[data-act]');
  if (act) {
    const row = act.closest('[data-id]'), id = row ? +row.dataset.id : null, a = act.dataset.act;
    if (a === 'done') complete(id); else if (a === 'del') remove(id); else if (a === 'restore') restore(id);
    else if (a === 'edit') openDlg(byId(id)); else if (a === 'add') openDlg();
    else if (a === 'add-on') openDlg(null, UI.calSel);
    else if (a === 'focus') { UI.focusTask = id; go('focus'); }
    return;
  }
  if (t.closest('#addBtn')) return openDlg();
  if (t.closest('#undoBtn')) return undo();
  if (t.closest('#redoBtn')) return redo();
  if (t.closest('#modeBtn')) { const dark = document.documentElement.dataset.mode === 'dark'; G.mode = dark ? 'light' : 'dark'; saveG(); return applyTheme(); }
  if (t.closest('#settingsBtn')) { const p = $('#settings'); p.hidden = !p.hidden; return syncSettings(); }
  if (t.closest('#dlgCancel')) return $('#taskDlg').close();
  if (t.closest('#checkin')) { if (G.checkin !== todayISO()) { G.checkin = todayISO(); markActive(); gainXP(10); burst(); toast('Checked in. +10 XP'); render(); } return; }
  if (t.closest('#quickAdd')) return quick();
  if (t.closest('#tgo')) return tStart();
  if (t.closest('#treset')) { if (T.run) tStart(); T.left = T.len[T.mode]*60; return render(); }
  if (t.closest('#rGo')) {
    const a = $('#rA').value, b = $('#rB').value; if (!a || !b) return toast('Pick both dates first.', { err:true });
    const r = await api(`/api/range?start=${toDMY(a)}&end=${toDMY(b)}`); if (!r) return;
    $('#rOut').innerHTML = r.tasks.length ? r.tasks.map(miniTask).join('') + (r.highestPriorityId > 0 ? `<p class="small muted">Highest priority in range: #${r.highestPriorityId}</p>` : '') : '<p class="muted">No tasks fall in that range.</p>'; return;
  }
  if (t.closest('#hGo')) { const r = await api('/api/search?id=' + ($('#hId').value || 0)); if (r) $('#hOut').innerHTML = miniTask(r.task); return; }
  if (t.closest('#gGo')) {
    const mode = document.querySelector('[data-gm].on').dataset.gm, s = $('#gSub').value; if (!s) return toast('Add a task first.', { err:true });
    const r = await api(`/api/graph?mode=${mode}&start=${encodeURIComponent(s)}`); if (!r) return;
    $('#gOut').innerHTML = `<div class="chain">${r.order.map((n,i) => `${i?'<span class="arrow">→</span>':''}<span class="node ${n.kind}">${n.kind==='subject' ? sub(n.value) : '#'+n.value+' '+esc(n.name)}</span>`).join('')}</div>`;
  }
});
function syncSettings() {
  $('#nameInput').value = G.name; $('#goalInput').value = G.goal; $('#goalVal').textContent = G.goal;
  document.querySelectorAll('.sw').forEach(s => s.classList.toggle('on', s.dataset.t === G.theme));
  document.querySelectorAll('#modeSeg button').forEach(b => b.classList.toggle('on', b.dataset.m === G.mode));
}
function quick() {
  const i = $('#quickInput'), name = i.value.trim(); if (!name) return i.focus();
  const d = new Date(); d.setDate(d.getDate() + 7);
  api('/api/tasks', 'POST', { type:'Other', subject:UI.subject || 'general', name, deadline:toDMY(isoOf(d)) }).then(r => { if (r) { markActive(); gainXP(2); toast('Task added for next week', { undo:true }); render(); } });
}
document.addEventListener('input', (e) => {
  const t = e.target;
  if (t.id === 'search') {
  const pos = t.selectionStart;
  UI.q = t.value;
  if (UI.view !== 'tasks') UI.view = 'tasks';
  render();
  const s = $('#search');
  s.focus();
  s.setSelectionRange(pos, pos);
}
  if (t.id === 'subjectSel') { UI.subject = t.value; render(); }
  if (t.id === 'fDate') hint();
  if (t.id === 'nameInput') { G.name = t.value; saveG(); if (UI.view === 'home') { const h = $('.hero h1'); if (h) h.textContent = greeting() + (G.name ? ', ' + G.name : '') + '!'; } }
  if (t.id === 'goalInput') { G.goal = +t.value; $('#goalVal').textContent = G.goal; saveG(); }
  if (t.id === 'lenF' || t.id === 'lenB') { const k = t.id === 'lenF' ? 'focus' : 'break'; T.len[k] = Math.max(1, Math.min(120, +t.value || 25)); if (!T.run && T.mode === k) { T.left = T.len[k]*60; tickUI(); } }
});
document.addEventListener('change', (e) => { if (e.target.id === 'ftask') UI.focusTask = e.target.value; });
document.addEventListener('keydown', (e) => {
  if (e.target.id === 'quickInput' && e.key === 'Enter') return quick();
  if (e.target.matches('input,select,textarea') || e.ctrlKey || e.metaKey) { if (e.key === 'Escape') e.target.blur(); return; }
  if (e.key === '/') { e.preventDefault(); $('#search').focus(); }
  else if (e.key === 'n') { e.preventDefault(); openDlg(); }
  else if (e.key === 'z') undo(); else if (e.key === 'Z') redo();
  else if ('12345'.includes(e.key) && e.key) go(['home','tasks','calendar','focus','insights'][+e.key - 1]);
});
$('#taskForm').addEventListener('submit', saveDlg);
$('#search').addEventListener('keydown', async (e) => {
  if (e.key === 'Enter' && /^\d+$/.test(e.target.value.trim())) { const r = await api('/api/search?id=' + e.target.value.trim()); if (r) openDlg(r.task); }
});
window.addEventListener('beforeunload', (e) => { if (T.run) { e.preventDefault(); e.returnValue = ''; } });
$('#swatches').innerHTML = Object.entries(THEMES).map(([k, c]) => `<button class="sw" data-t="${k}" title="${k}" style="background:linear-gradient(135deg,${c[0]},${c[1]})" aria-label="${k} theme"></button>`).join('');
$('#swatches').onclick = (e) => { const s = e.target.closest('.sw'); if (s) { G.theme = s.dataset.t; saveG(); applyTheme(); syncSettings(); } };
$('#modeSeg').onclick = (e) => { const b = e.target.closest('button'); if (b) { G.mode = b.dataset.m; saveG(); applyTheme(); syncSettings(); } };
$('#notifyBtn').onclick = async () => {
  if (!('Notification' in window)) return toast('This browser does not support notifications.', { err:true });
  const p = await Notification.requestPermission();
  toast(p === 'granted' ? 'Reminders on. You’ll hear about tasks due soon.' : 'Notifications were blocked in the browser.', { err:p !== 'granted' });
  if (p === 'granted') { G.notified = ''; deadlineReminder(); }
};

/* ---------- boot ---------- */
applyTheme(); T.left = T.len.focus * 60;
api('/api/state').then(() => { if (!S) { $('#view').innerHTML = '<section class="card">' + emptyBox('🔌', 'The C++ server is not running. Start it, then reload this page.') + '</section>'; return; }
  render(); deadlineReminder(); setInterval(() => api('/api/state').then(() => { if (!document.hidden && !$('#taskDlg').open) render(); }), 60000); });
document.addEventListener('visibilitychange', () => { if (!document.hidden) api('/api/state').then(render); });