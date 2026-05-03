# CPU Scheduling Simulator

A full-featured CPU scheduling simulator written in C, with an interactive browser-based visualizer built in HTML/CSS/JS. Supports six scheduling algorithms with a real-time Gantt chart, per-process metrics, and side-by-side algorithm comparison.

---

## Features

- **Six scheduling algorithms** — FCFS, Round Robin, Priority, SJF, SRTF, and Predictive SJF (PSJF)
- **Multi-burst process support** — processes can have multiple CPU bursts with zero I/O time between them
- **Interactive web UI** — live Gantt chart, metrics table, and algorithm comparison mode
- **Predictive scheduling (PSJF)** — exponential averaging (`τ = α·actual + (1−α)·τ_prev`) with configurable `α` and `τ₀`
- **Side-by-side comparison mode** — run two algorithms on the same process set and compare waiting/turnaround times visually
- **CLI simulator** — standalone `.exe` for interactive terminal-based simulation
- **Clean C11 codebase** — modular architecture split across parse, scheduler, metrics, and predictor layers

---

## Algorithms

| Algorithm | Type | Selection Criterion |
|-----------|------|---------------------|
| **FCFS** | Non-preemptive | Arrival order; tie-break by PID |
| **RR** | Preemptive | FIFO queue with configurable time quantum |
| **SJF** | Non-preemptive | Shortest burst (oracle); tie-break by PID |
| **SRTF** | Preemptive | Shortest remaining time; preempts on new arrival |
| **PRIORITY** | Non-preemptive | Smallest priority number = highest urgency; tie-break by PID |
| **PSJF** | Non-preemptive | Predicted next burst via exponential averaging; tie-break by PID |

> **Tie-breaking rule across all algorithms:** lower PID always wins.

---

## Project Structure

```
├── src/
│   ├── main.c          # Entry point; interactive CLI input collection
│   ├── common.c/h      # Shared types: ProcessSpec, GanttChart, ScheduleResult
│   ├── scheduler.c/h   # Simulation engine for all six algorithms
│   ├── metrics.c/h     # Turnaround / waiting time computation and ASCII printing
│   ├── parse.c/h       # File-based input parser (directives: ALGO, QUANTUM, P, ...)
│   └── predictor.c/h   # Exponential-averaging burst predictor for PSJF
├── index.html          # Web UI — single + comparison mode
├── script.js           # Simulation logic re-implemented in JS for the browser
├── style.css           # UI styling
├── sched.exe           # Pre-built Windows binary (x86-64)
└── run.bat             # Build & run script for Windows (auto-detects clang/gcc)
```

---

## Getting Started

### Web Visualizer (no install needed)

Open `index.html` directly in any modern browser. No server required.

```
open index.html        # macOS
start index.html       # Windows
xdg-open index.html    # Linux
```

### CLI Simulator — Windows (pre-built)

Double-click `run.bat`. It will auto-detect your compiler, rebuild if needed, and launch the interactive simulator.

```
run.bat
```

### CLI Simulator — Build from Source

**Requirements:** GCC or Clang with C11 support.

```bash
# Linux / macOS
gcc -std=c11 -Wall -Wextra -O2 -o sched \
    src/main.c src/common.c src/parse.c \
    src/metrics.c src/predictor.c src/scheduler.c

./sched
```

```powershell
# Windows (MinGW / LLVM)
gcc -std=c11 -Wall -Wextra -O2 -o sched.exe ^
    src\main.c src\common.c src\parse.c ^
    src\metrics.c src\predictor.c src\scheduler.c

sched.exe
```

---

## CLI Usage

The simulator prompts interactively:

```
Algorithms: 1=FCFS 2=RR 3=PRIORITY 4=SJF 5=SRTF 6=PSJF
Select algorithm number: 2
Enable multi-burst mode? (1=Yes, 0=No): 0
Quantum: 3
Number of processes: 3

Process P1 (PID: 1)
Input line: 0 8

Process P2 (PID: 2)
Input line: 1 4

Process P3 (PID: 3)
Input line: 2 9
```

**Single-burst input format:** `arrival_time burst_length [priority]`  
**Multi-burst input format:** `arrival_time burst_count burst1 burst2 ... [priority]`  
Priority field is only prompted when the PRIORITY algorithm is selected.

### Sample Output

```
Gantt (pid per time unit, '|' = tick):
|1|1|1|2|2|2|3|3|3|1|1|2|3|3|3|1|1|3|3|3|

Per-process metrics (sorted by pid):
  pid 1: completion=17 turnaround=17 waiting=9
  pid 2: completion=11 turnaround=10 waiting=6
  pid 3: completion=20 turnaround=18 waiting=9
Averages: avg_waiting=8.000 avg_turnaround=15.000
```

---

## File-Based Input (parse.c)

You can also pipe a structured file to the simulator. Supported directives:

```
ALGO   FCFS          # FCFS | RR | SJF | SRTF | PRIORITY | PSJF
QUANTUM 4            # Required for RR
ALPHA  0.5           # Required for PSJF (0.0–1.0)
TAU0   5.0           # Required for PSJF (initial burst estimate)

# P  pid  arrival  burst(s)...  priority
P  1  0  8  0
P  2  1  4  0
P  3  2  9  0
```

Lines beginning with `#` are treated as comments.

---

## PSJF — Predictive Scheduling

PSJF uses exponential averaging to estimate a process's next CPU burst:

```
τ_(n+1) = α · t_n + (1 − α) · τ_n
```

- `α` — weight on the most recent observed burst (0 = ignore history, 1 = use only last burst)
- `τ₀` — initial estimate before any burst is observed

The prediction log is printed during simulation, showing how `τ` evolves per process.

---

## Limits

| Parameter | Limit |
|-----------|-------|
| Max processes | 256 |
| Max bursts per process | 32 |
| Max Gantt segments | 4096 |

---
