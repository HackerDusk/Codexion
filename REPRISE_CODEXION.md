# Reprise — Apprentissage Codexion (42 Antananarivo)

## Contexte étudiant
- Étudiant à 42 Antananarivo, login: `srandro`
- Niveau de départ : zéro en threads/mutex/concurrence, C rouillé
- Objectif : comprendre profondément, pas juste faire compiler

---

## Méthode pédagogique (OBLIGATOIRE à respecter)

- Toujours expliquer le **Pourquoi** avant le **Comment**
- Utiliser des **analogies simples** avant les détails techniques
- **Ne pas donner la solution complète** — guider avec des indices
- Laisser l'étudiant coder lui-même, corriger ses erreurs en expliquant
- **Une question à la fois**
- Si bloqué : question socratique → micro-étape → exemple simplifié
- Jamais la solution complète sauf si l'étudiant dit "je capitule"
- Quand un exercice marche → demander d'expliquer avec ses propres mots avant de passer à la suite
- Respecter la **norme 42** (norminette) : max 25 lignes/fonction, max 5 fonctions/fichier, pas de variables globales

---

## Ce que l'étudiant maîtrise maintenant ✅

- Thread vs Processus (concept clair)
- Race condition (vécu et compris)
- `pthread_create` / `pthread_join`
- Mutex : `pthread_mutex_lock` / `pthread_mutex_unlock` / `init` / `destroy`
- `pthread_cond_wait` / `pthread_cond_signal` (salle d'attente)
- `gettimeofday` / `usleep` / timestamps relatifs
- Struct pour passer des données aux threads (`void *arg`)
- Pattern : attendre une ressource, la prendre, la libérer
- Modulo pour table circulaire (`(i+1) % n`)
- Norminette : sait découper les fonctions, utiliser `static`

---

## Le projet — Codexion

### But pédagogique profond
Résoudre le **problème des dîneurs de Dijkstra** version "coders et dongles USB".
Apprendre : deadlock, starvation, synchronisation, scheduling temps-réel.

### Résumé du sujet
- N coders assis en cercle, N dongles entre eux
- Pour compiler : besoin de 2 dongles (gauche + droite)
- Cycle : compiler → déboguer → refactoriser → recommencer
- Si un coder ne compile pas dans `time_to_burnout` ms → burnout → simulation stop
- Scheduler : `fifo` (premier arrivé) ou `edf` (deadline la plus proche d'abord)
- Après libération, un dongle est indisponible pendant `dongle_cooldown` ms

### Arguments du programme
```
./codexion number_of_coders time_to_burnout time_to_compile time_to_debug
           time_to_refactor number_of_compiles_required dongle_cooldown scheduler
```

### Format des logs
```
timestamp_in_ms X has taken a dongle
timestamp_in_ms X is compiling
timestamp_in_ms X is debugging
timestamp_in_ms X is refactoring
timestamp_in_ms X burned out
```

---

## Architecture du projet

### Arborescence
```
coders/
├── Makefile
├── codexion.h
├── main.c
├── init.c
├── utils.c
├── coder.c      ← À CRÉER (routine du coder)
├── monitor.c    ← À CRÉER (thread moniteur)
└── scheduler.c  ← À CRÉER (priority queue fifo/edf)
```

### Structs (codexion.h) — FINALISÉES ✅
```c
typedef struct s_params
{
    char    *scheduler;
    int     number_of_coders;
    int     time_to_burnout;
    int     time_to_compile;
    int     time_to_debug;
    int     time_to_refactor;
    int     number_of_compiles_required;
    int     dongle_cooldown;
}   t_params;

typedef struct s_dongle
{
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    long            cooldown_end;
    long            dongle_cooldown;
    int             available;
}   t_dongle;

typedef struct s_sim    t_sim;
typedef struct s_coder
{
    t_dongle    *left_dongle;
    t_dongle    *right_dongle;
    t_sim       *sim;
    long        last_compile_time;
    int         n_coder;
    int         n_compilation;
}   t_coder;

typedef struct s_sim
{
    pthread_mutex_t log_mutex;
    t_params        params;
    t_dongle        *dongles;
    t_coder         *coders;
    long            start_time;
}   t_sim;
```

---

## Code existant — état actuel

### main.c ✅ (norminette OK)
- `is_positive_int` : valide les entiers positifs non nuls
- `check_args` : valide les 8 arguments
- `routine` : vide pour l'instant (à remplir dans coder.c)
- `launch_threads` : crée et joint les threads
- `main` : parsing → setup → start_time → launch → free

### init.c ✅ (norminette OK)
- `init_dongle_and_coder` : initialise chaque dongle et coder
  - `available = 1`, `cooldown_end = 0`
  - `left_dongle = &dongles[i]`
  - `right_dongle = &dongles[(i+1) % n]`
- `init_sim` : remplit params depuis argv, malloc dongles et coders

### utils.c ✅ (norminette OK)
- `free_sim` : libère dongles et coders
- `setup` : appelle init_sim + malloc threads

---

## Prochaine étape — `coder.c`

### Ce qui reste à faire (dans l'ordre)

**1. `get_time_ms`** dans utils.c
```c
long    get_time_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}
```

**2. `log_action`** dans utils.c — affichage thread-safe
```c
void    log_action(t_coder *coder, char *msg)
{
    long timestamp = get_time_ms() - coder->sim->start_time;
    pthread_mutex_lock(&coder->sim->log_mutex);
    printf("%ld %d %s\n", timestamp, coder->n_coder, msg);
    pthread_mutex_unlock(&coder->sim->log_mutex);
}
```

**3. `take_dongle`** dans coder.c
- Lock mutex du dongle
- Attendre que `available == 1` ET `get_time_ms() >= cooldown_end`
- Marquer `available = 0`
- Unlock
- Afficher "has taken a dongle"

**4. `release_dongle`** dans coder.c
- Lock mutex
- `available = 1`
- `cooldown_end = get_time_ms() + dongle_cooldown`
- Signal pour réveiller les waiters
- Unlock

**5. `routine`** dans coder.c — boucle principale du coder
```
boucle:
    take_dongle(left)
    take_dongle(right)
    log "is compiling" + usleep(time_to_compile)
    release_dongle(left) + release_dongle(right)
    log "is debugging" + usleep(time_to_debug)
    log "is refactoring" + usleep(time_to_refactor)
    n_compilation++
    si n_compilation >= number_of_compiles_required → stop
```

**6. Thread moniteur** dans monitor.c
- Thread séparé qui vérifie toutes les ~1ms
- Si `get_time_ms() - coder->last_compile_time > time_to_burnout` → burnout

**7. Scheduler fifo/edf** dans scheduler.c
- Priority queue (tas min) pour arbitrer qui obtient le dongle
- fifo : ordre d'arrivée
- edf : deadline = last_compile_start + time_to_burnout

---

## Question en suspens (répondre avant de coder `take_dongle`)

> Pourquoi prendre dongle gauche puis droite dans un ordre pair/impair évite le deadlock ?

L'étudiant comprend le cercle d'attente mais n'a pas encore expliqué
comment pair/impair le casse. Faire expliquer avant de continuer.

---

## Anti-deadlock — solution à implémenter

Coder impair (n_coder % 2 == 1) : prend gauche puis droite
Coder pair  (n_coder % 2 == 0) : prend droite puis gauche

Casser la circularité de l'attente = casser le deadlock.

---

## Notes importantes
- `dongle_cooldown` dans `t_dongle` est redondant (déjà dans params) — peut être supprimé
- Le `log_mutex` dans `t_sim` doit être initialisé dans `init_sim`
- `last_compile_time` doit être initialisé à `start_time` (pas 0) pour que le premier burnout soit correctement calculé
- Compiler avec : `cc -Wall -Wextra -Werror -pthread`
