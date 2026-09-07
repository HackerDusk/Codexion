#!/usr/bin/env bash
#
# run_tests_extra.sh — Batterie COMPLÉMENTAIRE pour "codexion"
#
# À poser à côté de run_tests.sh et log_validator.py (même dossier).
# Usage: ./run_tests_extra.sh [chemin_vers_binaire]
#
# Cible spécifiquement la classe de bug "actions après burnout" que
# run_tests.sh ne couvre pas encore (aucun de ses scénarios n'utilise
# un time_to_burnout proche de 0, qui est justement le cas qui révèle
# une race d'ordonnancement de threads au démarrage).
#
#   SECTION F — Burnout "zéro grâce" (deadline atteinte dès t=0)
#   SECTION G — Arrêt immédiat sans burnout (number_of_compiles_required=0)
#   SECTION H — Single coder sous tension (jamais 2 dongles disponibles)
#   SECTION I — Combos aléatoires "chaos" (mini fuzz temporel)
#
set -u
BIN="${1:-./codexion}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VALIDATOR="$SCRIPT_DIR/log_validator.py"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; BLUE='\033[0;34m'; NC='\033[0m'

PASS=0
FAIL=0
FAILED_NAMES=()

section() { echo -e "\n${BLUE}=============================================================${NC}"; echo -e "${BLUE}  $1${NC}"; echo -e "${BLUE}=============================================================${NC}"; }
ok()   { PASS=$((PASS+1)); echo -e "${GREEN}✅ PASS${NC} - $1"; }
ko()   { FAIL=$((FAIL+1)); FAILED_NAMES+=("$1"); echo -e "${RED}❌ FAIL${NC} - $1"; [ -n "${2:-}" ] && echo -e "     ${RED}-> $2${NC}"; }

if [ ! -x "$BIN" ]; then
    echo -e "${RED}Binaire introuvable ou non exécutable: $BIN${NC}"; exit 1
fi
if [ ! -f "$VALIDATOR" ]; then
    echo -e "${RED}log_validator.py introuvable à côté de ce script.${NC}"; exit 1
fi

crashed() {
    local code="$1" out="$2"
    if [ "$code" -ge 128 ]; then return 0; fi
    if echo "$out" | grep -qiE "segmentation fault|core dumped|bus error|double free|abort|stack smashing"; then return 0; fi
    return 1
}

# ---------------------------------------------------------------------------
# SECTION F — Burnout "zéro grâce": time_to_burnout tellement petit que la
# deadline est déjà due au tout premier instant. C'est le cas qui expose une
# course entre "le moniteur commence à surveiller" et "un coder fonce prendre
# un dongle avant que quiconque surveille". Répété N fois par config car
# une race intermittente peut très bien ne jamais se montrer sur un seul run.
# ---------------------------------------------------------------------------
section "SECTION F — Burnout zéro-grâce (deadline due dès t=0)"

run_zero_grace() {
    local coders="$1" burnout="$2" compile="$3" debug="$4" refactor="$5" \
          required="$6" cooldown="$7" sched="$8" reps="$9" label="${10}"
    local fails=0 crash_fails=0
    for i in $(seq 1 "$reps"); do
        out=$(timeout 3 "$BIN" "$coders" "$burnout" "$compile" "$debug" "$refactor" "$required" "$cooldown" "$sched" 2>&1)
        code=$?
        if crashed "$code" "$out"; then
            crash_fails=$((crash_fails+1))
            continue
        fi
        if [ "$code" -eq 124 ]; then
            crash_fails=$((crash_fails+1))
            continue
        fi
        res=$(echo "$out" | python3 "$VALIDATOR" --coders "$coders" --burnout "$burnout" \
              --compile "$compile" --debug "$debug" --refactor "$refactor" \
              --required "$required" --cooldown "$cooldown" 2>&1)
        if echo "$res" | grep -q "❌"; then
            fails=$((fails+1))
            if [ "$fails" -eq 1 ]; then
                FIRST_FAIL_OUT="$out"
                FIRST_FAIL_RES="$res"
            fi
        fi
    done
    if [ "$crash_fails" -gt 0 ]; then
        ko "$label ($crash_fails/$reps crash ou hang)" "voir manuellement: $BIN $coders $burnout $compile $debug $refactor $required $cooldown $sched"
    elif [ "$fails" -gt 0 ]; then
        ko "$label ($fails/$reps violations détectées par log_validator.py)" "premier échec -> $(echo "$FIRST_FAIL_RES" | grep -m1 -- '-')"
    else
        ok "$label ($reps/$reps runs propres)"
    fi
}

run_zero_grace 3 0 200 200 200 5 10 edf   40 "F1: 3 coders, burnout=0, EDF"
run_zero_grace 3 0 200 200 200 5 10 fifo  40 "F2: 3 coders, burnout=0, FIFO"
run_zero_grace 5 0 100 100 100 3 15 edf   30 "F3: 5 coders, burnout=0, EDF, forte contention"
run_zero_grace 8 1 100 100 100 3 5  fifo  30 "F4: 8 coders, burnout=1ms, FIFO"
run_zero_grace 4 2 150 100 100 3 20 edf   30 "F5: 4 coders, burnout=2ms, EDF"
run_zero_grace 1 0 200 200 200 3 10 fifo  30 "F6: 1 coder, burnout=0"

# ---------------------------------------------------------------------------
# SECTION G — number_of_compiles_required=0: la condition d'arrêt est déjà
# satisfaite pour tout le monde à l'instant 0 (0 compile faite >= 0 requise).
# Rien ne devrait JAMAIS être loggé: aucun coder n'a de raison de bouger.
# ---------------------------------------------------------------------------
section "SECTION G — Arrêt immédiat (number_of_compiles_required=0)"

run_required_zero() {
    local coders="$1" burnout="$2" sched="$3" reps="$4" label="$5"
    local bad=0
    for i in $(seq 1 "$reps"); do
        out=$(timeout 2 "$BIN" "$coders" "$burnout" 100 100 100 0 10 "$sched" 2>&1)
        code=$?
        if crashed "$code" "$out" || [ "$code" -eq 124 ]; then
            bad=$((bad+1)); continue
        fi
        if [ -n "$out" ]; then
            bad=$((bad+1))
            [ "$bad" -eq 1 ] && FIRST_BAD_OUT="$out"
        fi
    done
    if [ "$bad" -gt 0 ]; then
        ko "$label ($bad/$reps runs avec des lignes inattendues ou un crash/hang)" "exemple de sortie non-vide: $FIRST_BAD_OUT"
    else
        ok "$label ($reps/$reps: sortie vide, arrêt exit 0, comme attendu)"
    fi
}

run_required_zero 3 500 fifo 25 "G1: 3 coders, required=0, marge confortable"
run_required_zero 3 0   edf  25 "G2: 3 coders, required=0 ET burnout=0 (double cas limite)"
run_required_zero 6 50  fifo 20 "G3: 6 coders, required=0, burnout serré"

# ---------------------------------------------------------------------------
# SECTION H — Single coder: avec 1 seul coder il n'y a qu'1 seul dongle sur
# la table (règle du sujet), donc il ne peut JAMAIS réunir les 2 dongles
# nécessaires pour compiler. Il doit finir par "burned out" avec précision,
# sans jamais logger "is compiling", sans hang, sans double burnout.
# ---------------------------------------------------------------------------
section "SECTION H — Single coder sous tension (jamais 2 dongles dispo)"

for burnout in 5 20 100 300; do
    out=$(timeout 3 "$BIN" 1 "$burnout" 50 50 50 5 10 fifo 2>&1)
    code=$?
    label="H: 1 coder, burnout=${burnout}ms"
    if crashed "$code" "$out" || [ "$code" -eq 124 ]; then
        ko "$label" "crash ou hang: code=$code"
        continue
    fi
    n_compiling=$(echo "$out" | grep -c "is compiling")
    n_burned=$(echo "$out" | grep -c "burned out")
    if [ "$n_compiling" -ne 0 ]; then
        ko "$label" "a réussi à compiler alors qu'un seul coder ne peut jamais avoir 2 dongles (n_compiling=$n_compiling)"
    elif [ "$n_burned" -ne 1 ]; then
        ko "$label" "attendu exactement 1 ligne 'burned out', trouvé $n_burned"
    else
        res=$(echo "$out" | python3 "$VALIDATOR" --coders 1 --burnout "$burnout" --compile 50 --debug 50 --refactor 50 --required 5 --cooldown 10 2>&1)
        if echo "$res" | grep -q "❌"; then
            ko "$label" "$(echo "$res" | grep -m1 -- '-')"
        else
            ok "$label (burnout précis, aucune tentative de compile)"
        fi
    fi
done

# ---------------------------------------------------------------------------
# SECTION I — Chaos: combos aléatoires de petits burnout / cooldown / coders,
# histoire de couvrir des zones que je n'ai pas pensé à tester explicitement.
# ---------------------------------------------------------------------------
section "SECTION I — Chaos aléatoire (mini fuzz temporel)"

CHAOS_FAILS=0
CHAOS_RUNS=40
for i in $(seq 1 "$CHAOS_RUNS"); do
    coders=$(( (RANDOM % 6) + 1 ))
    burnout=$(( RANDOM % 4 ))                  # 0..3 ms : zone à risque
    compile=$(( (RANDOM % 150) + 20 ))
    debug=$(( (RANDOM % 150) + 20 ))
    refactor=$(( (RANDOM % 150) + 20 ))
    required=$(( (RANDOM % 4) + 1 ))
    cooldown=$(( RANDOM % 20 ))
    sched=$([ $((RANDOM % 2)) -eq 0 ] && echo "fifo" || echo "edf")

    out=$(timeout 3 "$BIN" "$coders" "$burnout" "$compile" "$debug" "$refactor" "$required" "$cooldown" "$sched" 2>&1)
    code=$?
    if crashed "$code" "$out" || [ "$code" -eq 124 ]; then
        CHAOS_FAILS=$((CHAOS_FAILS+1))
        echo -e "   ${RED}crash/hang${NC} sur: $coders $burnout $compile $debug $refactor $required $cooldown $sched"
        continue
    fi
    res=$(echo "$out" | python3 "$VALIDATOR" --coders "$coders" --burnout "$burnout" \
          --compile "$compile" --debug "$debug" --refactor "$refactor" \
          --required "$required" --cooldown "$cooldown" 2>&1)
    if echo "$res" | grep -q "❌"; then
        CHAOS_FAILS=$((CHAOS_FAILS+1))
        echo -e "   ${RED}violation${NC} sur: $coders $burnout $compile $debug $refactor $required $cooldown $sched"
        echo "$res" | grep -m2 -- '-' | sed 's/^/      /'
    fi
done
if [ "$CHAOS_FAILS" -gt 0 ]; then
    ko "I: chaos aléatoire ($CHAOS_FAILS/$CHAOS_RUNS combos en échec, détails ci-dessus)"
else
    ok "I: chaos aléatoire ($CHAOS_RUNS/$CHAOS_RUNS combos propres)"
fi

# ---------------------------------------------------------------------------
section "RÉSUMÉ (tests complémentaires)"
echo -e "${GREEN}PASS: $PASS${NC}   ${RED}FAIL: $FAIL${NC}"
if [ "$FAIL" -gt 0 ]; then
    echo -e "\n${RED}Échecs:${NC}"
    for n in "${FAILED_NAMES[@]}"; do echo "  - $n"; done
    exit 1
fi
exit 0
