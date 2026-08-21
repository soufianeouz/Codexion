*This project has been created as part of the 42 curriculum by selouizg.*

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

**Busy-waiting (polling)** — A thread that cannot yet proceed repeatedly checks a condition in a loop, sleeping briefly between checks, instead of blocking on a condition variable. Simpler to reason about at the cost of a small fixed CPU/latency overhead. This is the approach Codexion uses everywhere a thread has to wait.

## Features

- Multithreading with `pthread` (one thread per coder, plus a dedicated monitor thread)
- Mutex-based dongle synchronization (one `pthread_mutex_t` per dongle)
- Fixed-size, per-dongle waiting slots driving FIFO/EDF ordering (see *Scheduling* below for why two slots is enough)
- Busy-wait polling (`usleep`-based) everywhere a thread has to wait — no condition variables
- Race-free, timestamped logging
- FIFO scheduling
- EDF (Earliest Deadline First) scheduling
- Deadlock prevention through asymmetric dongle acquisition order
- Dongle cooldown enforcement
- Monitor thread for precise burnout detection and program termination

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

All arguments are mandatory and validated: non-integers, negative numbers, `number_of_coders == 0`, a wrong argument count, or a scheduler other than `fifo`/`edf` are all rejected with an error message.

### Example

```bash
./codexion 5 3000 200 200 200 10 800 fifo
```

## Scheduling

Every dongle is shared by exactly two coders — its left neighbor and its right neighbor (or, in the single-coder case, by that one coder alone). That means a dongle can never have more than two threads waiting on it at once, so instead of a general-purpose priority-queue structure, each dongle stores its (at most two) waiters directly in a fixed two-slot array (`t_coder *queue[2]`), ordered according to the active scheduler.

### FIFO

Coders are inserted into the array in the order they requested the dongle — first to ask occupies slot 0 and is served first.

### EDF

On each request, the coder already in slot 0 is compared against the requesting coder by deadline (`last_compile_start + time_to_burnout`); whichever has the earlier deadline is placed in slot 0. If the two deadlines are exactly equal, the coder already occupying slot 0 keeps its place (arrival order breaks the tie).

## Blocking cases handled

**Deadlock prevention.** Compiling requires a coder to hold both its left and right dongle at once. If every coder always locked its left dongle first and then its right dongle, all coders could simultaneously succeed in locking their left dongle and then block forever waiting on their right one — a circular wait where each coder holds one resource while waiting on a neighbor (all four of Coffman's conditions: mutual exclusion, hold-and-wait, no preemption, and circular wait, would be met). Codexion breaks this circular wait by using a different lock order depending on parity: even-numbered coders lock left, then right; odd-numbered coders lock right, then left. Since neighboring coders now reach for their shared dongle in a different order, the full circular chain can never form. The single-coder case is handled separately: with only one coder there is only one dongle, so that coder can never legitimately acquire two distinct dongles — it simply waits until the monitor thread detects it hasn't compiled in time and stops the simulation.

**Starvation prevention.** Under FIFO, a coder's request is placed in a dongle's waiting array in arrival order and is never skipped by a later arrival, so it is guaranteed to reach the front. Under EDF, a coder's position is re-evaluated by deadline on every request; because a coder's deadline strictly gets closer as time passes without it compiling, a coder that keeps losing arbitration is guaranteed to eventually hold the earliest deadline of anyone still waiting for that dongle (provided the supplied parameters are feasible, i.e. the timings can physically serve every coder before its deadline). This was verified empirically: a 6-coder EDF run with a tight burnout window produced zero burnouts and every coder reached the required compile count.

**Cooldown handling.** Each dongle stores a `last_released` timestamp, updated under that dongle's mutex when a coder finishes compiling. Before a coder is allowed to proceed into a compile, `dongles_ready()` checks that `current_time - last_released >= dongle_cooldown` for both of its dongles; if not, the coder releases the mutexes it's holding and polls again shortly after (`wait_dongles`), rather than compiling with a dongle still in cooldown.

**Precise burnout detection.** A dedicated monitor thread repeatedly checks, under `mutex_for_stop`, every coder's deadline (`last_compile_start + time_to_burnout`) against the current elapsed time. It polls every 1 ms, which keeps the reported burnout time well within the 10 ms tolerance required by the subject — verified directly across repeated runs, where burnout was consistently logged within 0–1 ms of the target `time_to_burnout`.

**Log serialization.** Every state-change log line is wrapped in `pthread_mutex_lock(&config->mutex_for_printing)` / `pthread_mutex_unlock(...)` around the `printf` call, so two threads can never interleave partial output on the same line.

## Thread synchronization mechanisms

Codexion uses three mutexes, each protecting a distinct piece of shared state, and no condition variables — every thread that needs to wait does so by polling with a short `usleep` between checks:

- **Per-dongle mutex** (`t_dongle.mutex`) — one per dongle. A coder must hold both its left and right dongle's mutex for the entire duration of a compile (acquired via `lock_dongles`, using the parity-based ordering described above, and released via `unlock_dongles` once the compile phase ends). While a coder holds a dongle's mutex, no other coder can read or modify that dongle's waiting-slot array or its `last_released` timestamp.
- **`mutex_for_stop`** — protects the shared `stop` flag and every coder's `last_compile_start` / `compile_count` / `state` fields. Both the monitor thread (writing `stop`, reading every coder's deadline) and each coder thread (writing its own state, reading `stop`) always take this lock before touching any of that shared state, so the monitor can never observe a half-written deadline and no coder can race the monitor's decision to stop.
- **`mutex_for_printing`** — a single shared mutex guarding every `printf` call for state-change logs, so output from concurrent coder threads never garbles together on the same line.

**Example of a race condition this design prevents:** without a per-dongle mutex, two neighboring coder threads could both check a dongle's waiting-slot array at nearly the same instant, both conclude they're first in line, and both proceed to compile using the same physical dongle simultaneously. By requiring the mutex to be locked before that array is ever read or written, only one thread can ever be inside that check-and-claim sequence at a time.

**Example of thread-safe coordination with the monitor:** the monitor thread never reads a coder's `last_compile_start` without holding `mutex_for_stop` first — the same lock every coder thread holds while updating that field at the start of a compile. This was a real bug during development: an early version read `stop` in one waiting loop without taking `mutex_for_stop` first, which a data-race checker (`valgrind --tool=helgrind`) flagged as an unsynchronized read racing against the monitor's write. It was fixed by wrapping that read in the same lock used everywhere else `stop` is touched.


## Resources

- POSIX Threads Programming (Lawrence Livermore National Laboratory tutorial) — background on `pthread_create`, `pthread_join`, mutexes and condition variables.
- `man` pages: `pthread_create`, `pthread_mutex_lock`, `pthread_cond_wait`, `pthread_cond_timedwait`, `gettimeofday`.
- The classic "Dining Philosophers" problem (Edsger Dijkstra) as background for the deadlock/starvation concepts this project is built around.
- **AI usage**: an AI assistant (Claude) was used throughout this project as a learning aid — to explain concurrency concepts (threads, mutexes, condition variables, deadlock/starvation, Coffman's conditions) before implementation, to review hand-written code and point out specific bugs (without providing ready-made solutions), and to help structure this README. All C code in this repository was written and debugged by hand by the author(s); the AI was not used to generate finished implementation code.