# Plan Codexion — de zéro pthread à projet soutenable (échéance : dimanche/lundi)

*Généré à partir de ton bilan de compétences actuel (C rouillé, listes chaînées
pas solides, pthread quasi inconnu, EDF pas compris, heap à implémenter en C89,
precision du burnout pas claire). Feuille de route à ajuster selon ton rythme
réel — le but reste celui de ton README : comprendre pour pouvoir réutiliser,
pas cocher des cases pour finir vite.*

## 1. Où t'en es réellement (constat, pas jugement)

- Compréhension du sujet : ~80%. Ce qui manque n'est pas la lecture, c'est la
  mécanique derrière les mots : pourquoi malloc, pourquoi un thread, pourquoi
  un tas, pourquoi EDF plutôt que FIFO.
- C rouillé depuis le switch Python du common core. libft / get_next_line /
  ft_printf / push_swap sont faits mais la mémoire s'est envolée — normal, on
  ne repart pas de zéro, on réactive vite (Jour 1).
- ~90% des External Functions du sujet (tous les `pthread_*`, `gettimeofday`,
  `usleep`) sont inconnues. C'est cohérent : Fly-in était un projet
  algorithmique, Codexion est un projet **système / concurrence**. Ce n'est
  pas la continuité de Fly-in, c'est un nouveau monde.
- Listes chaînées : le "pourquoi du malloc" n'est pas acquis. On le traite
  avant d'écrire une seule struct de Codexion, sinon chaque `malloc` du projet
  restera du copier-coller sans compréhension.
- FIFO, tu l'as déjà vu (BFS, Python) — la moitié du scheduler est un concept
  connu sous un autre nom. EDF est le vrai nouveau concept.

## 2. Le vrai visage du projet

Fly-in = algorithmique pure : ton programme *décide* un plan, puis l'exécute
seul, pas à pas. Codexion = concurrence réelle : plusieurs threads *tournent
en même temps*, se disputent une ressource physique (les dongles), et ton
rôle est d'arbitrer sans jamais les laisser se marcher dessus (race condition)
ni se bloquer mutuellement pour toujours (deadlock).

**Le modèle mental central** : ce projet est une variante du problème des
*philosophes dînants* (dining philosophers problem, Dijkstra 1965) :

| Codexion | Philosophes dînants |
|---|---|
| coder | philosophe |
| dongle | baguette |
| compiler (besoin de 2 dongles) | manger (besoin de 2 baguettes) |
| debug / refactor | penser |
| burnout | mourir de faim |

Toutes les solutions classiques à ce problème (ordre d'acquisition des
ressources, arbitre central, limiter à N-1 acteurs simultanés) s'appliquent
directement ici. Retiens ce lien — il va revenir dans n'importe quel cours ou
projet touchant à l'OS, aux bases de données (locks), ou aux systèmes
distribués. C'est le genre de chose que ton README appelle à "mémoriser
durablement".

## 3. Les briques à poser, dans l'ordre

Chacune s'appuie sur la précédente. Ne saute pas un niveau parce que "c'est
juste un mutex" :

1. Pointeurs & mémoire dynamique (`malloc`/`free`) — pourquoi, stack vs heap
2. Listes chaînées ou tableaux dynamiques — le concept de nœud
3. Threads (`pthread_create`/`join`) — le modèle "plusieurs exécutions en parallèle"
4. Race conditions & mutex (`pthread_mutex_*`)
5. Condition variables (`pthread_cond_*`) — attendre sans consommer de CPU
6. Le temps en C (`gettimeofday`, `usleep`, calcul de delta en ms)
7. Deadlocks & les 4 conditions de Coffman
8. Tas binaire / priority queue — implémentation manuelle (C89 n'a rien de prêt)
9. FIFO vs EDF — même structure, comparateur différent

## 4. Plan jour par jour (ajustable — pas un carcan)

### Jour 1 — Pointeurs, malloc, listes chaînées
*But : que "pourquoi malloc" cesse d'être une question.*
- [ ] Modèle mental : une variable normale, c'est une boîte avec un nom.
  Un pointeur, c'est un post-it avec l'adresse d'une boîte dessus.
- [ ] Exercice à la main : dessine sur papier une liste chaînée de 3 nœuds
  (des boîtes reliées par des flèches), puis simule "insérer en tête" en
  dessinant les flèches qui bougent.
- [ ] Exercice code : `swap(int *a, int *b)` — sans ça, `pthread_mutex_lock`
  n'aura aucun sens (tout pthread carbure aux pointeurs vers des structs).
- [ ] Exercice code : liste chaînée d'entiers — `push_front`, `print_all`,
  `free_all`. Fais-le à la main sur papier d'abord (comme ton README l'exige),
  code ensuite.
- [ ] Question à te poser (pas à moi) avant de coder Codexion : le nombre de
  coders est connu dès le lancement (`argv`). Est-ce qu'une liste chaînée est
  vraiment nécessaire pour représenter tes coders/dongles, ou un simple
  tableau alloué une fois suffit ? Garde ta réponse, on la challenge au Jour 5.

### Jour 2 — Threads & mutex
*But : voir une race condition de tes propres yeux avant qu'on t'explique le mutex.*
- [ ] Modèle mental : plusieurs cuisiniers (threads) dans une cuisine, un
  seul couteau (ressource partagée) sur le plan de travail.
- [ ] Exercice code : `pthread_create`/`pthread_join` — lance N threads qui
  affichent juste leur numéro. Fais varier N comme `number_of_coders`.
- [ ] **L'exercice qui compte** : deux threads incrémentent un compteur
  partagé 1 000 000 de fois chacun, sans protection. Lance-le plusieurs fois,
  regarde le résultat final varier. C'est la preuve expérimentale de la race
  condition — pas une histoire qu'on te raconte.
- [ ] Ajoute un `pthread_mutex_t` autour de l'incrémentation. Relance, le
  résultat devient stable. Explique en français pourquoi.
- [ ] Relie au sujet : c'est exactement le rôle du mutex sur chaque dongle
  (empêcher deux coders de "prendre" le même dongle en même temps).

### Jour 3 — Condition variables + le temps en C
*But : attendre une ressource sans faire du "busy-wait" (boucle qui consomme du CPU pour rien).*
- [ ] Modèle mental : une salle d'attente avec une sonnette. Sans sonnette
  (condition variable), tu dois ouvrir la porte toutes les secondes pour
  vérifier si c'est libre (gaspillage). Avec sonnette, tu dors jusqu'à ce
  qu'on te réveille.
- [ ] Exercice code : producteur-consommateur avec une seule case partagée —
  le producteur attend (`pthread_cond_wait`) que la case soit vide, le
  consommateur attend qu'elle soit pleine. C'est la structure exacte de
  "attendre qu'un dongle se libère".
- [ ] Exercice code : `gettimeofday`, calcule un delta en millisecondes entre
  deux appels, affiche-le. Puis `usleep(x * 1000)`.
- [ ] Exercice code : `pthread_cond_timedwait` avec une échéance absolue
  calculée à partir de `gettimeofday() + time_to_burnout`. C'est directement
  ce dont tu auras besoin pour détecter un burnout à 10 ms près sans boucler
  bêtement.
- [ ] Réponds-toi : pourquoi `printf` sans mutex autour peut donner des lignes
  de log entrelacées entre deux threads ? (Le sujet l'interdit explicitement.)

### Jour 4 — Deadlocks (Coffman) + tas binaire / priority queue
*But : comprendre pourquoi un deadlock arrive, et construire l'outil de tri du scheduler.*
- [ ] Modèle mental : reprends le tableau philosophes/Codexion de la section 2.
  Les 4 conditions de Coffman (exclusion mutuelle, hold-and-wait, pas de
  préemption, attente circulaire) doivent TOUTES être vraies pour qu'un
  deadlock se produise. Casser une seule condition suffit à l'empêcher.
- [ ] Simulation à la main : dessine le scénario classique de deadlock à 2
  coders (chacun prend son dongle gauche, puis attend le droit que l'autre a
  déjà pris). Identifie quelle condition de Coffman tu vas casser dans ton
  design (ordre d'acquisition ? arbitre central ?).
- [ ] Modèle mental du tas : une file d'urgences aux urgences (ER) — le
  patient le plus urgent (deadline la plus proche) passe en premier, pas le
  premier arrivé. Un tas binaire, c'est cette file organisée en arbre pour
  que "trouver le plus urgent" coûte O(log n) au lieu de O(n).
- [ ] Simulation à la main : insère 5 valeurs dans un tas min sur papier,
  fais `extract-min` deux fois, dessine l'arbre à chaque étape.
- [ ] Exercice code : tas binaire sur tableau (`push`, `pop_min`) avec un
  comparateur passé en paramètre (function pointer) — comme ça la même
  structure sert pour FIFO (clé = timestamp d'arrivée) et EDF (clé =
  deadline), sans dupliquer le code.
- [ ] Relie explicitement à ton PLAN Fly-in : c'est le même motif que le tas
  de Dijkstra ("toujours traiter le nœud le moins coûteux en premier"), sauf
  que la clé n'est plus un coût cumulé mais un timestamp ou une deadline. Tu
  ne réapprends pas un concept, tu le généralises.

### Jour 5 — FIFO vs EDF + modélisation complète de Codexion (à la main)
*But : avant d'écrire codexion.c, être capable de tracer une simulation entière sur papier.*
- [ ] Simulation à la main : 3 coders, quelques requêtes de dongle à des
  instants différents. Trace qui obtient le dongle en premier sous FIFO, puis
  refais le même scénario sous EDF (deadline = `last_compile_start +
  time_to_burnout`). Observe où les deux divergent.
- [ ] Explique en français (pas en code) : pourquoi EDF garantit la
  "liveness" (pas de famine) si les paramètres sont faisables, et ce que
  "faisable" veut dire concrètement.
- [ ] Reviens sur ta réponse du Jour 1 (liste chaînée vs tableau) — tranche
  maintenant que tu connais le tas et sa structure sous-jacente.
- [ ] Modélise en français la state machine d'un coder : compiling → debugging
  → refactoring → (retente de compiler). Précise où et comment il communique
  avec les dongles et avec le thread moniteur.
- [ ] Design du monitor thread : comment détecter un burnout à moins de 10 ms
  sans faire une boucle qui poll en permanence (indice : `pthread_cond_timedwait`
  sur la deadline la plus proche parmi tous les coders, pas un sleep fixe).

### Jour 6 — Implémentation
*But : traduire le raisonnement du Jour 5, pas en improviser un nouveau.*
Ordre conseillé (chaque étape testée avant la suivante) :
- [ ] Parsing des arguments (rejeter négatifs, non-entiers, scheduler invalide)
- [ ] Struct dongle (mutex + cond + état + cooldown) + `take`/`release`
- [ ] Struct coder (thread + state machine) — sans scheduler d'abord, juste
  FIFO naïf, pour valider le cycle compile/debug/refactor
- [ ] Priority queue générique (celle du Jour 4) branchée sur les dongles
- [ ] Scheduler FIFO complet, puis EDF (même code, comparateur différent)
- [ ] Monitor thread pour le burnout (précision 10 ms)
- [ ] Logger avec mutex (aucune ligne entrelacée)
- [ ] `Makefile` avec les règles obligatoires + flags `-Wall -Wextra -Werror -pthread`

### Jour 7 (tampon) — Tests, revue, README
- [ ] Teste avec `valgrind` (fuites) et `helgrind` ou ThreadSanitizer (races)
  si disponible — les yeux ne voient pas les race conditions, les outils oui.
- [ ] Vérifie la Norm (le sujet l'exige, 0 si erreur de Norm).
- [ ] Rejoue le tableau de simulation manuelle du Jour 5 contre la vraie sortie
  du programme — ça doit correspondre.
- [ ] Rédige les sections README obligatoires ("Blocking cases handled",
  "Thread synchronization mechanisms") **en réutilisant tes notes des
  Jours 2 à 4** — si tu as bien tenu ton raisonnement en français à chaque
  étape, cette section s'écrit toute seule, tu ne repars pas de zéro.
- [ ] Auto-test "recode" : ferme le fichier, explique à voix haute pourquoi
  tu as choisi ce scheduler, cette structure de dongle. Si tu bloques, ce
  n'est pas encore compris — retourne à l'étape concernée.

## 5. Ce qu'on ne fait pas avant la deadline

- Le mode graphique / affichage coloré avancé au-delà du minimum (le sujet
  n'exige qu'un feedback visuel, terminal coloré simple suffit).
- Les benchmarks de performance "bonus" (pas dans ce projet — pas de niveau
  bonus documenté dans le sujet Codexion, contrairement à Fly-in).
- L'optimisation fine du tas (au-delà de O(log n) correct, pas la peine de
  chercher plus rapide pour l'instant).

Rien ci-dessus n'est interdit plus tard, juste hors du chemin critique pour
rendre un projet qui fonctionne et que tu maîtrises d'ici dimanche/lundi.

## 6. Rappel méthode (ton propre README, appliqué à chaque ligne ci-dessus)

Pour chaque brique : 1. Comprendre le problème → 2. Modèle mental (métaphore)
→ 3. Simulation à la main → 4. Expliquer en français → 5. Implémenter →
6. Tester → 7. Critiquer → 8. Optimiser → 9. Généraliser. Jamais l'inverse —
surtout pas pour les mutex/cond var, où sauter à l'implémentation garantit un
code qui "marche par hasard" et qui s'effondre à la moindre question en
soutenance.

## 7. Lien avec ton `PLAN_curriculum_apres_Fly-in.md`

Le tas binaire que tu vas construire ici (Jour 4) est directement réutilisable
pour Horizon 2 de ce plan-là (Dijkstra, Cheapest Flights, Path with Minimum
Effort — tous ont besoin d'une priority queue). Codexion ne retarde pas ce
plan, il te fait coder à la main l'outil que tu utiliseras ensuite en LeetCode.
Si tu veux, je peux fusionner une ligne de rappel dans ce fichier-là une fois
Codexion terminé — dis-le-moi quand tu y arrives.
