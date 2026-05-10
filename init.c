# include <unistd.h>

static void init_dongle_and_coder(t_sim *sim)
{
    int i = 0;
    while (i < sim->params.number_of_coders)
    {
        sim->dongles[i].available = 1;
        sim->dongles[i].cooldown_end = 0;
        pthread_mutex_init(&sim->dongles[i].mutex, NULL);
        pthread_cond_init(&sim->dongles[i].cond, NULL);
        sim->coders[i].n_coder = i + 1;
        sim->coders[i].n_compilation = 0;
        sim->coders[i].last_compile_time = 0;
        sim->coders[i].sim = sim;
        sim->coders[i].left_dongle  = &sim->dongles[i];
        sim->coders[i].right_dongle = &sim->dongles[(i + 1) % sim->params.number_of_coders];
        i++;
    }
}

int init_sim(t_sim *sim, char **argv)
{
    sim->params.number_of_coders = atoi(argv[1]);
    sim->params.time_to_burnout = atoi(argv[2]);
    sim->params.time_to_compile = atoi(argv[3]);
    sim->params.time_to_debug = atoi(argv[4]);
    sim->params.time_to_refactor = atoi(argv[5]);
    sim->params.number_of_compiles_required = atoi(argv[6]);
    sim->params.dongle_cooldown = atoi(argv[7]);
    sim->params.scheduler = argv[8];
    sim->dongles = malloc(sizeof(t_dongle) * atoi(argv[1]));
    sim->coders  = malloc(sizeof(t_coder) * atoi(argv[1]));
    if (!sim->dongles || !sim->coders)
        return (0);
    return (1);
}