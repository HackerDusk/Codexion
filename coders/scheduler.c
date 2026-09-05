# include "codexion.h"

int cmp_fifo(t_coder *cd_a, t_coder *cd_b)
{
    return (cd_a->request_time < cd_b->request_time);
}

int cmp_edf(t_coder *cd_a, t_coder *cd_b)
{
    return((cd_a->last_compile_start +
        cd_a->time_to_burnout) < (
        cd_b->last_compile_start +
        cd_b->time_to_burnout));
}

t_coder *get_next_coder(t_monitor *monitor)
{
    t_coder *next;

    next = heap_pop(monitor->heap);
    if (!next)
        return (NULL);
    next->turn = 1;
    pthread_cond_broadcast(&next->turn_cond);
    return (next);
}

void    *scheduler_routine(void *arg)
{
    t_monitor   *monitor;

    monitor = (t_monitor *)arg;
    while(!is_simulation_stopped(monitor))
    {
        pthread_mutex_lock(&monitor->scheduler_mutex);
        while (!monitor->heap->size && !is_simulation_stopped(monitor))
            pthread_cond_wait(&monitor->scheduler_cond, &monitor->scheduler_mutex);
        if (is_simulation_stopped(monitor))
        {
            pthread_mutex_unlock(&monitor->scheduler_mutex);
            break;
        }
        get_next_coder(monitor);
        pthread_mutex_unlock(&monitor->scheduler_mutex);
    }
    return (NULL);
}

void    book_a_slot(t_coder *coder)
{
    coder->request_time = get_time_ms();
    heap_push(coder->monitor->heap, coder);
    pthread_cond_broadcast(&coder->monitor->scheduler_cond);
}