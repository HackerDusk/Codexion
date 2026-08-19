# Exercices ciblés — Jours 1 à 5 (préparation Codexion)

*Liste fermée et volontairement courte : pas d'exercices en trop, juste ceux
qui verrouillent chaque brique. Fais-les dans l'ordre, dans ton dossier
scratch (`~/exercices_codexion/`), un fichier `.c` par exercice.*

**Comment les utiliser** : pour chaque exercice, applique ta méthode avant de
coder (comprendre le problème → modèle mental → simulation à la main →
expliquer en français). Le "critère de réussite" te dit quand tu peux passer
au suivant — pas de solution ici, seulement l'énoncé. Si tu bloques plus de
20-30 min sur un exercice, reviens dans le chat avec ce que tu as essayé.

---

## Jour 1 — Pointeurs, malloc, listes chaînées

### 1.1 — Swap
Écris `void swap(int *a, int *b)` qui échange deux entiers via leurs adresses.
**Critère de réussite** : tu peux dessiner sur papier les 3 boîtes mémoire
(a, b, tmp) et leurs échanges, sans regarder ton code.
**Piège classique** : oublier que passer `a` et `b` par valeur (sans `*`) ne
changerait rien à l'extérieur de la fonction.

### 1.2 — Tableau dynamique
Alloue avec `malloc` un tableau de N entiers (N lu au clavier ou en `argv`),
remplis-le, affiche-le, libère-le avec `free`.
**Critère de réussite** : tu sais expliquer pourquoi ce tableau n'existerait
pas si tu avais fait `int tab[N];` avec N non-constant à la compilation.

### 1.3 — Liste chaînée simple
Implémente `push_front`, `print_all`, `free_all` sur une liste chaînée
d'entiers.
**Critère de réussite** : tu peux tracer à la main (boîtes + flèches) ce que
fait `push_front` sur une liste de 2 éléments, avant d'exécuter le code.
**Cas limites à tester** : liste vide, `free_all` appelé deux fois (observe
ce qui casse — ne corrige pas encore, comprends juste pourquoi c'est cassé).

### 1.4 — La vraie question (pas de code, à l'écrit)
`number_of_coders` est connu dès le lancement du programme (`argv`), avant
que le moindre thread démarre. Rédige 3-4 lignes : dans ce cas précis, un
tableau alloué une fois suffit-il, ou une liste chaînée apporte-t-elle un
vrai avantage ? Justifie.
**Critère de réussite** : ta réponse tient compte du fait que la taille est
connue à l'avance — pas une réponse générique "les listes chaînées c'est
mieux/moins bien".

---

## Jour 2 — Threads & mutex

### 2.1 — Hello threads
Lance N threads avec `pthread_create`, chacun affiche son propre numéro
(passé via l'argument `void *`), puis `pthread_join` tous.
**Piège classique** : passer l'adresse d'une variable de boucle (`&i`)
directement sans copie — tous les threads risquent d'afficher le même
numéro (ou n'importe quoi). Cherche pourquoi.

### 2.2 — La race condition (l'exercice qui compte le plus)
Deux threads incrémentent un `int` partagé 2 000 000 de fois chacun chacun,
**sans aucune protection**. Affiche le résultat final. Relance le programme
5 fois.
**Critère de réussite** : tu observes un résultat différent (et faux) à
chaque run, et tu peux expliquer en français pourquoi une simple
instruction `compteur++` n'est pas atomique.

### 2.3 — Le fix
Reprends 2.2, ajoute un `pthread_mutex_t` autour de l'incrémentation.
**Critère de réussite** : le résultat est maintenant stable et correct sur
5 runs consécutifs.

### 2.4 — Mini-dongle
Simule un seul dongle : une struct avec un `pthread_mutex_t` et un booléen
`libre`. Deux threads essaient de le "prendre" (boucle avec petites pauses),
celui qui réussit logue `"Thread X a pris le dongle"`.
**Critère de réussite** : jamais les deux threads n'affichent qu'ils l'ont
pris en même temps, même en relançant plusieurs fois.

---

## Jour 3 — Condition variables + le temps en C

### 3.1 — Chrono
Utilise `gettimeofday` deux fois, calcule le delta en millisecondes entre
les deux appels, affiche-le. Ajoute un `usleep` entre les deux et vérifie
que le delta mesuré correspond à peu près.

### 3.2 — Busy-wait (à voir une fois, jamais à reproduire)
Un thread boucle indéfiniment en testant un flag partagé (sans dormir, sans
cond var) jusqu'à ce qu'un autre thread le mette à 1.
**Critère de réussite** : tu observes (via `top`/`htop` ou équivalent) que
ce thread consomme un cœur CPU à 100% pendant l'attente. C'est exactement
ce que les condition variables permettent d'éviter.

### 3.3 — Producteur-consommateur, une case
Un producteur remplit une case partagée, un consommateur la vide, en
alternance, avec `pthread_cond_wait`/`pthread_cond_broadcast` (ou `signal`).
**Critère de réussite** : ça tourne sans busy-wait (CPU proche de 0% pendant
les attentes) et sans jamais lire une case vide ou écraser une case pleine.

### 3.4 — Timedwait avec échéance
Utilise `pthread_cond_timedwait` avec une échéance absolue calculée comme
`gettimeofday() + X ms`. Mesure le temps réellement attendu si personne ne
signale la condition, compare-le à X.
**Critère de réussite** : l'écart entre le temps mesuré et X est faible
(quelques ms) — c'est le mécanisme qui te donnera la précision de 10 ms
exigée pour la détection de burnout.

---

## Jour 4 — Deadlocks (Coffman) + tas binaire

### 4.1 — Provoque un deadlock, puis répare-le
Deux threads, deux mutex (A et B). Thread 1 prend A puis essaie de prendre
B. Thread 2 prend B puis essaie de prendre A, en même temps.
**Étape 1** : observe le programme freeze (deadlock réel), tue-le (Ctrl+C).
**Étape 2** : répare en forçant les deux threads à acquérir les mutex dans
le même ordre (ex. toujours A avant B).
**Critère de réussite** : tu peux dire quelle condition de Coffman tu as
cassée en changeant l'ordre d'acquisition.

### 4.2 — Tas à la main
Sur papier : insère les valeurs 7, 3, 9, 1, 5 une par une dans un tas min
(dessine l'arbre après chaque insertion), puis fais `extract-min` deux fois
(dessine l'arbre après chaque extraction).
**Critère de réussite** : ton dernier arbre dessiné respecte la propriété
de tas (chaque parent ≤ ses enfants) sans que tu aies eu besoin de vérifier
avec du code.

### 4.3 — Tas générique en code
Implémente un tas sur tableau (`push`, `pop_min`) avec un comparateur passé
en paramètre (function pointer). Teste-le deux fois sur les mêmes données :
une fois avec un comparateur croissant, une fois décroissant.
**Critère de réussite** : le même code de tas donne un ordre différent
juste en changeant le comparateur — c'est exactement le mécanisme qui te
servira pour basculer entre FIFO et EDF sans dupliquer le tas.

---

## Jour 5 — FIFO vs EDF + modélisation Codexion (à l'écrit, pas de code)

### 5.1 — Simulation papier FIFO
Scénario : 3 coders demandent le même dongle aux instants t=0, t=2, t=5 (ms).
Trace sur papier l'ordre de service sous FIFO.

### 5.2 — Simulation papier EDF
Même scénario, mais chaque coder a en plus un `time_to_burnout` différent,
donc une deadline différente (`last_compile_start + time_to_burnout`).
Refais la trace sous EDF. Identifie le ou les points où l'ordre diverge de
FIFO, et pourquoi.

### 5.3 — État des lieux écrit
Rédige 8-10 lignes en français, sans code :
- la state machine complète d'un coder (les états et les transitions),
- comment le monitor détecte un burnout à moins de 10 ms sans boucler en
  permanence,
- quelle stratégie tu choisis pour empêcher le deadlock (quelle condition de
  Coffman tu casses, et comment concrètement dans ton design).
**Critère de réussite** : ce texte, tu pourras le recopier presque tel quel
dans les sections "Blocking cases handled" et "Thread synchronization
mechanisms" du README exigé par le sujet.

---

Une fois ces cinq blocs solides (tu peux expliquer chaque exercice sans
relire ton code), tu passes au Jour 6 du plan : le vrai repo git.