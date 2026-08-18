*This project has been created as part of the 42 curriculum by <selouizg>.*

# Codexion

## Description

Codexion is a concurrent programming project written in C, built around one of the classic problems in computer science: the Dining Philosophers problem.

In the original problem, a group of philosophers sit around a circular table. Between each pair of philosophers lies a single fork, and eating requires picking up both the fork on the left and the fork on the right at the same time. Since forks are shared between neighbors, philosophers must compete for them — and if every philosopher grabs their left fork at the same instant and then waits forever for their right one, the whole table freezes: nobody can ever eat again. This is a deadlock, and it's the central challenge the problem is designed to illustrate: how do you let multiple independent actors share limited resources safely, fairly, and without ever getting permanently stuck?

Codexion reframes this exact problem around a modern, relatable scenario: a shared co-working space where coders need USB dongles to compile quantum code. Just like the philosophers and their forks, each coder sits between two dongles — one on their left, one on their right — and compiling requires holding both simultaneously. Coders cycle endlessly through three phases: compiling (which needs both dongles), debugging, and refactoring (neither of which needs any dongle). If a coder goes too long without managing to start a new compile, they burn out, and the whole simulation stops.

### Challenges

This project focuses on several concurrency and resource-sharing challenges:

1. **Deadlock** — preventing circular waiting between coders competing for dongles.
2. **Starvation** — ensuring that a coder is not indefinitely denied access to the dongles.
3. **Fair arbitration** — resolving competition for dongles according to the selected scheduling policy: FIFO or EDF.
4. **Resource cooldown** — handling the period during which a dongle remains unavailable after being released.
5. **Race-free logging** — ensuring that messages from multiple threads are timestamped correctly and printed without interleaving.

### Concurrency Concepts

**Process** — An independent instance of a running program with its own memory space and system resources. Codexion runs as a single process, which contains multiple threads representing the coders.

**Thread** — A lightweight execution path within a process. Threads share the same process memory, which allows them to communicate and access shared resources concurrently.

**Mutex** — A synchronization mechanism used to protect shared resources. Only one thread can hold a mutex at a time, preventing multiple threads from modifying the same resource simultaneously.

**Race Condition** — Occurs when multiple threads access shared data concurrently and the result depends on the order in which the threads execute.

**Deadlock** — Occurs when threads are waiting for resources held by each other, causing all of them to remain blocked indefinitely.

**Starvation** — Occurs when a thread is repeatedly denied access to a resource because other threads are continually given priority.

**Condition Variable** — Allows threads to wait until a particular condition becomes true, while temporarily releasing a mutex. It can then wake waiting threads when the shared state changes.

## Features

- Multithreading with `pthread` (one thread per coder)
- Mutex-based dongle synchronization
- Condition-variable-based waiting queues (no busy-waiting)
- Custom heap-based priority queue driving FIFO/EDF ordering
- Race-free, timestamped logging
- FIFO scheduling
- EDF (Earliest Deadline First) scheduling
- Deadlock prevention through asymmetric dongle acquisition
- Starvation prevention through ordered, queue-based arbitration
- Dongle cooldown enforcement
- Monitor thread for burnout detection and program termination
- Condition variables for coordinating coders waiting for dongles

## Instructions

### Compilation

```bash
make
```

This compiles the project using `cc` with the flags `-Wall -Wextra -Werror -pthread`, producing the `codexion` executable.

Other Makefile rules available: `all`, `clean`, `fclean`, `re`.

### Usage

```bash
./codexion number_of_coders time_to_burnout time_to_compile \
time_to_debug time_to_refactor number_of_compiles_required \
dongle_cooldown scheduler
```

### Arguments

| Argument | Description |
|---|---|
| `number_of_coders` | Number of coders and dongles |
| `time_to_burnout` | Maximum time (ms) before a coder burns out |
| `time_to_compile` | Time (ms) required to compile |
| `time_to_debug` | Time (ms) required to debug |
| `time_to_refactor` | Time (ms) required to refactor |
| `number_of_compiles_required` | Number of compilations required from every coder before the simulation stops successfully |
| `dongle_cooldown` | Cooldown time (ms) before a released dongle can be taken again |
| `scheduler` | `fifo` or `edf` |

### Example

```bash
./codexion 5 3000 200 200 200 10 800 fifo
```

## Scheduling

### FIFO

Coders are served according to the order in which they requested the dongle — first to ask, first to receive it.

### EDF

The coder with the closest burnout deadline (`last_compile_start + time_to_burnout`) is prioritized over a coder that requested the dongle earlier but is in less immediate danger of burning out. If two deadlines are exactly equal, the coder with the lower coder number is served first, guaranteeing a fully deterministic ordering even in that edge case.

## Blocking cases handled

**Deadlock prevention.** Compiling requires a coder to hold both its left and right dongle at once. If every coder always locked its left dongle first and then its right dongle, all coders could simultaneously succeed in locking their left dongle and then block forever waiting on their right one — a circular wait where each coder holds one resource while waiting on a neighbor (all four of Coffman's conditions: mutual exclusion, hold-and-wait, no preemption, and circular wait, would be met). Codexion breaks this circular wait by using a different lock order depending on parity: even-numbered coders lock left, then right; odd-numbered coders lock right, then left. Since neighboring coders now reach for their shared dongle in a different order, the full circular chain can never form.

**Starvation prevention.** A coder never busy-loops or retries a dongle request at random; every request is pushed onto that dongle's heap-based waiting queue and the coder blocks on a `pthread_cond_t` until it is specifically woken. Under FIFO, the heap orders waiters strictly by request-arrival time, so a coder can never be skipped over by a later arrival — it is guaranteed to reach the front of the queue. Under EDF, the heap orders waiters by burnout deadline, and because every coder's deadline strictly decreases as time passes without it compiling, a coder that keeps losing arbitration is guaranteed to eventually hold the earliest deadline of anyone still waiting and be served before it can burn out (provided the supplied parameters are feasible, i.e. the table can physically serve every coder before its deadline).

**Cooldown handling.** Each dongle stores a `last_released` timestamp. When a coder finishes compiling and releases both dongles, that timestamp is updated under the dongle's mutex. A dongle is only considered a valid candidate for arbitration once `current_time - last_released >= dongle_cooldown`; coders whose wait would otherwise be satisfied by a dongle still in cooldown remain queued (via `pthread_cond_timedwait`, timed to wake exactly when the cooldown expires) instead of grabbing it prematurely.

**Precise burnout detection.** A dedicated monitor thread periodically re-checks, under `mutex_for_stop`, every coder's deadline (`last_compile_start + time_to_burnout`). The polling interval is kept short enough (well under the 10 ms tolerance required by the subject) that a missed deadline is detected and logged within 10 ms of the actual burnout, after which the monitor sets the shared `stop` flag so every coder thread exits cleanly.

**Log serialization.** Every state-change log line is wrapped in `pthread_mutex_lock(&config->mutex_for_printing)` / `pthread_mutex_unlock(...)` around the `printf` call, so two threads can never interleave partial output on the same line.

## Thread synchronization mechanisms

Codexion uses the following synchronization primitives, each protecting a distinct piece of shared state:

- **Per-dongle mutex** (`t_dongle.mutex`) — one per dongle, protecting that dongle's own fields (`taken`, `last_released`, `waiter_count`). A coder must lock a dongle's mutex before touching any of its state, and holds it for the duration of the compile phase.
- **Per-dongle condition variable** (`t_dongle.cond`) — paired with the dongle's mutex. A coder that cannot immediately take a dongle is enqueued in that dongle's heap-based waiting queue and calls `pthread_cond_wait` (or `pthread_cond_timedwait` when it is only blocked by an active cooldown), releasing the mutex while it sleeps instead of spinning. When the dongle becomes available, the releasing coder calls `pthread_cond_broadcast` so every waiter re-checks the heap; only the coder now at the top of the heap (per FIFO arrival order or EDF deadline) proceeds, and the rest go back to sleep.
- **Custom heap-based waiting queue** — a binary heap (no standard-library priority queue is used, per the subject's constraint) keyed by arrival timestamp in FIFO mode or by deadline in EDF mode. All heap operations (push on request, pop on grant) happen while the dongle's mutex is held, so the heap itself never needs a separate lock and can't be corrupted by concurrent inserts.
- **Print-lock mutex** (`t_config.mutex_for_printing`) — a single shared mutex guarding every `printf` call for state-change logs, so output from concurrent coder threads never garbles together on the same line.
- **Stop-flag mutex** (`t_config.mutex_for_stop`) — protects the shared `stop` flag, read by every coder thread and written by the monitor thread once burnout or the compile-count goal is detected.

**Example of a race condition this design prevents:** without a per-dongle mutex, two neighboring coder threads could both read a dongle's `taken` field as `0` at nearly the same instant, both conclude it is free, and both proceed to use it simultaneously — the exact "dongle duplication" bug the subject forbids. By requiring the mutex to be locked before the dongle's state is ever read or written, only one thread can ever be inside that check-and-claim sequence at a time.

**Example of thread-safe coordination with the monitor:** the monitor thread never reads a coder's deadline directly off shared memory without synchronization — it reads it under the same mutex the coder thread uses to update `last_compile_start`, so it can never observe a half-written value. Once it detects a burnout, it sets `stop` under `mutex_for_stop` and broadcasts on every dongle's condition variable, waking any coder threads that are asleep waiting for a dongle so they can observe the flag and exit instead of blocking forever.

## Resources

- POSIX Threads Programming (Lawrence Livermore National Laboratory tutorial) — background on `pthread_create`, `pthread_join`, mutexes and condition variables.
- `man` pages: `pthread_create`, `pthread_mutex_lock`, `pthread_cond_wait`, `pthread_cond_timedwait`, `gettimeofday`.
- The classic "Dining Philosophers" problem (Edsger Dijkstra) as background for the deadlock/starvation concepts this project is built around.
- **AI usage**: an AI assistant (Claude) was used throughout this project as a learning aid — to explain concurrency concepts (threads, mutexes, condition variables, deadlock/starvation, Coffman's conditions) before implementation, to review hand-written code and point out specific bugs (without providing ready-made solutions), and to help structure this README. All C code in this repository was written and debugged by hand by the author(s); the AI was not used to generate finished implementation code.