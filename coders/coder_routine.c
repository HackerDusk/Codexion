#include "codexion.h"

void	*coder_routine(void *arg)
{
	long long		debug_timestamp;
	t_coder			*coder;
	struct timespec	ts;

	coder = (t_coder *)arg;
	debug_timestamp = 0;
	while (1)
	{
		pthread_mutex_lock(&coder->monitor->monitor_mutex);
		if (is_simulation_stopped(coder->monitor)
		|| coder->compiles_done == coder->number_of_compiles_required
		)
		{
			pthread_mutex_unlock(&coder->monitor->monitor_mutex);
			break;
		}
		pthread_mutex_unlock(&coder->monitor->monitor_mutex);
		pthread_mutex_lock(&coder->monitor->scheduler_mutex);
		if (!coder->turn)
			book_a_slot(coder);
		if (!keep_going_after_waiting(coder))
		{
			pthread_mutex_unlock(&coder->monitor->scheduler_mutex);
			break;
		}
		coder->turn = 0;
		pthread_mutex_unlock(&coder->monitor->scheduler_mutex);
		if (!access_dongle(coder, ts))
			break;
		debug_timestamp = coder_is_compiling(coder, debug_timestamp);
		coder_is_debugging(coder, debug_timestamp);
		coder_is_refactoring(coder);
	}
	return NULL;
}

void	*monitor_routine(void *arg)
{
	t_monitor		*monitor;
	struct timespec	ts;
	long long		closest_deadline;
	int			ret;
	

	monitor = (t_monitor *)arg;
	ret = 0;
	while(1)
	{
		pthread_mutex_lock(&monitor->monitor_mutex);
		closest_deadline = get_closest_deadline(monitor);
		ms_to_timespec(closest_deadline, &ts); 
		ret = pthread_cond_timedwait(&monitor->monitor_cond, &monitor->monitor_mutex, &ts);
		if (ret == ETIMEDOUT)
		{
			if (check_alert(monitor))
			{
				pthread_mutex_unlock(&monitor->monitor_mutex);
				break;
			}
		}
		if (is_routine_finished(monitor))
		{
			set_simulation_stopped(monitor);
			pthread_cond_broadcast(&monitor->monitor_cond);
			wake_scheduler(monitor);
			awake_coders(monitor);
			pthread_mutex_unlock(&monitor->monitor_mutex);
			break;
		}
		pthread_mutex_unlock(&monitor->monitor_mutex);
	}
	return (NULL);
}

void	routine_simulator(t_monitor *monitor, void *(*coder_routine)(void *))
{
	int	i;
	
	i = 0;
	monitor->start_time = get_time_ms();
	while (i < monitor->nb_coders)
		monitor->coders[i++].last_compile_start = monitor->start_time;
	i = 0;
	while (i < monitor->nb_coders)
	{
		pthread_create(&monitor->coders[i].coder_thread, NULL, coder_routine,
			&monitor->coders[i]);
			i++;
	}
	pthread_create(&monitor->scheduler_thread, NULL, scheduler_routine,
		monitor);
	pthread_create(&monitor->monitor_thread, NULL, monitor_routine,
		monitor);
	i = 0;
	while (i < monitor->nb_coders)
	{
		pthread_join(monitor->coders[i].coder_thread, NULL);
		i++;
	}
	pthread_join(monitor->monitor_thread, NULL);
	pthread_join(monitor->scheduler_thread, NULL);
	i = 0;
	while (i < monitor->nb_coders)
	{
		pthread_mutex_destroy(&monitor->dongles[i].dongle_mutex);
		pthread_cond_destroy(&monitor->dongles[i].dongle_cond);
		pthread_cond_destroy(&monitor->coders[i].turn_cond);
		i++;
	}
	pthread_mutex_destroy(&monitor->monitor_mutex);
	pthread_mutex_destroy(&monitor->print_mutex);
 	pthread_cond_destroy(&monitor->monitor_cond);
    pthread_mutex_destroy(&monitor->scheduler_mutex);
    pthread_cond_destroy(&monitor->scheduler_cond);
	pthread_mutex_destroy(&monitor->stop_mutex);
	free(monitor->heap->arr);
	free(monitor->heap);
	free(monitor->coders);
	free(monitor->dongles);
	free(monitor);
}
