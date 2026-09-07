#!/usr/bin/env bash
set -u

BIN="${BIN:-./codexion}"
TIMEOUT_SEC="${TIMEOUT_SEC:-5}"
TMP="${TMPDIR:-/tmp}/codexion_tests_$$"

mkdir -p "$TMP"
trap 'rm -rf "$TMP"' EXIT

PASS=0
FAIL=0
SKIP=0

RED=""
GREEN=""
YELLOW=""
RESET=""

if [ -t 1 ]; then
    RED=$'\033[31m'
    GREEN=$'\033[32m'
    YELLOW=$'\033[33m'
    RESET=$'\033[0m'
fi

pass() {
    PASS=$((PASS + 1))
    printf '%s[PASS]%s %s\n' "$GREEN" "$RESET" "$1"
}

fail() {
    FAIL=$((FAIL + 1))
    printf '%s[FAIL]%s %s\n' "$RED" "$RESET" "$1"
    [ -n "${2:-}" ] && printf '       %s\n' "$2"
}

skip() {
    SKIP=$((SKIP + 1))
    printf '%s[SKIP]%s %s\n' "$YELLOW" "$RESET" "$1"
}

section() {
    printf '\n============================================================\n'
    printf '%s\n' "$1"
    printf '============================================================\n'
}

have() {
    command -v "$1" >/dev/null 2>&1
}

run_case() {
    local name="$1"
    shift

    local out="$TMP/out"
    local err="$TMP/err"
    local rc

    : > "$out"
    : > "$err"

    if ! timeout "${TIMEOUT_SEC}s" "$BIN" "$@" >"$out" 2>"$err"; then
        rc=$?
    else
        rc=0
    fi

    printf '%s\n' "$rc" > "$TMP/rc"
}

expect_reject() {
    local name="$1"
    shift

    run_case "$name" "$@"

    # Invalid arguments must not produce a normal simulation.
    if [ ! -s "$TMP/out" ] || grep -qiE 'has taken a dongle|is compiling|is debugging|is refactoring|burned out' "$TMP/out"; then
        fail "$name" "Le programme semble accepter des arguments invalides."
        return
    fi

    pass "$name"
}

expect_success() {
    local name="$1"
    shift

    run_case "$name" "$@"

    local rc
    rc=$(cat "$TMP/rc")

    if [ "$rc" -eq 124 ]; then
        fail "$name" "Timeout: possible deadlock/livelock."
        return
    fi

    if [ "$rc" -ne 0 ]; then
        fail "$name" "Code retour=$rc"
        return
    fi

    pass "$name"
}

parse_common_log() {
    local file="$1"

    awk '
    {
        if ($0 !~ /^[0-9]+ [0-9]+ (has taken a dongle|is compiling|is debugging|is refactoring|burned out)$/) {
            print "BAD_FORMAT:" NR ":" $0
            bad=1
        }

        ts=$1
        id=$2

        if (ts < 0) {
            print "NEGATIVE_TIMESTAMP:" NR
            bad=1
        }

        if (id <= 0) {
            print "INVALID_ID:" NR
            bad=1
        }

        if (prev_ts != "" && ts < prev_ts) {
            print "TIMESTAMP_DECREASE:" NR
            bad=1
        }

        prev_ts=ts
    }

    END {
        if (bad) exit 1
    }' "$file"
}

test_log_format() {
    local name="$1"
    local args="$2"

    echo "$args" | xargs timeout "${TIMEOUT_SEC}s" "$BIN" > "$TMP/out" 2> "$TMP/err"
    local rc=$?

    if [ "$rc" -eq 124 ]; then
        fail "$name" "Timeout."
        return
    fi

    if [ "$rc" -ne 0 ]; then
        fail "$name" "Programme terminé avec rc=$rc."
        return
    fi

    if [ ! -s "$TMP/out" ]; then
        fail "$name" "Aucune sortie."
        return
    fi

    if ! parse_common_log "$TMP/out" > "$TMP/parse_errors"; then
        fail "$name" "$(cat "$TMP/parse_errors")"
        return
    fi

    pass "$name"
}

test_no_output_after_burnout() {
    local args="$1"

    echo "$args" | xargs timeout "${TIMEOUT_SEC}s" "$BIN" > "$TMP/out" 2> "$TMP/err"

    local burnout_line
    burnout_line=$(grep -nE '^[0-9]+ [0-9]+ burned out$' "$TMP/out" | head -1 | cut -d: -f1)

    if [ -z "$burnout_line" ]; then
        fail "Aucun état burnout détectable"
        return
    fi

    local after
    after=$(tail -n +"$((burnout_line + 1))" "$TMP/out")

    if [ -n "$after" ]; then
        fail "Rien après burnout" "$after"
        return
    fi

    pass "Rien après burnout"
}

test_two_dongles_before_compile() {
    local args="$1"

    echo "$args" | xargs timeout "${TIMEOUT_SEC}s" "$BIN" > "$TMP/out" 2> "$TMP/err"

    python3 - "$TMP/out" <<'PY'
import sys
from collections import defaultdict

path = sys.argv[1]

lines = []
with open(path, encoding="utf-8", errors="replace") as f:
    for raw in f:
        raw = raw.strip()
        if raw:
            p = raw.split(maxsplit=3)
            if len(p) == 4:
                lines.append((int(p[0]), int(p[1]), p[2], p[3]))

for i, (ts, cid, action, rest) in enumerate(lines):
    if action == "is" and rest == "compiling":
        before = lines[:i]

        if len(before) < 2:
            print("compile without 2 previous dongle logs")
            sys.exit(1)

        a = before[-2]
        b = before[-1]

        if not (a[2] == "has" and a[3] == "taken a dongle"):
            print("compile not preceded by dongle #1")
            sys.exit(1)

        if not (b[2] == "has" and b[3] == "taken a dongle"):
            print("compile not preceded by dongle #2")
            sys.exit(1)

        if a[1] != cid or b[1] != cid:
            print("wrong coder took dongle before compile")
            sys.exit(1)

print("OK")
PY

    if [ $? -eq 0 ]; then
        pass "Deux dongles avant chaque compilation"
    else
        fail "Deux dongles avant chaque compilation"
    fi
}

test_sequence_per_coder() {
    local args="$1"

    echo "$args" | xargs timeout "${TIMEOUT_SEC}s" "$BIN" > "$TMP/out" 2> "$TMP/err"

    python3 - "$TMP/out" <<'PY'
import sys
from collections import defaultdict

path = sys.argv[1]
state = defaultdict(str)

with open(path, encoding="utf-8", errors="replace") as f:
    for n, raw in enumerate(f, 1):
        raw = raw.strip()
        if not raw:
            continue

        p = raw.split(maxsplit=3)
        if len(p) != 4:
            continue

        cid = int(p[1])
        action = p[2] + " " + p[3]

        old = state[cid]

        if action == "has taken a dongle":
            # allowed from initial/refactoring/waiting state
            pass
        elif action == "is compiling":
            state[cid] = "compiling"
        elif action == "is debugging":
            if old != "compiling":
                print(f"coder {cid}: debugging without compiling (line {n})")
                sys.exit(1)
            state[cid] = "debugging"
        elif action == "is refactoring":
            if old != "debugging":
                print(f"coder {cid}: refactoring without debugging (line {n})")
                sys.exit(1)
            state[cid] = "refactoring"
        elif action == "burned out":
            state[cid] = "burned out"

print("OK")
PY

    if [ $? -eq 0 ]; then
        pass "Séquence compile -> debug -> refactor"
    else
        fail "Séquence compile -> debug -> refactor"
    fi
}

test_no_duplicate_compile_overlap() {
    local args="$1"

    echo "$args" | xargs timeout "${TIMEOUT_SEC}s" "$BIN" > "$TMP/out" 2> "$TMP/err"

    python3 - "$TMP/out" <<'PY'
import sys
from collections import defaultdict

events = defaultdict(list)

with open(sys.argv[1], encoding="utf-8", errors="replace") as f:
    for raw in f:
        p = raw.strip().split(maxsplit=3)
        if len(p) != 4:
            continue
        ts = int(p[0])
        cid = int(p[1])
        action = p[2] + " " + p[3]
        events[cid].append((ts, action))

# We cannot infer exact intervals solely from timestamps,
# but we can ensure each compile has a corresponding debug later.
for cid, evs in events.items():
    compiling = 0

    for ts, action in evs:
        if action == "is compiling":
            compiling += 1
        elif action == "is debugging":
            if compiling <= 0:
                print("debug without compile")
                sys.exit(1)
            compiling -= 1

print("OK")
PY

    if [ $? -eq 0 ]; then
        pass "Pas de cycle de compilation incohérent"
    else
        fail "Pas de cycle de compilation incohérent"
    fi
}

test_quota_termination() {
    local args="$1"
    local required="$2"

    echo "$args" | xargs timeout "${TIMEOUT_SEC}s" "$BIN" > "$TMP/out" 2> "$TMP/err"

    local rc=$?

    if [ "$rc" -eq 124 ]; then
        fail "Arrêt sur quota" "Timeout."
        return
    fi

    if [ "$rc" -ne 0 ]; then
        fail "Arrêt sur quota" "rc=$rc"
        return
    fi

    if grep -qE 'burned out$' "$TMP/out"; then
        fail "Arrêt sur quota" "Un coder a burn out alors que le test devait finir proprement."
        return
    fi

    python3 - "$TMP/out" "$required" <<'PY'
import sys
from collections import Counter

path = sys.argv[1]
required = int(sys.argv[2])

counts = Counter()

with open(path, encoding="utf-8", errors="replace") as f:
    for raw in f:
        p = raw.strip().split(maxsplit=3)
        if len(p) != 4:
            continue
        if p[2] == "is" and p[3] == "compiling":
            counts[int(p[1])] += 1

if not counts:
    print("Aucune compilation")
    sys.exit(1)

bad = {cid: n for cid, n in counts.items() if n < required}
if bad:
    print("Quota non atteint:", bad)
    sys.exit(1)

print("OK")
PY

    if [ $? -eq 0 ]; then
        pass "Arrêt propre après quota"
    else
        fail "Arrêt propre après quota"
    fi
}

test_burnout() {
    local args="$1"

    echo "$args" | xargs timeout "${TIMEOUT_SEC}s" "$BIN" > "$TMP/out" 2> "$TMP/err"

    local rc=$?

    if [ "$rc" -eq 124 ]; then
        fail "Burnout volontaire" "Timeout."
        return
    fi

    if ! grep -qE '^[0-9]+ [0-9]+ burned out$' "$TMP/out"; then
        fail "Burnout volontaire" "Aucun burnout détecté."
        return
    fi

    pass "Burnout détecté"
}

test_burnout_only_once() {
    local args="$1"

    echo "$args" | xargs timeout "${TIMEOUT_SEC}s" "$BIN" > "$TMP/out" 2> "$TMP/err"

    local n
    n=$(grep -cE '^[0-9]+ [0-9]+ burned out$' "$TMP/out" || true)

    if [ "$n" -ne 1 ]; then
        fail "Un seul burnout terminal" "Nombre de burnout=$n"
        return
    fi

    pass "Un seul burnout terminal"
}

test_burnout_time() {
    local args="$1"
    local burnout="$2"

    echo "$args" | xargs timeout "${TIMEOUT_SEC}s" "$BIN" > "$TMP/out" 2> "$TMP/err"

    python3 - "$TMP/out" "$burnout" <<'PY'
import sys

path = sys.argv[1]
deadline = int(sys.argv[2])

burnout_lines = []

with open(path, encoding="utf-8", errors="replace") as f:
    for raw in f:
        p = raw.strip().split(maxsplit=3)
        if len(p) != 4:
            continue
        if p[2] == "burned" and p[3] == "out":
            burnout_lines.append((int(p[0]), int(p[1])))

if not burnout_lines:
    print("No burnout")
    sys.exit(1)

# We only enforce a broad lower bound here because thread scheduling
# and gettimeofday/usleep granularity make exact equality inappropriate.
ts, cid = burnout_lines[0]

if ts > deadline + 50:
    print(f"burnout trop tard: {ts}ms vs deadline {deadline}ms")
    sys.exit(1)

print("OK")
PY

    if [ $? -eq 0 ]; then
        pass "Burnout dans une fenêtre temporelle raisonnable"
    else
        fail "Burnout dans une fenêtre temporelle raisonnable"
    fi
}

test_no_compile_after_quota() {
    local args="$1"
    local required="$2"

    echo "$args" | xargs timeout "${TIMEOUT_SEC}s" "$BIN" > "$TMP/out" 2> "$TMP/err"

    python3 - "$TMP/out" "$required" <<'PY'
import sys
from collections import Counter

path = sys.argv[1]
required = int(sys.argv[2])
counts = Counter()

with open(path, encoding="utf-8", errors="replace") as f:
    lines = [x.strip() for x in f if x.strip()]

for raw in lines:
    p = raw.split(maxsplit=3)
    if len(p) == 4 and p[2] == "is" and p[3] == "compiling":
        counts[int(p[1])] += 1

for cid, n in counts.items():
    if n > required:
        print(f"coder {cid}: {n} compilations > required={required}")
        sys.exit(1)

print("OK")
PY

    if [ $? -eq 0 ]; then
        pass "Pas de compilation au-delà du quota"
    else
        fail "Pas de compilation au-delà du quota"
    fi
}

test_ids() {
    local args="$1"
    local n="$2"

    echo "$args" | xargs timeout "${TIMEOUT_SEC}s" "$BIN" > "$TMP/out" 2> "$TMP/err"

    python3 - "$TMP/out" "$n" <<'PY'
import sys

path = sys.argv[1]
n = int(sys.argv[2])

with open(path, encoding="utf-8", errors="replace") as f:
    for raw in f:
        p = raw.strip().split(maxsplit=3)
        if len(p) != 4:
            continue
        cid = int(p[1])
        if not 1 <= cid <= n:
            print(f"ID invalide: {cid}")
            sys.exit(1)

print("OK")
PY

    if [ $? -eq 0 ]; then
        pass "IDs des coders dans [1..N]"
    else
        fail "IDs des coders dans [1..N]"
    fi
}

test_stderr_clean() {
    local args="$1"

    echo "$args" | xargs timeout "${TIMEOUT_SEC}s" "$BIN" > "$TMP/out" 2> "$TMP/err"

    if [ -s "$TMP/err" ]; then
        fail "stderr propre" "$(head -20 "$TMP/err")"
    else
        pass "stderr propre"
    fi
}

test_no_crash_signatures() {
    local args="$1"

    echo "$args" | xargs timeout "${TIMEOUT_SEC}s" "$BIN" > "$TMP/out" 2> "$TMP/err"

    if grep -qiE \
        'segmentation fault|segfault|bus error|double free|invalid free|abort|assertion failed|core dumped' \
        "$TMP/out" "$TMP/err"; then
        fail "Aucun crash détectable"
        return
    fi

    local rc
    rc=$(cat "$TMP/rc")

    if [ "$rc" -ge 128 ]; then
        fail "Aucun signal fatal" "rc=$rc"
        return
    fi

    pass "Aucun crash détectable"
}

section "0. BUILD"

if [ ! -x "$BIN" ]; then
    printf '%s\n' "Binary introuvable: $BIN"
    printf '%s\n' "Compile d'abord avec:"
    printf '  make re\n'
    exit 1
fi

if ! "$BIN" >/dev/null 2>&1; then
    :
fi

if make -n >/dev/null 2>&1; then
    if make -n 2>/dev/null | grep -q -- '-Wall.*-Wextra.*-Werror'; then
        pass "Makefile contient Wall/Wextra/Werror"
    else
        skip "Impossible de confirmer les flags Makefile automatiquement"
    fi
else
    skip "Pas de Makefile détectable depuis ici"
fi

section "1. ARGUMENTS INVALIDES"

expect_reject "0 arg" 
expect_reject "1 arg" 4
expect_reject "2 args" 4 400
expect_reject "3 args" 4 400 100
expect_reject "4 args" 4 400 100 100
expect_reject "5 args" 4 400 100 100 100
expect_reject "6 args" 4 400 100 100 100 1
expect_reject "7 args" 4 400 100 100 100 1 0
expect_reject "8 args + extra" 4 400 100 100 100 1 0 fifo extra

expect_reject "negative N" -1 400 100 100 100 1 0 fifo
expect_reject "negative burnout" 1 -1 100 100 100 1 0 fifo
expect_reject "negative compile" 1 400 -1 100 100 1 0 fifo
expect_reject "negative debug" 1 400 100 -1 100 1 0 fifo
expect_reject "negative refactor" 1 400 100 100 -1 1 0 fifo
expect_reject "negative required" 1 400 100 100 100 -1 0 fifo
expect_reject "negative cooldown" 1 400 100 100 100 1 -1 fifo

expect_reject "N non-integer" abc 400 100 100 100 1 0 fifo
expect_reject "burnout non-integer" 1 abc 100 100 100 1 0 fifo
expect_reject "compile non-integer" 1 400 abc 100 100 1 0 fifo
expect_reject "debug non-integer" 1 400 100 abc 100 1 0 fifo
expect_reject "refactor non-integer" 1 400 100 100 abc 1 0 fifo
expect_reject "required non-integer" 1 400 100 100 100 abc 0 fifo
expect_reject "cooldown non-integer" 1 400 100 100 100 1 abc fifo

expect_reject "scheduler invalide" 1 400 100 100 100 1 0 random
expect_reject "scheduler FIFO majuscule" 1 400 100 100 100 1 0 FIFO
expect_reject "scheduler vide" 1 400 100 100 100 1 0 ""
expect_reject "N flottant" 2.5 400 100 100 100 1 0 fifo
expect_reject "N signe +" +2 400 100 100 100 1 0 fifo
expect_reject "espace dans integer" " 2" 400 100 100 100 1 0 fifo

section "2. CAS LIMITES"

test_log_format "1 coder, cooldown=0" "1 1000 10 10 10 1 0 fifo"
test_log_format "1 coder, cooldown=1" "1 1000 10 10 10 2 1 fifo"
test_log_format "1 coder, cooldown élevé" "1 5000 10 10 10 1 100 fifo"

test_log_format "2 coders FIFO" "2 2000 10 10 10 2 0 fifo"
test_log_format "2 coders EDF" "2 2000 10 10 10 2 0 edf"

test_log_format "3 coders FIFO" "3 3000 10 10 10 2 0 fifo"
test_log_format "3 coders EDF" "3 3000 10 10 10 2 0 edf"

test_log_format "4 coders FIFO" "4 3000 10 10 10 2 0 fifo"
test_log_format "4 coders EDF" "4 3000 10 10 10 2 0 edf"

test_log_format "8 coders" "8 5000 5 5 5 2 0 fifo"
test_log_format "16 coders" "16 10000 2 2 2 2 0 fifo"

section "3. TIME_TO_BURNOUT = 0"

test_burnout "1 0 10 10 10 1 0 fifo"
test_burnout_only_once "1 0 10 10 10 1 0 fifo"
test_no_output_after_burnout "1 0 10 10 10 1 0 fifo"

section "4. BURNOUT VOLONTAIRE"

test_burnout "2 20 100 100 100 100 0 fifo"
test_burnout_only_once "2 20 100 100 100 100 0 fifo"
test_no_output_after_burnout "2 20 100 100 100 100 0 fifo"

section "5. FORMAT ET ÉTATS"

test_log_format "format 4 coders" "4 3000 20 20 20 2 0 fifo"
test_sequence_per_coder "4 3000 20 20 20 2 0 fifo"
test_ids "4 3000 20 20 20 2 0 fifo" 4
test_two_dongles_before_compile "4 3000 20 20 20 2 0 fifo"
test_no_duplicate_compile_overlap "4 3000 20 20 20 2 0 fifo"
test_no_crash_signatures "4 3000 20 20 20 2 0 fifo"
test_stderr_clean "4 3000 20 20 20 2 0 fifo"

section "6. QUOTA"

test_quota_termination "1 3000 10 10 10 1 0 fifo" 1
test_no_compile_after_quota "1 3000 10 10 10 1 0 fifo" 1

test_quota_termination "2 5000 10 10 10 3 0 fifo" 3
test_no_compile_after_quota "2 5000 10 10 10 3 0 fifo" 3

test_quota_termination "4 8000 5 5 5 3 0 fifo" 3
test_no_compile_after_quota "4 8000 5 5 5 3 0 fifo" 3

test_quota_termination "4 8000 5 5 5 3 0 edf" 3
test_no_compile_after_quota "4 8000 5 5 5 3 0 edf" 3

section "7. COOLDOWN"

test_log_format "cooldown 1ms" "3 5000 5 5 5 3 1 fifo"
test_log_format "cooldown 10ms" "3 5000 5 5 5 3 10 fifo"
test_log_format "cooldown 50ms" "3 10000 5 5 5 2 50 fifo"

section "8. FIFO"

test_log_format "FIFO faible concurrence" "2 2000 20 20 20 5 0 fifo"
test_log_format "FIFO forte concurrence" "8 10000 1 1 1 5 0 fifo"

section "9. EDF"

test_log_format "EDF faible concurrence" "2 2000 20 20 20 5 0 edf"
test_log_format "EDF forte concurrence" "8 10000 1 1 1 5 0 edf"
test_log_format "EDF deadline courte" "8 1000 1 10 10 3 0 edf"

section "10. ADVERSARIAL / STRESS OUTPUT"

# Plusieurs exécutions pour essayer de faire ressortir les races intermittentes.
for i in $(seq 1 10); do
    if ! echo "8 3000 1 1 1 3 0 fifo" | \
        xargs timeout "${TIMEOUT_SEC}s" "$BIN" > "$TMP/stress_$i.out" 2> "$TMP/stress_$i.err"; then
        fail "Stress FIFO #$i" "Crash/timeout."
    elif ! parse_common_log "$TMP/stress_$i.out" >/dev/null; then
        fail "Stress FIFO #$i" "Format/log invalide."
    elif grep -qE 'burned out' "$TMP/stress_$i.out"; then
        fail "Stress FIFO #$i" "Burnout inattendu."
    else
        pass "Stress FIFO #$i"
    fi
done

for i in $(seq 1 10); do
    if ! echo "8 3000 1 1 1 3 0 edf" | \
        xargs timeout "${TIMEOUT_SEC}s" "$BIN" > "$TMP/edf_$i.out" 2> "$TMP/edf_$i.err"; then
        fail "Stress EDF #$i" "Crash/timeout."
    elif ! parse_common_log "$TMP/edf_$i.out" >/dev/null; then
        fail "Stress EDF #$i" "Format/log invalide."
    elif grep -qE 'burned out' "$TMP/edf_$i.out"; then
        fail "Stress EDF #$i" "Burnout inattendu."
    else
        pass "Stress EDF #$i"
    fi
done

section "11. VALGRIND"

if have valgrind; then
    timeout "${TIMEOUT_SEC}s" \
        valgrind \
        --leak-check=full \
        --show-leak-kinds=all \
        --track-origins=yes \
        --error-exitcode=42 \
        "$BIN" 2> "$TMP/valgrind.err" \
        4 5000 5 5 5 2 0 fifo > "$TMP/valgrind.out"

    rc=$?

    if [ "$rc" -eq 42 ]; then
        fail "Valgrind memory check" "Erreur Valgrind détectée."
    elif [ "$rc" -eq 124 ]; then
        fail "Valgrind memory check" "Timeout."
    elif grep -qE \
        'definitely lost: [1-9]|indirectly lost: [1-9]|possibly lost: [1-9]|ERROR SUMMARY: [1-9]' \
        "$TMP/valgrind.err"; then
        fail "Valgrind memory check" "Fuite/erreur mémoire détectée."
    else
        pass "Valgrind memory check"
    fi
else
    skip "Valgrind non installé"
fi

section "12. HELGRIND"

if have valgrind; then
    timeout "${TIMEOUT_SEC}s" \
        valgrind \
        --tool=helgrind \
        --error-exitcode=43 \
        "$BIN" \
        4 5000 5 5 5 3 0 fifo \
        > "$TMP/helgrind.out" \
        2> "$TMP/helgrind.err"

    rc=$?

    if [ "$rc" -eq 43 ]; then
        fail "Helgrind race/deadlock check" "Helgrind rapporte une erreur."
    elif [ "$rc" -eq 124 ]; then
        fail "Helgrind race/deadlock check" "Timeout."
    else
        # Helgrind can print useful warnings without necessarily forcing rc=43
        # depending on version/configuration.
        if grep -qE \
            'Possible data race|data race|conflicting load|conflicting store|lock order' \
            "$TMP/helgrind.err"; then
            fail "Helgrind race/deadlock check" \
                "$(grep -E 'Possible data race|data race|conflicting load|conflicting store|lock order' "$TMP/helgrind.err" | head -10)"
        else
            pass "Helgrind race/deadlock check"
        fi
    fi
else
    skip "Valgrind/Helgrind non installé"
fi

section "13. HEAVY STRESS"

# N plus grand, timings minuscules, quota suffisant pour multiplier
# les acquisitions/releases de dongles.
for scheduler in fifo edf; do
    for n in 8 16 32; do
        args="$n 5000 1 1 1 5 0 $scheduler"

        echo "$args" | xargs timeout "${TIMEOUT_SEC}s" "$BIN" \
            > "$TMP/heavy_${scheduler}_${n}.out" \
            2> "$TMP/heavy_${scheduler}_${n}.err"

        rc=$?

        if [ "$rc" -eq 124 ]; then
            fail "Heavy stress $scheduler N=$n" "Timeout/deadlock."
            continue
        fi

        if [ "$rc" -ne 0 ]; then
            fail "Heavy stress $scheduler N=$n" "rc=$rc"
            continue
        fi

        if ! parse_common_log "$TMP/heavy_${scheduler}_${n}.out" >/dev/null; then
            fail "Heavy stress $scheduler N=$n" "Log invalide."
            continue
        fi

        if grep -qE 'burned out' "$TMP/heavy_${scheduler}_${n}.out"; then
            fail "Heavy stress $scheduler N=$n" "Burnout inattendu."
            continue
        fi

        pass "Heavy stress $scheduler N=$n"
    done
done

section "14. CHECK FINAL"

printf '\n'
printf '==================== RESULT ====================\n'
printf '%sPASS : %d%s\n' "$GREEN" "$PASS" "$RESET"
printf '%sFAIL : %d%s\n' "$RED" "$FAIL" "$RESET"
printf '%sSKIP : %d%s\n' "$YELLOW" "$SKIP" "$RESET"
printf '=================================================\n'

if [ "$FAIL" -eq 0 ]; then
    printf '%s\n' "$GREEN"
    printf 'ALL BLACK-BOX TESTS PASSED.\n'
    printf 'Tu peux passer aux dernières vérifications de soumission.\n'
    printf '%s\n' "$RESET"
    exit 0
else
    printf '%s\n' "$RED"
    printf 'TEST SUITE FAILED.\n'
    printf 'NE PUSH PAS tant que les FAIL ne sont pas expliqués.\n'
    printf '%s\n' "$RESET"
    exit 1
fi