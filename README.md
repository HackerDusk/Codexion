*This project has been created as part of the 42 curriculum by srandro*

# Codexion

## Table of contents

- [Description](#description)
- [Instructions](#instructions)
- [Available tests](#available-tests)
- [Blocking cases handled](#blocking-cases-handled)
- [Thread synchronization mechanisms](#thread-synchronization-mechanisms)
- [What I've learned](#what-ive-learned)
- [Resources](#resources)

## Description

Codexion simulates `number_of_coders` coder threads competing for a shared pool of
`number_of_coders` USB dongles (one dongle placed between each pair of neighbouring
coders, arranged in a circle). Each coder repeatedly:

1. acquires the dongle on its **left** and the dongle on its **right**,
2. **compiles** for `time_to_compile` ms while holding both dongles,
3. releases both dongles,
4. **debugs** for `time_to_debug` ms,
5. **refactors** for `time_to_refactor` ms,
6. goes back to step 1.

A coder that fails to *start* compiling within `time_to_burnout` ms of its last compile
(or of the start of the simulation) **burns out**, which stops the whole simulation
immediately. The simulation also stops successfully once every coder has compiled at
least `number_of_compiles_required` times.

This is a direct re-skin of the classic **dining philosophers problem** (coders =
philosophers, dongles = forks, compiling = eating). It is a good vehicle to practice
real concurrency engineering rather than toy multithreading: the four conditions for
deadlock (Coffman's conditions) all show up naturally once every coder tries to grab
two shared resources at once, and on top of that the subject adds two real-time-systems
concerns that the textbook version doesn't have — a hard per-coder deadline (burnout)
that a monitor thread must detect within 10 ms, and a pluggable arbitration policy
(FIFO vs. EDF) for who wins a contested dongle.

## Instructions

### Build

```bash
make        # builds ./codexion
make bonus  # builds the bonus part, if any
make clean  # removes object files
make fclean # removes object files + binary
make re     # fclean + all
```

### Run

```bash
./codexion number_of_coders time_to_burnout time_to_compile time_to_debug \
           time_to_refactor number_of_compiles_required dongle_cooldown scheduler
```

| Argument | Unit | Meaning |
|---|---|---|
| `number_of_coders` | count | number of coder threads = number of dongles |
| `time_to_burnout` | ms | time allowed before a coder burns out |
| `time_to_compile` | ms | duration of the compiling phase |
| `time_to_debug` | ms | duration of the debugging phase |
| `time_to_refactor` | ms | duration of the refactoring phase |
| `number_of_compiles_required` | count | simulation stops (success) once every coder reaches this many compiles |
| `dongle_cooldown` | ms | time a dongle stays unavailable after being released |
| `scheduler` | `fifo` \| `edf` | arbitration policy used when several coders request the same dongle |

Example:

```bash
./codexion 4 800 200 100 100 5 10 fifo
```

> **Note on `-pthread`:** the Makefile compiles with `-pthread` in addition to
> `-Wall -Wextra -Werror`. This flag does two things, not just one: it links against
> the POSIX threads implementation (`pthread_create`, `pthread_mutex_*`,
> `pthread_cond_*` would otherwise fail at link time), *and* it defines
> `_REENTRANT`/thread-safety related macros so the C library itself behaves correctly
> when called from multiple threads (e.g. `errno` becomes a thread-local variable
> instead of a single global one shared — and corrupted — by every thread). Passing
> `-lpthread` alone only gets you the first half.

### Implementation notes

- **`number_of_coders` has no artificial upper cap.** The subject only requires
  rejecting negative numbers, non-integers, and values over `INT_MAX` — it does not
  ask for a specific maximum, so we don't invent one. In practice, 200+ coders run
  fine (tested), since each coder/dongle is a small, fixed-size struct. For
  astronomically large values (e.g. `INT_MAX`), the `malloc` calls for the dongles
  or coders arrays will eventually fail because the requested memory is larger than
  what the machine can provide; `monitor_initializer` detects this, frees everything
  it had already allocated, prints a clear error on `stderr`, and exits with code 1 —
  no crash, no leak, no silent failure.
- **Turn dispatch vs. dongle mutual exclusion are two separate layers.** A single
  binary min-heap (`t_heap`, hand-rolled since C89 has no standard priority queue)
  orders *when* a coder is allowed to start requesting its two dongles (FIFO =
  arrival order, EDF = earliest `last_compile_start + time_to_burnout` first, see
  `cmp_fifo`/`cmp_edf`). Actually holding a dongle is a completely separate,
  per-dongle `pthread_mutex_t`/`pthread_cond_t` pair. This means the scheduler never
  serializes compiling itself — it only orders who gets to *try* next — so several
  coders can legitimately be compiling at the same time as long as they're not
  sharing a physical dongle. `⌊number_of_coders / 2⌋` concurrent compiles is the
  normal, expected steady state, not a bug.
- **EDF tie-break:** the submitted `cmp_edf` compares raw deadlines only
  (`last_compile_start + time_to_burnout`); two coders with the exact same
  deadline currently compare as "equal" and the heap doesn't guarantee which one
  goes first.

## Available tests

This is a starting point, not an exhaustive test plan — add your own as you find
edge cases worth locking down.

### Argument parsing

All of the following are rejected: the program prints one line on `stderr` and
exits with code 1, without allocating anything or starting a single thread.

```bash
# Incorrect number of arguments (fewer or more than 8)
./codexion
# -> "Missing arguments, got only 0/8 args." + usage reminder

./codexion 4 800 200 100 100 5 0
# -> "Missing arguments, got only 7/8 args." + usage reminder

# Negative or invalid arguments
./codexion -4 800 200 100 100 5 0 fifo
# -> "-4 must be a POSITIVE INTEGER."

./codexion 4 -800 200 100 100 5 0 fifo
# -> "-800 must be a POSITIVE INTEGER."

./codexion 4 800 200 100 100 5 0 invalid_scheduler
# -> "Scheduler must be exactly one of: \"fifo\" or \"edf\"."

# Non-numeric or floating-point characters
./codexion 4 800abc 200 100 100 5 0 fifo
# -> "800abc must be a POSITIVE INTEGER."

./codexion 4 800.5 200 100 100 5 0 fifo
# -> "800.5 must be a POSITIVE INTEGER."

# Zero coders
./codexion 0 800 200 100 100 5 0 fifo
# -> "number_of_coders must be greater than 0."

# Integer overflow / INT_MAX edge cases
./codexion 4 2147483648 200 100 100 5 0 fifo
# -> "2147483648 must not be over INTMAX."

./codexion 4 800 200 100 100 999999999999999 0 fifo
# -> "999999999999999 must not be over INTMAX."

# Unreasonably large but technically valid number_of_coders
./codexion 2147483647 300 200 100 100 5 0 fifo
# -> allocation fails gracefully: "Error: allocation failed, number_of_coders
#    too large." on stderr, exit code 1, nothing leaked (see Instructions above)
```

### Memory (valgrind --tool=memcheck)

```bash
# Normal termination (number_of_compiles_required reached)
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
  ./codexion 4 800 200 100 100 5 0 fifo

# Termination via burnout
valgrind --leak-check=full --show-leak-kinds=all \
  ./codexion 4 300 200 100 100 5 0 fifo

# Cleanup on an argument error
valgrind --leak-check=full ./codexion 4 -800 200 100 100 5 0 fifo

# Cleanup when allocation itself fails (forces malloc to fail with a tiny
# address-space limit instead of relying on a huge number_of_coders)
valgrind --leak-check=full ./codexion 200000000 300 200 100 100 5 0 fifo
```

Expect `0 bytes in 0 blocks` lost in every case, including the last one — that's
exactly what `free_partial_init()` (see below) exists to guarantee.

### Thread safety (Helgrind / DRD)

```bash
# Helgrind, FIFO scheduling
valgrind --tool=helgrind ./codexion 4 800 200 100 100 5 10 fifo

# Helgrind, EDF scheduling
valgrind --tool=helgrind ./codexion 4 800 200 100 100 5 10 edf
```

> **Note on false positives:** Helgrind's lockset/happens-before algorithm is
> generally reliable as long as every synchronization goes through primitives it
> wraps (`pthread_mutex_*`, `pthread_cond_*`, etc.) — it should not invent races on
> correctly-guarded code. One known exception: recent glibc versions changed the
> internal implementation of `pthread_cond_wait`/`pthread_cond_timedwait`, and
> Helgrind sometimes misreports an internal wake-up inside that implementation as a
> `pthread_cond_{signal,broadcast}: associated lock is not held` error — even though
> the trace never leaves libc/libpthread and involves none of your own code. If DRD
> (a different tool, different instrumentation) reports 0 errors on the exact same
> run, that's a good cross-check that the Helgrind report is this known limitation
> rather than a real bug.

To turn that manual cross-check into an automated smoke test, grep the report for
any stack frame that actually points into one of *our* source files. If nothing
matches, every reported race (if any) is confined to libc/libpthread internals —
i.e. the known false positive above, not a bug in this project:

```bash
valgrind --tool=helgrind ./codexion 4 800 200 100 100 5 10 fifo 2>&1 \
  | grep -E "(main|init|argument_checker|heap|dongle_access|taking_dongle|\
deadlock_breaker|coder_routine|coder_actions|coder_routine_simulator|\
scheduler_routine|scheduler_tools|monitor_routine|monitor_tools|\
simulation_tools|simulator_tools)\.c"
# -> no output = no Helgrind frame touches our code = pass
```

Still double-check that every `pthread_cond_signal`/`pthread_cond_broadcast` you
write yourself is issued while holding the associated mutex — that part is
genuinely under your control and this grep won't catch a mistake there if it
happens to not race during a given run.

### Scenario / edge cases

```bash
# 1. Immediate burnout (time_to_burnout = 0)
./codexion 4 0 200 100 100 5 0 fifo

# 2. Immediate success (number_of_compiles_required = 0)
./codexion 4 800 200 100 100 0 0 fifo

# 3. Single coder (only one dongle exists, compiling is impossible)
./codexion 1 800 200 100 100 5 0 fifo

# 4. High dongle cooldown
./codexion 4 800 200 100 100 5 300 fifo

# 5. Scheduler comparison (FIFO vs EDF)
./codexion 5 800 200 100 100 7 20 fifo
./codexion 5 800 200 100 100 7 20 edf

# 6. Large but reasonable coder count (the kind of value used during evaluation)
./codexion 200 300 200 100 100 5 0 fifo
```

### Log integrity

```bash
# No log line should ever be printed AFTER the "burned out" line
./codexion 4 300 200 100 100 5 0 fifo | grep -A 10 "burned out"
```

## Blocking cases handled

- **Circular wait (Coffman's 4th condition) — the main deadlock risk.**
  Every coder needs two dongles, taken from two other coders that need them too;
  without care this is exactly the textbook full-deadlock scenario (everyone grabs
  their left dongle, then waits forever for their right one). We break the cycle in
  `coffman_circular_wait_breaker()`: coders with an **even** id pick up their
  **left** dongle first, then their right; coders with an **odd** id pick up their
  **right** dongle first, then their left. Because two neighbouring coders always
  have different parity, for every dongle shared between two coders, at least one
  of the two treats it as their *first* pick and the other as their *second* pick.
  It is therefore never possible for every coder in the ring to simultaneously hold
  one dongle and wait for the other — there is always at least one coder that holds
  nothing yet, which breaks the cycle. This holds regardless of whether
  `number_of_coders` is even or odd (verified by hand for both cases, and by
  running the simulation with 3, 4, 5 and 200 coders without any stall).
- **Livelock/starvation at start-up.** Right after start-up all coders race for
  their first dongle at once. `coffman_circular_wait_breaker()` adds a small
  `usleep(1000)` before an even-id coder even attempts its first lock. This is a
  pure scheduling optimization, not a correctness requirement (no lock is held
  during the sleep) — it staggers the even/odd pairs that are about to contend for
  the same first dongle so the contention resolves faster and more predictably
  instead of both threads hammering the same mutex at the exact same instant.
- **Fair arbitration / starvation across the run.** A shared, hand-rolled binary
  heap (`heap.c`) orders coders by arrival time (`fifo`) or by
  `last_compile_start + time_to_burnout` (`edf`) before they are allowed to attempt
  a dongle. A dedicated scheduler thread continuously pops the highest-priority
  waiting coder and grants it a "turn". Under `edf`, a coder that is close to
  burning out is always served before one that just compiled, which is what
  prevents starvation: nobody can be repeatedly skipped while their deadline gets
  closer and closer. When two coders share the exact same deadline, `cmp_edf` falls
  back to the higher `id`, so the comparison is always a strict order and the heap
  never has to make an arbitrary choice.
- **Dongle cooldown.** Each `t_dongle` stores `available_at`
  (`release time + dongle_cooldown`). A coder waiting on a dongle either
  `pthread_cond_wait`s (if the dongle is simply held by someone else) or
  `pthread_cond_timedwait`s until `available_at` (if it's free but still cooling
  down), so cooldown is enforced without ever busy-waiting the CPU.
- **Precise burnout detection.** A dedicated monitor thread never polls: it
  computes the single closest upcoming deadline across all coders
  (`get_closest_deadline`) and calls `pthread_cond_timedwait` for exactly that long.
  On wake-up, if it timed out (`ETIMEDOUT`) it re-checks every coder's real deadline
  (`is_real_burnout`) before declaring a burnout — this two-step check (wait exactly
  until the earliest deadline, then re-verify) is what keeps the reported burnout
  timestamp within the 10 ms tolerance the subject asks for, instead of drifting.
- **Log serialization.** Every `fprintf`/`printf` that writes a state-change line is
  wrapped in `monitor->print_mutex`, so two coder threads (or a coder and the
  monitor) can never interleave two lines into a corrupted one.

## Thread synchronization mechanisms

| Primitive | Protects | Notes |
|---|---|---|
| `dongle_mutex` + `dongle_cond` (per dongle) | `is_free`, `available_at` of one specific dongle | A coder locks it, loops on `pthread_cond_wait`/`timedwait` while the dongle isn't free or still cooling down, then marks it taken. `release_dongles()` sets `available_at` and broadcasts so every waiter re-checks. |
| `scheduler_mutex` + `scheduler_cond` | the turn-dispatch heap, and each coder's `turn` flag | `book_a_slot()` pushes a coder into the heap and wakes the scheduler thread; `scheduler_routine()` pops the highest-priority coder and sets `turn = 1`, broadcasting on that coder's own `turn_cond` so *only* that coder resumes. |
| `monitor_mutex` + `monitor_cond` | the compile/debug/refactor timing loops, and the monitor's own wake-ups | `pthread_cond_timedwait` is used instead of `usleep` for these phases so a burnout or an early simulation stop can interrupt a phase immediately instead of waiting for a fixed sleep to elapse. |
| `stop_mutex` | `monitor->stop_simulation` | The one piece of state read from *every* thread (coders, scheduler, monitor) on every loop iteration; kept in a mutex of its own instead of piggy-backing on `monitor_mutex` to avoid unrelated threads contending on the same lock just to check "are we done?". |
| `print_mutex` | stdout | Held only for the duration of a single `fprintf` call — the shortest possible critical section — so log lines are serialized without adding real contention between coders. |

**Example of a race the locking prevents:** without `dongle_mutex`, two coders could
both read `is_free == 1` on the same dongle, both then set `is_free = 0` and both
believe they hold it — a torn read-modify-write. Because both the read and the write
happen while holding the same per-dongle mutex, only one coder can ever observe
`is_free == 1` and flip it before the other one's read; the loser correctly goes back
to waiting on `dongle_cond` instead of double-taking the resource.

**Example of thread-safe coder ↔ monitor communication:** the monitor never writes
into a coder's compile timers; it only *reads* `compiles_done` and
`last_compile_start` (under `monitor_mutex`, which is also held by the coder while it
updates those same fields in `in_middle_of_compilation()`), and it only ever
*signals* coders back through `wake_coders_up()`/`wake_scheduler_up()`, which
broadcast on the relevant condition variables while holding their matching mutex.
This one-writer-many-readers-under-the-same-lock pattern is what makes it safe for
the monitor to inspect every coder's state at once without a coder being mid-update.

## New things I've learned

| Fonction / notion | Role | When to use it |
|---|---|---|
| `pthread_create(&t, NULL, routine, arg)` | Starts a new thread running `routine(arg)` | Once per coder (and once for the monitor and the scheduler) |
| `pthread_join(t, NULL)` | Waits for a thread to finish before continuing | After the main loop, before exiting cleanly |
| `pthread_mutex_init/lock/unlock/destroy` | Mutual-exclusion lock | Protecting any shared data (a dongle, a counter, console output) |
| `pthread_cond_init/wait/timedwait/signal/broadcast/destroy` | Passive waiting (0% CPU) with a targeted or deadline-based wake-up | `wait`: wait for a resource to be released. `timedwait`: wait until a precise deadline (the burnout monitor) |
| `gettimeofday(&tv, NULL)` | Current time (seconds + microseconds) | Computing ms deltas, building an absolute deadline |
| `usleep(n)` | Sleeps the thread for `n` microseconds (0% CPU during that time) | Simulating a duration (compile/debug/refactor), cooldown |
| `struct timeval` | seconds + microseconds | Output of `gettimeofday` |
| `struct timespec` | seconds + nanoseconds | Input of `pthread_cond_timedwait` (conversion: `tv_usec * 1000`) |
| `ETIMEDOUT` (`<errno.h>`) | Return value of `timedwait` when the deadline has passed | Detecting a burnout |
| `atoi` | Converts a string (`argv`) to an `int` | Parsing the arguments — careful: it doesn't detect errors itself, so negative values/non-integers must be validated by hand (as the subject requires) |
| `fprintf` | Output | Always guarded by a logging mutex so two lines never interleave |

A few things this project actually changed in how I think about concurrency:

- I originally assumed that adding a global priority queue for "who requests a
  dongle next" would necessarily serialize compiling — one of my peers raised
  exactly that concern during peer learning. Testing (and then reading the code
  more carefully) showed the opposite: the heap only orders the very short
  "request a turn" step, and the actual dongle mutexes are per-resource, so
  multiple coders keep compiling in parallel. It made the difference between
  "ordering who asks" and "serializing who gets" very concrete for me.
- `pthread_cond_timedwait` clicked once I saw it used for two very different
  purposes in the same project: waiting for a *resource* (a dongle becoming free
  again after its cooldown) and waiting for a *deadline* (the monitor detecting a
  burnout within 10 ms). Same primitive, same `ETIMEDOUT` check, two different
  mental models.
- Small scheduling details (like the `usleep(1000)` jitter before even-id coders
  attempt their first dongle) don't change correctness but visibly change how the
  simulation behaves under load — a good reminder that "correct" and "well-behaved"
  aren't the same thing, and that some fixes are about liveness/fairness quality
  rather than pure deadlock avoidance.

## Resources

- `man pthread_create`, `man pthread_cond_timedwait`, `man pthread_mutex_lock`
- The dining philosophers problem (Dijkstra), as general background reading:
  [Dining Philosophers Problem — GeeksforGeeks](https://www.geeksforgeeks.org/dining-philosophers-problem/)
- Binary heaps / priority queues, used to hand-roll the FIFO/EDF scheduling queue
  (C89 has no standard library container for this):
  [Heap Data Structure — GeeksforGeeks](https://www.geeksforgeeks.org/dsa/binary-heap/)
- [Coffman conditions](https://faq.computersciencewiki.org/index.php/home/article/coffman-conditions)

- The Valgrind Helgrind manuals

## How AI was used:
 - To find more deep ressources to learn
 - README Skeleton

>The project was developed with a focus on understanding, experimentation, and mainly peer learning.