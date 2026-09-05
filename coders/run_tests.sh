#!/usr/bin/env bash
#
# run_tests.sh — Batterie de tests STRICTE pour le projet "codexion"
#
# Usage:
#   ./run_tests.sh [chemin_vers_binaire]        (défaut: ./codexion)
#
# Ce script suppose que log_validator.py se trouve dans le même dossier.
# Il a besoin de: bash, python3, timeout (coreutils), et optionnellement
# valgrind pour la section mémoire.
#
# Organisation:
#   SECTION A — Arguments invalides (doivent être rejetés proprement)
#   SECTION B — Comportement / correction fonctionnelle (log_validator.py)
#   SECTION C — Valgrind (fuites mémoire, accès invalides)
#   SECTION D — Fuzz "hacker" (arguments tordus, injection, overflow)
#   SECTION E — Stress / répétition anti-flaky (races intermittentes)
#
set -u
BIN="${1:-./codexion}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VALIDATOR="$SCRIPT_DIR/log_validator.py"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; BLUE='\033[0;34m'; NC='\033[0m'

PASS=0
FAIL=0
WARN=0
FAILED_NAMES=()

section() { echo -e "\n${BLUE}=============================================================${NC}"; echo -e "${BLUE}  $1${NC}"; echo -e "${BLUE}=============================================================${NC}"; }
ok()   { PASS=$((PASS+1)); echo -e "${GREEN}✅ PASS${NC} - $1"; }
ko()   { FAIL=$((FAIL+1)); FAILED_NAMES+=("$1"); echo -e "${RED}❌ FAIL${NC} - $1"; [ -n "${2:-}" ] && echo -e "     ${RED}-> $2${NC}"; }
adv()  { WARN=$((WARN+1)); echo -e "${YELLOW}⚠️  ADVISORY${NC} - $1 (comportement ambigu selon le sujet, à vérifier toi-même)"; }

if [ ! -x "$BIN" ]; then
    echo -e "${RED}Binaire introuvable ou non exécutable: $BIN${NC}"
    echo "Usage: $0 [chemin_vers_binaire]"
    exit 1
fi
if ! command -v python3 >/dev/null 2>&1; then
    echo -e "${RED}python3 est requis pour la validation des logs.${NC}"
    exit 1
fi
if ! command -v timeout >/dev/null 2>&1; then
    echo -e "${RED}La commande 'timeout' (coreutils) est requise.${NC}"
    exit 1
fi

crashed() {
    # $1 = code de sortie de `timeout`, $2 = contenu stderr+stdout combiné
    local code="$1" out="$2"
    if [ "$code" -ge 128 ]; then return 0; fi
    if echo "$out" | grep -qiE "segmentation fault|core dumped|bus error|double free|abort|stack smashing"; then return 0; fi
    return 1
}

# ---------------------------------------------------------------------------
# SECTION A — Arguments invalides : doit être rejeté (code retour != 0),
#             SANS jamais crasher.
# ---------------------------------------------------------------------------
section "SECTION A — Rejet des arguments invalides"

run_invalid() {
    local desc="$1"; shift
    local out code
    out=$(timeout 2s "$BIN" "$@" 2>&1)
    code=$?
    if crashed "$code" "$out"; then
        ko "A: $desc" "le programme a CRASHÉ au lieu de rejeter proprement (code=$code)"
    elif [ "$code" -eq 0 ]; then
        ko "A: $desc" "le programme a accepté un argument invalide et a retourné 0"
    elif [ "$code" -eq 124 ]; then
        ko "A: $desc" "le programme a HANG (timeout) au lieu de rejeter l'entrée"
    else
        ok "A: $desc (rejeté, code=$code)"
    fi
}

run_invalid "aucun argument"                     
run_invalid "trop peu d'arguments"                 4 100 100 100 100 3 50
run_invalid "trop d'arguments"                     4 100 100 100 100 3 50 fifo extra
run_invalid "number_of_coders = 0"                 0 100 100 100 100 3 50 fifo
run_invalid "number_of_coders négatif"             -1 100 100 100 100 3 50 fifo
run_invalid "time_to_burnout négatif"              4 -100 100 100 100 3 50 fifo
run_invalid "time_to_compile négatif"              4 100 -1 100 100 3 50 fifo
run_invalid "time_to_debug non-entier (lettres)"   4 100 100 abc 100 3 50 fifo
run_invalid "time_to_refactor flottant"            4 100 100 100 3.14 3 50 fifo
run_invalid "number_of_compiles_required négatif"  4 100 100 100 100 -5 50 fifo
run_invalid "dongle_cooldown négatif"              4 100 100 100 100 3 -1 fifo
run_invalid "scheduler invalide (round_robin)"     4 100 100 100 100 3 50 round_robin
run_invalid "scheduler vide"                       4 100 100 100 100 3 50 ""
run_invalid "number_of_coders énorme (overflow)"   99999999999999999999 100 100 100 100 3 50 fifo
run_invalid "time_to_burnout = INT_MAX+1"          4 2147483648 100 100 100 3 50 fifo
run_invalid "argument vide"                        4 100 "" 100 100 3 50 fifo
run_invalid "scheduler avec tiret parasite (-fifo)" 4 100 100 100 100 3 50 "-fifo"
run_invalid "injection shell dans un argument"     "4; echo pwned" 100 100 100 100 3 50 fifo
run_invalid "argument gigantesque (10000 chiffres)" "$(printf '9%.0s' {1..10000})" 100 100 100 100 3 50 fifo
run_invalid "unicode pleine-largeur (non-ASCII)"   "４" 100 100 100 100 3 50 fifo

# Cas ambigus selon le sujet -> juste informatifs, on ne pénalise pas dur.
out=$(timeout 2s "$BIN" 4 100 100 100 100 3 50 FIFO 2>&1); code=$?
[ "$code" -ne 0 ] && ok "A: scheduler 'FIFO' en majuscules rejeté" || adv "scheduler 'FIFO' en majuscules accepté"

out=$(timeout 2s "$BIN" 4 100 100 100 100 3 50 " fifo" 2>&1); code=$?
crashed "$code" "$out" && ko "A: scheduler avec espace en préfixe" "crash au lieu de rejet propre" || adv "argument ' fifo' (espace) -> comportement à vérifier toi-même"

out=$(timeout 2s "$BIN" "+4" 100 100 100 100 3 50 fifo 2>&1); code=$?
crashed "$code" "$out" && ko "A: nombre positif avec '+'" "crash" || adv "argument '+4' -> accepté ou rejeté selon ton parsing, les deux sont défendables"

# ---------------------------------------------------------------------------
# SECTION B — Comportement fonctionnel (validation de trace stricte)
# ---------------------------------------------------------------------------
section "SECTION B — Correction fonctionnelle (analyse de trace)"

run_valid() {
    # $1=desc $2=timeout_s $3..=coders burnout compile debug refactor required cooldown scheduler
    #   puis: --burnout-tol X --phase-tol Y (optionnels, via variables globales ci-dessous)
    local desc="$1"; local tmo="$2"; shift 2
    local coders=$1 burnout=$2 compile=$3 debug=$4 refactor=$5 required=$6 cooldown=$7 scheduler=$8
    local out code
    out=$(timeout "${tmo}s" "$BIN" "$coders" "$burnout" "$compile" "$debug" "$refactor" "$required" "$cooldown" "$scheduler" 2>&1)
    code=$?
    if crashed "$code" "$out"; then
        ko "B: $desc" "le programme a CRASHÉ (code=$code)"; return
    fi
    if [ "$code" -eq 124 ]; then
        ko "B: $desc" "le programme a HANG (dépassement du timeout ${tmo}s) — deadlock probable"; return
    fi
    local vout vcode
    vout=$(echo "$out" | python3 "$VALIDATOR" --coders "$coders" --burnout "$burnout" \
        --compile "$compile" --debug "$debug" --refactor "$refactor" --required "$required" \
        --cooldown "$cooldown" --burnout-tol "${BURNOUT_TOL:-10}" --phase-tol "${PHASE_TOL:-40}" 2>&1)
    vcode=$?
    if [ "$vcode" -eq 0 ]; then
        ok "B: $desc"
        echo "$vout" | grep -q "⚠️" && { WARN=$((WARN+1)); echo -e "${YELLOW}   (des WARNING existent, voir détail ci-dessous)${NC}"; echo "$vout" | sed 's/^/     /'; }
    else
        ko "B: $desc" "violations détectées par log_validator.py (détail ci-dessous)"
        echo "$vout" | sed 's/^/     /'
    fi
}

BURNOUT_TOL=10 PHASE_TOL=40

# B1: un seul coder = un seul dongle = burnout GARANTI (règle explicite du sujet)
run_valid "1 seul coder -> burnout garanti et précis" 3 1 300 100 50 50 100 20 fifo

# B2: cas nominal simple à 2 coders, doit converger sans burnout
run_valid "2 coders, timings confortables, arrêt propre" 5 2 5000 200 100 100 3 50 fifo

# B3: 5 coders, FIFO, doit satisfaire number_of_compiles_required sans burnout
run_valid "5 coders FIFO, marge large, aucun burnout" 6 5 5000 150 100 100 4 30 fifo

# B4: 5 coders, EDF, même profil -> vérifie le scheduler alternatif
run_valid "5 coders EDF, marge large, aucun burnout" 6 5 5000 150 100 100 4 30 edf

# B5: contention forte, FIFO, cooldown non-nul -> teste la conservation de dongles+cooldown
run_valid "8 coders FIFO, forte contention, cooldown actif" 8 8 4000 120 80 80 3 60 fifo

# B6: idem en EDF -> vérifie la liveness (pas de famine) sous forte contention
run_valid "8 coders EDF, forte contention, pas de famine attendue" 8 8 4000 120 80 80 3 60 edf

# B7: cooldown = 0 (cas limite bas)
run_valid "cooldown = 0 (cas limite)" 5 3 4000 150 100 100 3 0 fifo

# B8: paramètres volontairement infaisables -> burnout ATTENDU, on vérifie juste
#     qu'il est loggé PRÉCISÉMENT et qu'il n'y a ni crash ni hang ni violation de dongles.
adv "B8: paramètres infaisables (cooldown >> burnout) -> un burnout est attendu, ce n'est PAS un bug en soi"
run_valid "params infaisables: burnout précis malgré tout" 5 6 300 100 100 100 100 400 fifo

# ---------------------------------------------------------------------------
# SECTION C — Valgrind (fuites mémoire, accès mémoire invalides)
# ---------------------------------------------------------------------------
section "SECTION C — Valgrind"

if command -v valgrind >/dev/null 2>&1; then
    run_valgrind() {
        local desc="$1"; shift
        local vgout vgcode
        vgout=$(timeout 30s valgrind --error-exitcode=42 --leak-check=full --show-leak-kinds=all \
                --track-origins=yes --errors-for-leak-kinds=all "$BIN" "$@" 2>&1 >/dev/null)
        vgcode=$?
        if [ "$vgcode" -eq 124 ]; then
            ko "C: $desc" "timeout sous valgrind (30s) — trop lent ou deadlock amplifié par le ralentissement"
        elif [ "$vgcode" -eq 42 ]; then
            ko "C: $desc" "valgrind a détecté une erreur mémoire ou une fuite"
            echo "$vgout" | grep -E "definitely lost|indirectly lost|Invalid (read|write)|uninitialised|ERROR SUMMARY" | sed 's/^/     /'
        else
            ok "C: $desc (valgrind propre)"
        fi
    }
    run_valgrind "1 coder, burnout rapide (chemin d'arrêt anticipé)" 1 300 100 50 50 100 20 fifo
    run_valgrind "3 coders, cycle complet, cooldown actif" 3 3000 100 80 80 2 40 fifo
    run_valgrind "3 coders, EDF, cycle complet" 3 3000 100 80 80 2 40 edf
else
    echo -e "${YELLOW}valgrind non installé, section C ignorée.${NC}"
fi

# ---------------------------------------------------------------------------
# SECTION D — Fuzz "armée complète" : arguments tordus, on vérifie SEULEMENT
#             l'absence de crash/hang (la validité du rejet est secondaire ici).
# ---------------------------------------------------------------------------
section "SECTION D — Fuzz anti-crash (armée complète)"

FUZZ_ARGS_SETS=(
    "2147483647 2147483647 2147483647 2147483647 2147483647 2147483647 2147483647 fifo"
    "1 1 1 1 1 1 1 fifo"
    "1 1 1 1 1 1 1 edf"
    "0 0 0 0 0 0 0 fifo"
    "-2147483648 -2147483648 -2147483648 -2147483648 -2147483648 -2147483648 -2147483648 fifo"
    "3 0 0 0 0 0 0 fifo"
    "3 10 10 10 10 999999999 0 fifo"
    "200 50 10 10 10 2 5 fifo"
    "200 50 10 10 10 2 5 edf"
    "3 100 100 100 100 3 50 fifoo"
    "3 100 100 100 100 3 50 edff"
    "3.0 100 100 100 100 3 50 fifo"
    "3 100 100 100 100 3 50fifo"
)
i=0
for args in "${FUZZ_ARGS_SETS[@]}"; do
    i=$((i+1))
    out=$(timeout 3s "$BIN" $args 2>&1); code=$?
    if crashed "$code" "$out"; then
        ko "D: fuzz #$i ($args)" "CRASH détecté (code=$code)"
    elif [ "$code" -eq 124 ]; then
        ko "D: fuzz #$i ($args)" "HANG détecté (timeout 3s)"
    else
        ok "D: fuzz #$i survit sans crash/hang ($args)"
    fi
done

# Test spécifique 200 coders sous stress court (pas de valgrind ici, trop lent)
out=$(timeout 8s "$BIN" 200 3000 30 20 20 2 10 fifo 2>&1); code=$?
if crashed "$code" "$out"; then
    ko "D: stress 200 coders" "CRASH (code=$code)"
elif [ "$code" -eq 124 ]; then
    ko "D: stress 200 coders" "HANG / trop lent (timeout 8s) -> vérifie la scalabilité de ton scheduler"
else
    echo "$out" | grep -qvE '^[0-9]+ [0-9]+ (has taken a dongle|is compiling|is debugging|is refactoring|burned out)$' \
        && ko "D: stress 200 coders" "des lignes de log ne respectent pas le format strict (voir sortie brute)" \
        || ok "D: stress 200 coders survit, format des logs respecté"
fi

# ---------------------------------------------------------------------------
# SECTION E — Répétitions anti-flaky (les races n'apparaissent pas toujours)
# ---------------------------------------------------------------------------
section "SECTION E — Répétitions anti-flaky (détection de races intermittentes)"

REPEAT=${REPEAT_COUNT:-15}
flaky_fail=0
for r in $(seq 1 "$REPEAT"); do
    out=$(timeout 4s "$BIN" 6 4000 100 80 80 3 40 fifo 2>&1); code=$?
    if crashed "$code" "$out" || [ "$code" -eq 124 ]; then
        flaky_fail=$((flaky_fail+1)); continue
    fi
    echo "$out" | python3 "$VALIDATOR" --coders 6 --burnout 4000 --compile 100 --debug 80 \
        --refactor 80 --required 3 --cooldown 40 --burnout-tol "$BURNOUT_TOL" \
        --phase-tol "$PHASE_TOL" >/dev/null 2>&1 || flaky_fail=$((flaky_fail+1))
done
if [ "$flaky_fail" -eq 0 ]; then
    ok "E: $REPEAT répétitions du scénario 6-coders FIFO, aucune race détectée"
else
    ko "E: répétitions 6-coders FIFO" "$flaky_fail / $REPEAT exécutions ont échoué -> race condition intermittente probable"
fi

# ---------------------------------------------------------------------------
# RÉSUMÉ
# ---------------------------------------------------------------------------
section "RÉSUMÉ"
echo -e "${GREEN}PASS: $PASS${NC}   ${RED}FAIL: $FAIL${NC}   ${YELLOW}ADVISORY/WARN: $WARN${NC}"
if [ "$FAIL" -gt 0 ]; then
    echo -e "\n${RED}Tests en échec:${NC}"
    for n in "${FAILED_NAMES[@]}"; do echo "  - $n"; done
    exit 1
fi
exit 0
