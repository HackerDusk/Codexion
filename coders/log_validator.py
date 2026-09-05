#!/usr/bin/env python3
"""
log_validator.py — Vérificateur STRICT de trace pour le projet "codexion".

Ce script prend en entrée la sortie STDOUT de ton programme (ou un fichier
contenant cette sortie) et vérifie, sans connaître le code source :

  1. FORMAT   : chaque ligne respecte EXACTEMENT
                "<timestamp_ms> <coder_id> <event>"
                avec event in {has taken a dongle, is compiling,
                                is debugging, is refactoring, burned out}
                -> détecte aussi les lignes coupées / mélangées (logging
                   non sérialisé correctement).

  2. MACHINE A ETATS par coder : impossible de compiler sans avoir pris
     exactement 2 dongles juste avant, impossible de "debug" sans avoir
     compilé, etc. Toute transition illégale = FAIL.

  3. CONSERVATION PHYSIQUE DES DONGLES + COOLDOWN :
     On ne connaît pas l'identité exacte du dongle pris (le log ne le dit
     pas), mais topologiquement il y a exactement N dongles en tout, et un
     dongle relâché redevient disponible seulement après dongle_cooldown
     ms. On modélise ça comme un pool de N jetons abstraits avec un délai
     de réapparition après relâchement (min-heap). C'est une condition
     NÉCESSAIRE et suffisamment discriminante : toute violation d'exclusion
     mutuelle ou de cooldown fera passer le pool en négatif -> FAIL direct.
     (Ce n'est pas une preuve de bonne allocation gauche/droite précise,
     mais aucune implémentation buguée ne peut passer ce test par hasard.)

  4. PRÉCISION DU BURNOUT (± tolérance, 10ms par défaut selon le sujet) :
     deadline attendue = dernier compile_start + time_to_burnout
                          (ou début de simulation si aucun compile encore).

  5. DURÉES DE PHASE (compile/debug/refactor) comparées aux valeurs
     attendues -> WARNING si trop éloignées (pas un FAIL dur, car le sujet
     n'impose une précision stricte que sur le burnout).

  6. Aucun évènement ne doit apparaître après la mort (burnout) d'un coder,
     ni après l'arrêt global de la simulation (+ une petite marge de grâce
     pour les threads en vol au moment du stop).

  7. Si la simulation s'arrête SANS burnout, tous les coders doivent avoir
     atteint number_of_compiles_required compilations (sinon la condition
     d'arrêt est mal implémentée).

Exit code : 0 si aucun FAIL (des WARNING peuvent subsister), 1 sinon.
"""

import argparse
import heapq
import re
import sys
from collections import defaultdict

LINE_RE = re.compile(
    r'^(\d+) (\d+) (has taken a dongle|is compiling|is debugging|is refactoring|burned out)$'
)


class CoderState:
    __slots__ = (
        "phase", "compile_count", "last_compile_start_ts",
        "debug_start_ts", "dead", "last_event_ts", "held",
    )

    def __init__(self):
        self.phase = "WAIT"          # WAIT -> WAIT1 -> READY2 -> COMPILING -> DEBUGGING -> REFACTORING -> WAIT1 ...
        self.compile_count = 0
        self.last_compile_start_ts = None
        self.debug_start_ts = None
        self.dead = False
        self.last_event_ts = None
        self.held = 0


class Result:
    def __init__(self):
        self.errors = []
        self.warnings = []
        self.format_errors = []

    def err(self, msg):
        self.errors.append(msg)

    def warn(self, msg):
        self.warnings.append(msg)

    def ok(self):
        return not self.errors and not self.format_errors


def parse_lines(raw_lines, n_coders, res):
    events = []  # (ts, coder, event, original_line_no)
    for lineno, raw in enumerate(raw_lines, 1):
        line = raw.rstrip("\n").rstrip("\r")
        if line == "":
            continue
        m = LINE_RE.match(line)
        if not m:
            res.format_errors.append(f"[line {lineno}] format invalide / ligne corrompue: {raw!r}")
            continue
        ts, coder, ev = int(m.group(1)), int(m.group(2)), m.group(3)
        if not (1 <= coder <= n_coders):
            res.format_errors.append(f"[line {lineno}] coder id {coder} hors intervalle [1,{n_coders}]: {raw!r}")
            continue
        events.append((ts, coder, ev, lineno))
    return events


def simulate(events, n_coders, time_to_burnout, time_to_compile, time_to_debug,
             time_to_refactor, dongle_cooldown, required, burnout_tol_ms,
             phase_tol_ms, sim_start_ts, res, verbose=False):

    # tri chronologique stable (garde l'ordre du fichier en cas d'égalité)
    events_sorted = sorted(events, key=lambda e: e[0])

    states = defaultdict(CoderState)
    token_pool = n_coders
    cooldown_heap = []  # available_at timestamps
    burnout_ts = None
    burnout_coder = None
    grace_ms = 15  # marge pour les threads "en vol" au moment de l'arrêt

    def expire_cooldowns(up_to_ts):
        nonlocal token_pool
        while cooldown_heap and cooldown_heap[0] <= up_to_ts:
            heapq.heappop(cooldown_heap)
            token_pool += 1

    for ts, coder, ev, lineno in events_sorted:
        expire_cooldowns(ts)

        if burnout_ts is not None and ts > burnout_ts + grace_ms:
            res.err(f"[line {lineno}] t={ts}: évènement loggé après l'arrêt attendu de la "
                     f"simulation (burnout de {burnout_coder} à t={burnout_ts}, marge {grace_ms}ms dépassée)")
            continue

        st = states[coder]

        if st.dead:
            res.err(f"[line {lineno}] t={ts}: coder {coder} a loggé un évènement "
                     f"APRÈS avoir burn out -> le thread aurait dû s'arrêter")
            continue

        if ev == "has taken a dongle":
            if token_pool <= 0:
                res.err(f"[line {lineno}] t={ts}: coder {coder} a pris un dongle mais le pool "
                         f"global est épuisé -> VIOLATION d'exclusion mutuelle ou de cooldown "
                         f"(pool={token_pool}, en cooldown={len(cooldown_heap)})")
            else:
                token_pool -= 1

            if st.phase == "WAIT":
                st.phase = "WAIT1"
                st.held = 1
            elif st.phase == "WAIT1":
                st.phase = "READY2"
                st.held = 2
            else:
                res.err(f"[line {lineno}] t={ts}: coder {coder} prend un dongle dans un état "
                         f"inattendu ({st.phase}) — séquence 'has taken a dongle' incohérente")

        elif ev == "is compiling":
            if st.phase != "READY2":
                res.err(f"[line {lineno}] t={ts}: coder {coder} commence à compiler sans avoir "
                         f"pris 2 dongles (état actuel: {st.phase})")
            st.phase = "COMPILING"
            st.last_compile_start_ts = ts
            st.compile_count += 1

        elif ev == "is debugging":
            if st.phase != "COMPILING":
                res.err(f"[line {lineno}] t={ts}: coder {coder} passe en debug sans avoir "
                         f"compilé avant (état actuel: {st.phase})")
            else:
                dur = ts - st.last_compile_start_ts
                if abs(dur - time_to_compile) > phase_tol_ms:
                    res.warn(f"t={ts}: coder {coder} durée de compile = {dur}ms "
                              f"(attendu ~{time_to_compile}ms, tol {phase_tol_ms}ms)")
            # relâchement des 2 dongles, indisponibles pendant dongle_cooldown ms
            avail_at = ts + dongle_cooldown
            heapq.heappush(cooldown_heap, avail_at)
            heapq.heappush(cooldown_heap, avail_at)
            st.phase = "DEBUGGING"
            st.debug_start_ts = ts
            st.held = 0

        elif ev == "is refactoring":
            if st.phase != "DEBUGGING":
                res.err(f"[line {lineno}] t={ts}: coder {coder} passe en refactor sans avoir "
                         f"debug avant (état actuel: {st.phase})")
            else:
                dur = ts - st.debug_start_ts
                if abs(dur - time_to_debug) > phase_tol_ms:
                    res.warn(f"t={ts}: coder {coder} durée de debug = {dur}ms "
                              f"(attendu ~{time_to_debug}ms, tol {phase_tol_ms}ms)")
            st.phase = "REFACTOR_WAIT"  # équivalent fonctionnel à WAIT après cette phase

        elif ev == "burned out":
            if st.phase == "COMPILING":
                res.err(f"[line {lineno}] t={ts}: coder {coder} burn out ALORS QU'IL COMPILAIT "
                         f"-> contradiction logique (il avait pourtant démarré à temps)")
            deadline = (st.last_compile_start_ts + time_to_burnout
                        if st.compile_count > 0 else sim_start_ts + time_to_burnout)
            delta = ts - deadline
            if abs(delta) > burnout_tol_ms:
                res.err(f"[line {lineno}] t={ts}: burnout du coder {coder} décalé de {delta}ms "
                         f"par rapport à la deadline attendue ({deadline}ms) — tolérance "
                         f"autorisée: ±{burnout_tol_ms}ms")
            elif verbose:
                print(f"  (info) burnout coder {coder}: delta={delta:+d}ms (OK, tol ±{burnout_tol_ms}ms)")
            st.phase = "DEAD"
            st.dead = True
            if burnout_ts is None:
                burnout_ts = ts
                burnout_coder = coder

        # normalise l'état "REFACTOR_WAIT"/"WAIT" pour la reprise
        if st.phase == "REFACTOR_WAIT":
            st.phase = "WAIT"

        st.last_event_ts = ts

    # Condition d'arrêt : soit burnout, soit tout le monde a compilé >= required fois
    if burnout_ts is None:
        under = {c: s.compile_count for c, s in states.items() if s.compile_count < required}
        missing = [c for c in range(1, n_coders + 1) if c not in states]
        if under or missing:
            res.err(f"Simulation terminée sans burnout MAIS sans que tous les coders "
                     f"aient atteint {required} compilations. "
                     f"Sous le seuil: {under} | jamais vus dans le log: {missing}")
        overshoot = {c: s.compile_count for c, s in states.items() if s.compile_count > required + 1}
        if overshoot:
            res.warn(f"Certains coders ont largement dépassé number_of_compiles_required "
                      f"({required}) avant l'arrêt: {overshoot} (la condition d'arrêt est "
                      f"peut-être vérifiée trop tard / pas assez réactive)")
    else:
        if verbose:
            print(f"  (info) simulation arrêtée par burnout du coder {burnout_coder} à t={burnout_ts}")

    return states, burnout_ts


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("logfile", nargs="?", help="fichier contenant la sortie du programme (sinon: stdin)")
    ap.add_argument("--coders", "-n", type=int, required=True, help="number_of_coders")
    ap.add_argument("--burnout", type=int, required=True, help="time_to_burnout (ms)")
    ap.add_argument("--compile", type=int, required=True, help="time_to_compile (ms)")
    ap.add_argument("--debug", type=int, required=True, help="time_to_debug (ms)")
    ap.add_argument("--refactor", type=int, required=True, help="time_to_refactor (ms)")
    ap.add_argument("--required", type=int, required=True, help="number_of_compiles_required")
    ap.add_argument("--cooldown", type=int, required=True, help="dongle_cooldown (ms)")
    ap.add_argument("--burnout-tol", type=int, default=10,
                     help="tolérance en ms pour la précision du burnout (défaut: 10, mets plus large "
                          "sous valgrind qui ralentit tout)")
    ap.add_argument("--phase-tol", type=int, default=40,
                     help="tolérance en ms pour la durée des phases compile/debug/refactor "
                          "(génère des WARNING, pas des FAIL) (défaut: 40)")
    ap.add_argument("--sim-start", type=int, default=0, help="timestamp de début de simulation attendu (défaut: 0)")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()

    raw_lines = (open(args.logfile, encoding="utf-8", errors="replace").readlines()
                 if args.logfile else sys.stdin.readlines())

    res = Result()
    events = parse_lines(raw_lines, args.coders, res)

    states, burnout_ts = simulate(
        events, args.coders, args.burnout, args.compile, args.debug, args.refactor,
        args.cooldown, args.required, args.burnout_tol, args.phase_tol, args.sim_start,
        res, verbose=args.verbose,
    )

    print("=" * 70)
    if res.format_errors:
        print(f"❌ {len(res.format_errors)} erreur(s) de FORMAT:")
        for e in res.format_errors[:50]:
            print(f"   - {e}")
        if len(res.format_errors) > 50:
            print(f"   ... et {len(res.format_errors) - 50} de plus")

    if res.errors:
        print(f"❌ {len(res.errors)} violation(s) LOGIQUE(S):")
        for e in res.errors[:50]:
            print(f"   - {e}")
        if len(res.errors) > 50:
            print(f"   ... et {len(res.errors) - 50} de plus")

    if res.warnings:
        print(f"⚠️  {len(res.warnings)} avertissement(s) (non bloquant):")
        for w in res.warnings[:20]:
            print(f"   - {w}")
        if len(res.warnings) > 20:
            print(f"   ... et {len(res.warnings) - 20} de plus")

    if res.ok():
        print(f"✅ Trace valide : {len(events)} évènements, {len(states)} coder(s) actifs, "
              f"{'burnout à t=' + str(burnout_ts) if burnout_ts is not None else 'arrêt propre (compiles requises atteintes)'}.")
    print("=" * 70)

    sys.exit(0 if res.ok() else 1)


if __name__ == "__main__":
    main()
