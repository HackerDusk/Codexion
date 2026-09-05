*This project has been created as part of the 42 curriculum by srandro*

# codexion
## Available tests

No test script is shipped with this submission; every check below
was run manually and is fully reproducible with the commands given.

### Build
    make re
Compiles cleanly with `-Wall -Wextra -Werror -pthread`, 0 warning.

### Functional correctness
Traces were inspected for: dongle pool never oversold, cooldown
respected, no event logged after a coder's burnout, burnout
precision within 10ms, correct FIFO/EDF ordering under contention.

    ./codexion 5 3000 150 100 100 3 0 fifo

### Memory safety — Valgrind (Memcheck)
    valgrind --leak-check=full --show-leak-kinds=all \
        ./codexion 5 3000 150 100 100 3 40 fifo
Result: 0 errors, 0 leaks, across light, moderate and high-contention
scenarios (1 to 200 coders).

### Thread safety — Helgrind
    valgrind --tool=helgrind ./codexion 5 3000 150 100 100 3 0 fifo
Detects pthread API misuse, lock-order inversions and data races via
lockset + happens-before analysis.

### Thread safety — ThreadSanitizer (independent cross-check)
    cc -Wall -Wextra -Werror -pthread -fsanitize=thread -g -O1 *.c \
        -o codexion_tsan
    ./codexion_tsan 5 3000 150 100 100 3 0 fifo
Run 15x on the highest-contention scenario: 0 warnings. Used to
cross-validate Helgrind's lockset heuristic, which can misreport on
heavily-shared condition variables under many simultaneous timed
waiters.

### Stress / anti-flaky
Same scenarios repeated 15–40 times (incl. cooldown=0 edge case,
200-coder stress) to rule out intermittent races and confirm
liveness under EDF scheduling.