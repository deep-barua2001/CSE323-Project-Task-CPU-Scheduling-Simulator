// ============================================================
//  CPU Scheduling Simulator — script.js
//  Algorithms: FCFS, RR, SJF, SRTF, PRIORITY, PSJF
//  Modes: Single Algorithm + Comparison
// ============================================================

const COLORS = [
    '#00ffc8','#ff6b6b','#ffd93d','#6bcbff','#c77dff',
    '#ff9f43','#48dbfb','#ff6b9d','#a8e063','#fd9644',
    '#74b9ff','#fd79a8','#55efc4','#fdcb6e','#e17055','#81ecec'
];

// ============================================================
//  Mode switching
// ============================================================
function switchMode(mode) {
    document.getElementById('mode-single').style.display  = mode === 'single'  ? 'block' : 'none';
    document.getElementById('mode-compare').style.display = mode === 'compare' ? 'block' : 'none';
    document.getElementById('tab-single').classList.toggle('active',  mode === 'single');
    document.getElementById('tab-compare').classList.toggle('active', mode === 'compare');
}

// ============================================================
//  Single mode — algo change
// ============================================================
function onAlgoChange() {
    const algo = document.getElementById('algo').value;
    document.getElementById('quantumGroup').style.display = (algo === 'RR')   ? 'flex' : 'none';
    document.getElementById('psjfParams').style.display   = (algo === 'PSJF') ? 'flex' : 'none';
    document.getElementById('processSection').style.display = 'none';
    document.getElementById('resultsSection').style.display = 'none';
}

// ============================================================
//  Single mode — generate process rows
// ============================================================
function generateInputs() {
    const n    = parseInt(document.getElementById('n').value);
    const algo = document.getElementById('algo').value;
    if (!n || n < 1) { alert('Enter a valid number of processes.'); return; }

    const div = document.getElementById('processInputs');
    div.innerHTML = buildProcessRows(n, algo, '');

    document.getElementById('processSection').style.display = 'block';
    document.getElementById('resultsSection').style.display = 'none';
}

// ============================================================
//  Shared row builder (used by both modes)
//  prefix: '' for single, 'c' for compare shared
// ============================================================
function buildProcessRows(n, algo, prefix) {
    const needPriority = (algo === 'PRIORITY');
    let html = '';
    for (let i = 1; i <= n; i++) {
        const color = COLORS[(i - 1) % COLORS.length];
        html += `
        <div class="proc-row" style="--proc-color:${color}">
            <span class="proc-label">P${i}</span>
            <div class="proc-fields">
                <div class="field-group small">
                    <label>Arrival</label>
                    <input type="number" id="${prefix}a${i}" value="${i-1}" min="0" placeholder="0">
                </div>
                <div class="field-group small">
                    <label>Burst</label>
                    <input type="number" id="${prefix}b${i}" value="${Math.floor(Math.random()*6)+2}" min="1" placeholder="1">
                </div>
                <div class="field-group small" id="${prefix}pfield${i}" style="${needPriority?'':'display:none'}">
                    <label>Priority <span class="hint">(lower=urgent)</span></label>
                    <input type="number" id="${prefix}p${i}" value="${i}" min="0" placeholder="1">
                </div>
                ${!needPriority ? `<input type="hidden" id="${prefix}p${i}" value="0">` : ''}
            </div>
        </div>`;
    }
    return html;
}

// ============================================================
//  Helpers
// ============================================================
function getProcesses(n, needPriority, prefix) {
    prefix = prefix || '';
    const list = [];
    for (let i = 1; i <= n; i++) {
        list.push({
            pid:      i,
            label:    `P${i}`,
            arrival:  Number(document.getElementById(`${prefix}a${i}`).value) || 0,
            burst:    Number(document.getElementById(`${prefix}b${i}`).value) || 1,
            priority: needPriority ? (Number(document.getElementById(`${prefix}p${i}`).value) || 0) : 0,
            color:    COLORS[(i - 1) % COLORS.length]
        });
    }
    return list;
}

function buildResultRow(p, finish, waiting, turnaround, showPriority) {
    return `
    <tr>
        <td><span class="pid-badge" style="background:${p.color}">${p.label}</span></td>
        <td>${p.arrival}</td>
        <td>${p.burst}</td>
        <td>${showPriority ? p.priority : '—'}</td>
        <td>${finish}</td>
        <td class="${waiting < 0 ? 'warn' : ''}">${waiting}</td>
        <td>${turnaround}</td>
    </tr>`;
}

function buildCompactResultRow(p, finish, waiting, turnaround) {
    return `
    <tr>
        <td><span class="pid-badge" style="background:${p.color}">${p.label}</span></td>
        <td>${finish}</td>
        <td class="${waiting < 0 ? 'warn' : ''}">${waiting}</td>
        <td>${turnaround}</td>
    </tr>`;
}

// ============================================================
//  Gantt Chart Renderer (generic — target any wrapper element)
// ============================================================
function renderGanttInto(container, segments, processMap) {
    container.innerHTML = '';
    if (!segments.length) return;

    const totalTime = segments[segments.length - 1].end;
    const CHART_W   = container.clientWidth || 420;

    const chart = document.createElement('div');
    chart.className = 'gantt-chart';
    chart.style.position = 'relative';
    chart.style.width    = '100%';

    const row = document.createElement('div');
    row.className = 'gantt-row';

    segments.forEach(seg => {
        const p     = processMap[seg.pid];
        const label = p ? p.label : 'IDLE';
        const pct   = ((seg.end - seg.start) / totalTime * 100).toFixed(4) + '%';

        const block = document.createElement('div');
        block.className = 'gantt-block';
        block.style.width = pct;
        block.title = `${label}  [${seg.start} → ${seg.end}]`;

        const match = label.match(/^([A-Za-z]+)(\d+)$/);
        block.innerHTML = match ? `${match[1]}<sub>${match[2]}</sub>` : label;

        row.appendChild(block);
    });
    chart.appendChild(row);

    const tickRow = document.createElement('div');
    tickRow.className = 'gantt-ticks';
    const boundaries = new Set();
    boundaries.add(0);
    segments.forEach(s => { boundaries.add(s.start); boundaries.add(s.end); });

    boundaries.forEach(t => {
        const tick = document.createElement('div');
        tick.className = 'tick-mark';
        tick.style.left = (t / totalTime * 100).toFixed(4) + '%';
        tick.textContent = t;
        tickRow.appendChild(tick);
    });

    chart.appendChild(tickRow);
    container.appendChild(chart);
}

// Original single-mode wrapper
function renderGantt(segments, processMap) {
    renderGanttInto(document.getElementById('gantt'), segments, processMap);
}

function renderAverages(results) {
    const n    = results.length;
    const avgW = (results.reduce((s, r) => s + r.waiting, 0) / n).toFixed(2);
    const avgT = (results.reduce((s, r) => s + r.turnaround, 0) / n).toFixed(2);
    document.getElementById('averages').innerHTML = `
        <div class="avg-item">Avg Waiting: <strong>${avgW}</strong></div>
        <div class="avg-sep">·</div>
        <div class="avg-item">Avg Turnaround: <strong>${avgT}</strong></div>
        <div class="avg-sep">·</div>
        <div class="avg-item">Processes: <strong>${n}</strong></div>`;
}

// ============================================================
//  runSimulation — single mode dispatcher
// ============================================================
function runSimulation() {
    const algo         = document.getElementById('algo').value;
    const n            = parseInt(document.getElementById('n').value);
    const needPriority = (algo === 'PRIORITY');

    const processes = getProcesses(n, needPriority, '');
    const procMap   = {};
    processes.forEach(p => procMap[p.pid] = p);

    let segments = [], results = [], predLogLines = [];
    ({ segments, results, predLogLines } = dispatchAlgo(algo, processes, ''));

    let tableHTML = '';
    results.forEach(r => {
        tableHTML += buildResultRow(r.proc, r.finish, r.waiting, r.turnaround, needPriority || algo === 'PSJF');
    });
    document.getElementById('output').innerHTML = tableHTML;

    renderGantt(segments, procMap);
    renderAverages(results);

    const logBox = document.getElementById('predLog');
    if (algo === 'PSJF' && predLogLines && predLogLines.length > 0) {
        document.getElementById('predLogContent').innerHTML =
            predLogLines.map(l => `<div class="log-line">${l}</div>`).join('');
        logBox.style.display = 'block';
    } else {
        logBox.style.display = 'none';
    }

    document.getElementById('resultsSection').style.display = 'block';
    document.getElementById('resultsSection').scrollIntoView({ behavior: 'smooth', block: 'start' });
}

// ============================================================
//  Algorithm dispatcher (shared by single and compare)
// ============================================================
function dispatchAlgo(algo, processes, domPrefix) {
    if (algo === 'FCFS')     return { ...runFCFS(processes), predLogLines: [] };
    if (algo === 'RR')       return { ...runRR(processes, domPrefix), predLogLines: [] };
    if (algo === 'SJF')      return { ...runSJF(processes), predLogLines: [] };
    if (algo === 'SRTF')     return { ...runSRTF(processes), predLogLines: [] };
    if (algo === 'PRIORITY') return { ...runPRIORITY(processes), predLogLines: [] };
    if (algo === 'PSJF')     return runPSJF(processes, domPrefix);
    return { segments: [], results: [], predLogLines: [] };
}

// ============================================================
//  COMPARISON MODE
// ============================================================
function onCmpAlgoChange(side) {
    const algo = document.getElementById(`cmp-algo-${side}`).value;
    document.getElementById(`cmp-quantum-${side}`).style.display      = (algo === 'RR')       ? 'block' : 'none';
    document.getElementById(`cmp-psjf-${side}`).style.display         = (algo === 'PSJF')     ? 'flex'  : 'none';
    document.getElementById(`cmp-priority-note-${side}`).style.display= (algo === 'PRIORITY') ? 'block' : 'none';
}

let _cmpProcessCount = 0;

function generateCompareInputs() {
    const n = parseInt(document.getElementById('cmp-n').value);
    if (!n || n < 1) { alert('Enter a valid number of processes.'); return; }
    _cmpProcessCount = n;

    // Show process table (using FCFS as baseline — priority hidden by default)
    const div = document.getElementById('cmpProcessInputs');
    div.innerHTML = buildProcessRows(n, 'FCFS', 'c');

    document.getElementById('cmpProcessSection').style.display = 'block';
    document.getElementById('cmpAlgoSection').style.display    = 'block';
    document.getElementById('cmpResultsSection').style.display = 'none';
}

function algoLabel(algo, domPrefix) {
    if (algo === 'RR') {
        const q = parseInt(document.getElementById(`cmp-q-${domPrefix}`).value) || 2;
        return `RR (q=${q})`;
    }
    return algo;
}

function runComparison() {
    const n = _cmpProcessCount;
    if (!n) { alert('Generate process table first.'); return; }

    const algoA = document.getElementById('cmp-algo-a').value;
    const algoB = document.getElementById('cmp-algo-b').value;

    // Both algos share same process definitions. Priority read if needed.
    const needPriorityA = (algoA === 'PRIORITY');
    const needPriorityB = (algoB === 'PRIORITY');
    const needPriority  = needPriorityA || needPriorityB;

    // Toggle priority column visibility in shared table
    for (let i = 1; i <= n; i++) {
        const pf = document.getElementById(`cpfield${i}`);
        if (pf) pf.style.display = needPriority ? '' : 'none';
    }

    const procsA = getProcesses(n, needPriorityA, 'c');
    const procsB = getProcesses(n, needPriorityB, 'c');

    // Build process map (same pids)
    const procMapA = {}, procMapB = {};
    procsA.forEach(p => procMapA[p.pid] = p);
    procsB.forEach(p => procMapB[p.pid] = p);

    const resA = dispatchAlgo(algoA, procsA, 'a');
    const resB = dispatchAlgo(algoB, procsB, 'b');

    // Gantt charts
    const labelA = algoLabel(algoA, 'a');
    const labelB = algoLabel(algoB, 'b');
    document.getElementById('cmp-gantt-title-a').textContent = labelA;
    document.getElementById('cmp-gantt-title-b').textContent = labelB;
    document.getElementById('cmp-tbl-title-a').textContent   = labelA;
    document.getElementById('cmp-tbl-title-b').textContent   = labelB;

    renderGanttInto(document.getElementById('cmp-gantt-a'), resA.segments, procMapA);
    renderGanttInto(document.getElementById('cmp-gantt-b'), resB.segments, procMapB);

    // Metrics
    const avgWA = resA.results.reduce((s,r) => s + r.waiting, 0) / n;
    const avgTA = resA.results.reduce((s,r) => s + r.turnaround, 0) / n;
    const maxFA = Math.max(...resA.results.map(r => r.finish));

    const avgWB = resB.results.reduce((s,r) => s + r.waiting, 0) / n;
    const avgTB = resB.results.reduce((s,r) => s + r.turnaround, 0) / n;
    const maxFB = Math.max(...resB.results.map(r => r.finish));

    renderMetricBars(labelA, labelB, avgWA, avgWB, avgTA, avgTB, maxFA, maxFB);
    renderVerdict(labelA, labelB, avgWA, avgWB, avgTA, avgTB, maxFA, maxFB);

    // Tables
    let htmlA = '', htmlB = '';
    resA.results.forEach(r => htmlA += buildCompactResultRow(r.proc, r.finish, r.waiting, r.turnaround));
    resB.results.forEach(r => htmlB += buildCompactResultRow(r.proc, r.finish, r.waiting, r.turnaround));
    document.getElementById('cmp-output-a').innerHTML = htmlA;
    document.getElementById('cmp-output-b').innerHTML = htmlB;

    document.getElementById('cmp-avg-a').innerHTML =
        `<div class="avg-item">Avg Wait: <strong>${avgWA.toFixed(2)}</strong></div>
         <div class="avg-sep">·</div>
         <div class="avg-item">Avg TAT: <strong>${avgTA.toFixed(2)}</strong></div>`;
    document.getElementById('cmp-avg-b').innerHTML =
        `<div class="avg-item">Avg Wait: <strong>${avgWB.toFixed(2)}</strong></div>
         <div class="avg-sep">·</div>
         <div class="avg-item">Avg TAT: <strong>${avgTB.toFixed(2)}</strong></div>`;

    document.getElementById('cmpResultsSection').style.display = 'block';
    document.getElementById('cmpResultsSection').scrollIntoView({ behavior: 'smooth', block: 'start' });
}

// ============================================================
//  Metric bar chart comparison
// ============================================================
function renderMetricBars(labelA, labelB, avgWA, avgWB, avgTA, avgTB, maxFA, maxFB) {
    const metrics = [
        { name: 'Avg Waiting Time',     a: avgWA, b: avgWB, lowerBetter: true },
        { name: 'Avg Turnaround Time',  a: avgTA, b: avgTB, lowerBetter: true },
        { name: 'Completion Time (Max)', a: maxFA, b: maxFB, lowerBetter: true }
    ];

    let html = '';
    metrics.forEach(m => {
        const maxVal = Math.max(m.a, m.b, 0.001);
        const pctA   = (m.a / maxVal * 100).toFixed(1);
        const pctB   = (m.b / maxVal * 100).toFixed(1);
        const winA   = m.lowerBetter ? m.a <= m.b : m.a >= m.b;
        const winB   = m.lowerBetter ? m.b <= m.a : m.b >= m.a;
        const tieA   = m.a === m.b;

        html += `
        <div class="metric-bar-card">
            <div class="metric-bar-title">${m.name}</div>
            <div class="metric-bar-row">
                <span class="metric-bar-label metric-label--a">${labelA}</span>
                <div class="metric-bar-track">
                    <div class="metric-bar-fill metric-fill--a ${(!tieA && winA)?'metric-winner':''}"
                         style="width:${pctA}%">
                    </div>
                </div>
                <span class="metric-bar-value ${(!tieA && winA)?'metric-val-winner--a':''}">${m.a.toFixed(2)} ${(!tieA && winA)?'✓':''}</span>
            </div>
            <div class="metric-bar-row">
                <span class="metric-bar-label metric-label--b">${labelB}</span>
                <div class="metric-bar-track">
                    <div class="metric-bar-fill metric-fill--b ${(!tieA && winB)?'metric-winner':''}"
                         style="width:${pctB}%">
                    </div>
                </div>
                <span class="metric-bar-value ${(!tieA && winB)?'metric-val-winner--b':''}">${m.b.toFixed(2)} ${(!tieA && winB)?'✓':''}</span>
            </div>
        </div>`;
    });
    document.getElementById('metricBars').innerHTML = html;
}

// ============================================================
//  Verdict
// ============================================================
function renderVerdict(labelA, labelB, avgWA, avgWB, avgTA, avgTB, maxFA, maxFB) {
    let scoreA = 0, scoreB = 0;
    if (avgWA < avgWB) scoreA++; else if (avgWB < avgWA) scoreB++;
    if (avgTA < avgTB) scoreA++; else if (avgTB < avgTA) scoreB++;
    if (maxFA < maxFB) scoreA++; else if (maxFB < maxFA) scoreB++;

    let verdictHTML = '';
    if (scoreA === scoreB) {
        verdictHTML = `
        <div class="verdict-icon verdict-icon--tie">═</div>
        <div class="verdict-text">
            <div class="verdict-title">TIE — Both algorithms perform equally</div>
            <div class="verdict-detail">
                ${labelA} and ${labelB} score equally across all three metrics for this process set.
                Consider other factors such as implementation complexity or real-time constraints.
            </div>
        </div>`;
    } else {
        const winner = scoreA > scoreB ? labelA : labelB;
        const loser  = scoreA > scoreB ? labelB : labelA;
        const cls    = scoreA > scoreB ? 'verdict-icon--a' : 'verdict-icon--b';
        const ws     = scoreA > scoreB ? scoreA : scoreB;
        const ls     = scoreA > scoreB ? scoreB : scoreA;

        const reasons = [];
        if (scoreA > scoreB) {
            if (avgWA < avgWB) reasons.push(`lower avg waiting time (${avgWA.toFixed(2)} vs ${avgWB.toFixed(2)})`);
            if (avgTA < avgTB) reasons.push(`lower avg turnaround (${avgTA.toFixed(2)} vs ${avgTB.toFixed(2)})`);
            if (maxFA < maxFB) reasons.push(`faster completion (${maxFA} vs ${maxFB})`);
        } else {
            if (avgWB < avgWA) reasons.push(`lower avg waiting time (${avgWB.toFixed(2)} vs ${avgWA.toFixed(2)})`);
            if (avgTB < avgTA) reasons.push(`lower avg turnaround (${avgTB.toFixed(2)} vs ${avgTA.toFixed(2)})`);
            if (maxFB < maxFA) reasons.push(`faster completion (${maxFB} vs ${maxFA})`);
        }

        verdictHTML = `
        <div class="verdict-icon ${cls}">★</div>
        <div class="verdict-text">
            <div class="verdict-title"><span class="verdict-winner">${winner}</span> is better for this case</div>
            <div class="verdict-detail">
                Wins <strong>${ws}</strong> out of 3 metrics vs <strong>${ls}</strong> for ${loser}.<br>
                Advantages: ${reasons.join('; ')}.
            </div>
        </div>`;
    }
    document.getElementById('verdictBox').innerHTML = verdictHTML;
}

// ============================================================
//  FCFS
// ============================================================
function runFCFS(procs) {
    const sorted = [...procs].sort((a, b) => a.arrival - b.arrival || a.pid - b.pid);
    let time = 0;
    const segments = [], results = [];
    sorted.forEach(p => {
        if (time < p.arrival) time = p.arrival;
        const start = time;
        time += p.burst;
        segments.push({ pid: p.pid, start, end: time });
        results.push({ proc: p, finish: time, waiting: start - p.arrival, turnaround: time - p.arrival });
    });
    return { segments, results };
}

// ============================================================
//  RR — supports both single ('' domPrefix) and compare ('a'/'b')
// ============================================================
function runRR(procs, domPrefix) {
    let qId = 'quantum';
    if (domPrefix === 'a') qId = 'cmp-q-a';
    if (domPrefix === 'b') qId = 'cmp-q-b';
    const quantum = parseInt(document.getElementById(qId).value) || 2;

    const remaining = procs.map(p => ({ ...p, rem: p.burst, finish: 0 }));
    remaining.sort((a, b) => a.arrival - b.arrival || a.pid - b.pid);

    const segments = [];
    let time = 0, i = 0;
    const queue = [];

    while (true) {
        while (i < remaining.length && remaining[i].arrival <= time) queue.push(remaining[i++]);
        if (queue.length === 0) {
            if (i < remaining.length) { time = remaining[i].arrival; continue; }
            else break;
        }
        const p   = queue.shift();
        const run = Math.min(quantum, p.rem);
        const start = time;
        time += run;
        p.rem -= run;
        segments.push({ pid: p.pid, start, end: time });
        while (i < remaining.length && remaining[i].arrival <= time) queue.push(remaining[i++]);
        if (p.rem > 0) queue.push(p);
        else p.finish = time;
    }

    const results = remaining.map(p => ({
        proc: p, finish: p.finish,
        waiting: p.finish - p.arrival - p.burst, turnaround: p.finish - p.arrival
    }));
    return { segments, results };
}

// ============================================================
//  SJF
// ============================================================
function runSJF(procs) {
    const remaining = [...procs];
    let time = 0;
    const segments = [], results = [];
    while (remaining.length > 0) {
        const available = remaining.filter(p => p.arrival <= time);
        if (available.length === 0) { time = Math.min(...remaining.map(p => p.arrival)); continue; }
        available.sort((a, b) => a.burst - b.burst || a.pid - b.pid);
        const p = available[0];
        remaining.splice(remaining.indexOf(p), 1);
        const start = time;
        time += p.burst;
        segments.push({ pid: p.pid, start, end: time });
        results.push({ proc: p, finish: time, waiting: start - p.arrival, turnaround: time - p.arrival });
    }
    return { segments, results };
}

// ============================================================
//  SRTF
// ============================================================
function runSRTF(procs) {
    const ps = procs.map(p => ({ ...p, rem: p.burst, finish: 0 }));
    const segments = [];
    let time = 0, done = 0;
    while (done < ps.length) {
        const avail = ps.filter(p => p.arrival <= time && p.rem > 0);
        if (avail.length === 0) { time++; continue; }
        avail.sort((a, b) => a.rem - b.rem || a.pid - b.pid);
        const p = avail[0];
        const nextArr = ps.filter(q => q.arrival > time && q.rem > 0)
            .reduce((m, q) => Math.min(m, q.arrival), Infinity);
        const runUntil = Math.min(time + p.rem, isFinite(nextArr) ? nextArr : time + p.rem);
        const dt = runUntil - time;
        if (segments.length > 0 && segments[segments.length - 1].pid === p.pid) {
            segments[segments.length - 1].end = runUntil;
        } else {
            segments.push({ pid: p.pid, start: time, end: runUntil });
        }
        p.rem -= dt;
        time = runUntil;
        if (p.rem === 0) { p.finish = time; done++; }
    }
    const results = ps.map(p => ({
        proc: p, finish: p.finish,
        waiting: p.finish - p.arrival - p.burst, turnaround: p.finish - p.arrival
    }));
    return { segments, results };
}

// ============================================================
//  PRIORITY
// ============================================================
function runPRIORITY(procs) {
    const remaining = [...procs];
    let time = 0;
    const segments = [], results = [];
    while (remaining.length > 0) {
        const available = remaining.filter(p => p.arrival <= time);
        if (available.length === 0) { time = Math.min(...remaining.map(p => p.arrival)); continue; }
        available.sort((a, b) => a.priority - b.priority || a.pid - b.pid);
        const p = available[0];
        remaining.splice(remaining.indexOf(p), 1);
        const start = time;
        time += p.burst;
        segments.push({ pid: p.pid, start, end: time });
        results.push({ proc: p, finish: time, waiting: start - p.arrival, turnaround: time - p.arrival });
    }
    return { segments, results };
}

// ============================================================
//  PSJF — supports both single ('' domPrefix) and compare ('a'/'b')
// ============================================================
function runPSJF(procs, domPrefix) {
    let alphaId = 'alpha', tau0Id = 'tau0';
    if (domPrefix === 'a') { alphaId = 'cmp-alpha-a'; tau0Id = 'cmp-tau0-a'; }
    if (domPrefix === 'b') { alphaId = 'cmp-alpha-b'; tau0Id = 'cmp-tau0-b'; }
    const alpha = parseFloat(document.getElementById(alphaId).value);
    const tau0  = parseFloat(document.getElementById(tau0Id).value);

    const tau = {}, hasObs = {};
    procs.forEach(p => { tau[p.pid] = tau0; hasObs[p.pid] = false; });

    const remaining = [...procs];
    const predLogLines = [];
    let time = 0;
    const segments = [], results = [];

    while (remaining.length > 0) {
        const available = remaining.filter(p => p.arrival <= time);
        if (available.length === 0) { time = Math.min(...remaining.map(p => p.arrival)); continue; }
        available.sort((a, b) => tau[a.pid] - tau[b.pid] || a.pid - b.pid);
        const p = available[0];
        remaining.splice(remaining.indexOf(p), 1);
        const start = time;
        time += p.burst;
        segments.push({ pid: p.pid, start, end: time });
        const oldTau = tau[p.pid].toFixed(4);
        tau[p.pid] = alpha * p.burst + (1 - alpha) * tau[p.pid];
        hasObs[p.pid] = true;
        predLogLines.push(
            `<b>${p.label}</b> ran [${start}→${time}], actual=${p.burst}, ` +
            `τ_old=${oldTau} → τ_new=<b>${tau[p.pid].toFixed(4)}</b> ` +
            `<span style="opacity:.5">(α=${alpha})</span>`
        );
        results.push({ proc: { ...p, priority: 0 }, finish: time, waiting: start - p.arrival, turnaround: time - p.arrival });
    }
    return { segments, results, predLogLines };
}
