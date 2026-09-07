*This project has been created as part of the 42 curriculum by srandro*

# Codexion

> TODO: fill in the placeholders below and remove the `> TODO` callouts once a section is complete.

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

> TODO: add 2-3 sentences on why this problem is interesting (adaptation of the dining
> philosophers problem, deadlock/starvation/fairness trade-offs, etc.).

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
> `-Wall -Wextra -Werror`. This flag tells the compiler to link against the POSIX
> threads library and to enable thread-safe variants of some libc internals (e.g.
> `errno` becomes thread-local). Without it, `pthread_create`, `pthread_mutex_*` and
> `pthread_cond_*` would fail to link, and calling libc from multiple threads would
> not be safe.

> TODO: mention anything specific to your implementation (e.g. how the heap/priority
> queue is used for FIFO/EDF, any design choice worth flagging).

## Available tests

> TODO: run every command below, note what you actually observe, and add any test
> you wrote yourself (this is a starting point, not an exhaustive test plan).

### Argument parsing

```bash
# Incorrect number of arguments (fewer or more than 8)
./codexion
./codexion 4 800 200 100 100 5 0

# Negative or invalid arguments
./codexion -4 800 200 100 100 5 0 fifo
./codexion 4 -800 200 100 100 5 0 fifo
./codexion 4 800 200 100 100 5 0 invalid_scheduler

# Non-numeric or floating-point characters
./codexion 4 800abc 200 100 100 5 0 fifo
./codexion 4 800.5 200 100 100 5 0 fifo

# Zero coders
./codexion 0 800 200 100 100 5 0 fifo

# Integer overflow / INT_MAX edge cases
./codexion 2147483647 800 200 100 100 5 0 fifo
./codexion 4 2147483648 200 100 100 5 0 fifo
./codexion 4 800 200 100 100 999999999999999 0 fifo
```

> TODO: document what your program does for each line above (it must reject all of
> them, per the subject's requirement to reject invalid input) — exit code, error
> message, etc.

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
```

### Thread safety (Helgrind / DRD)

```bash
# Helgrind, FIFO scheduling
valgrind --tool=helgrind ./codexion 4 800 200 100 100 5 10 fifo

# Helgrind, EDF scheduling
valgrind --tool=helgrind ./codexion 4 800 200 100 100 5 10 edf

# DRD
valgrind --tool=drd ./codexion 4 800 200 100 100 5 10 fifo
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
> rather than a real bug. Still double-check that every `pthread_cond_signal` /
> `pthread_cond_broadcast` you write yourself is issued while holding the associated
> mutex — that part is genuinely under your control.

> TODO: for every error Helgrind/DRD actually reports on your binary, note whether
> the stack trace touches your own code or only libc/libpthread internals, and
> conclude accordingly.

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
```

### Log integrity

```bash
# No log line should ever be printed AFTER the "burned out" line
./codexion 4 300 200 100 100 5 0 fifo | grep -A 10 "burned out"
```

## Blocking cases handled

> TODO — describe, for your actual implementation:
> - deadlock prevention (which of Coffman's four conditions you break, and how),
> - starvation prevention under both `fifo` and `edf`,
> - how `dongle_cooldown` is enforced,
> - how burnout is detected precisely (within 10 ms) without busy-waiting the CPU,
> - how log lines are serialized so two messages never interleave on one line.

## Thread synchronization mechanisms

> TODO — describe:
> - which `pthread_mutex_t` / `pthread_cond_t` protects which piece of shared state
>   (each dongle, the logger, the monitor/stop flag),
> - how a coder acquires both of its dongles without deadlocking against its
>   neighbours,
> - how the monitor thread detects burnout / is woken up,
> - a concrete example of a race condition your locking scheme prevents.

## What I've learned

> TODO — a short, personal section. Some ideas to start from:
> - what surprised you when implementing FIFO vs EDF scheduling,
> - a mistake you made early on and how you found/fixed it (e.g. via Helgrind/DRD),
> - what you now understand better about condition variables or deadlock avoidance
>   that you didn't before starting this project.

## Resources

> TODO — list what you actually consulted, for example:
> - `man pthread_create`, `man pthread_cond_timedwait`, `man pthread_mutex_lock`
> - the dining philosophers problem (Dijkstra) as background reading
> - Earliest Deadline First scheduling (real-time systems literature)
> - the Valgrind Helgrind/DRD manuals
>
> TODO — describe precisely how AI was used: for which tasks (e.g. debugging a
> specific deadlock, interpreting a Helgrind report, drafting this README) and which
> parts of the project were **not** AI-assisted. Be specific — a vague "used AI for
> everything" (or no mention at all) goes against the spirit of the AI Instructions
> chapter of the subject.
