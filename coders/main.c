/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/25 17:03:49 by srandro           #+#    #+#             */
/*   Updated: 2026/08/26 15:08:48 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <pthread.h>

long long	get_time_ms()
{
	struct timeval	tv;
	
	gettimeofday(&tv, NULL);
	return ((long long)tv.tv_sec * 1000 + tv.tv_usec / 1000);
	
}

void	ms_to_timespec(long long time_in_ms, struct timespec *ts)
{
	ts->tv_sec = time_in_ms / 1000;
	ts->tv_nsec = (time_in_ms % 1000) * 1000000;
}

void	take_a_dongle(t_coder *coder)
{
	t_dongle	*first;
	t_dongle	*second;
	
	if (coder->left_dongle->id < coder->right_dongle->id)
	{
		first = coder->left_dongle;
		second = coder->right_dongle;		
	}
	else {
		first = coder->right_dongle;
		second = coder->left_dongle;
	}
	pthread_mutex_lock(&first->mutex);
	while (!first->is_free)
		pthread_cond_wait(&first->dongle_cond, &first->mutex);
	first->is_free = 0;
	fprintf(stdout, "%d has taken a dongle\n", coder->id);
	pthread_mutex_unlock(&coder->left_dongle->mutex);
	pthread_mutex_lock(&second->mutex);
	while (!second->is_free)
		pthread_cond_wait(&second->dongle_cond, &second->mutex);
	second->is_free = 0;
	fprintf(stdout, "%d has taken a dongle\n", coder->id);
	pthread_mutex_unlock(&coder->right_dongle->mutex);
}

void	release_dongles(t_coder *coder)
{
	pthread_mutex_lock(&coder->left_dongle->mutex);
	usleep(coder->left_dongle->dongle_cooldown * 1000);
	coder->left_dongle->is_free = 1;
	pthread_cond_signal(&coder->left_dongle->dongle_cond);
	pthread_mutex_unlock(&coder->left_dongle->mutex);
	pthread_mutex_lock(&coder->right_dongle->mutex);
	usleep(coder->right_dongle->dongle_cooldown * 1000);
	coder->right_dongle->is_free = 1;
	pthread_cond_signal(&coder->right_dongle->dongle_cond);
	pthread_mutex_unlock(&coder->right_dongle->mutex);
}

void	*coder_routine(void *arg)
{
	t_coder			*coder;

	coder = (t_coder *)arg;
	while (1)
	{
		pthread_mutex_lock(&coder->monitor->monitor_mutex);
		if (coder->monitor->stop_simulation ||
			coder->compliles_done == coder->number_of_compiles_required
		)
		{
			pthread_mutex_unlock(&coder->monitor->monitor_mutex);
			break;
		}
		pthread_mutex_unlock(&coder->monitor->monitor_mutex);
		take_a_dongle(coder);
		fprintf(stdout, "%lld %d is compiling\n",
			get_time_ms() - coder->monitor->start_time, coder->id);
			usleep(coder->time_to_compile * 1000);
		pthread_mutex_lock(&coder->monitor->monitor_mutex);
		coder->curr_time_before_burnout = get_time_ms() + coder->time_to_burnout;
		pthread_cond_signal(&coder->monitor->monitor_cond);
		pthread_mutex_unlock(&coder->monitor->monitor_mutex);
		pthread_mutex_lock(&coder->monitor->monitor_mutex);
		coder->compliles_done += 1;
		pthread_mutex_unlock(&coder->monitor->monitor_mutex);
		release_dongles(coder);
		fprintf(stdout, "¨%lld %d is debugging\n",
		get_time_ms() - coder->monitor->start_time, coder->id);
		usleep(coder->time_to_debug * 1000);
		fprintf(stdout, "%lld %d is refactoring\n",
		get_time_ms() - coder->monitor->start_time, coder->id);
		usleep(coder->time_to_refactor * 1000);
	}
	return NULL;
}

void	*monitor_routine(void *arg)
{
	t_monitor			*monitor;
	struct	timespec	ts;
	long long			closest_deadline;
	int					ret;

	monitor = (t_monitor *)arg;
	ret = 0;
	while(1)
	{
		pthread_mutex_lock(&monitor->monitor_mutex);
		// closest_deadline = ;
		ms_to_timespec(closest_deadline, &ts);
		ret = pthread_cond_timedwait(&monitor->monitor_cond, &monitor->monitor_mutex, &ts);
		if (ret == ETIMEDOUT)
		{
			fprintf(stdout, "burned out");
			monitor->stop_simulation = 1;
			pthread_mutex_unlock(&monitor->monitor_mutex);
			break;
		}
		pthread_mutex_unlock(&monitor->monitor_mutex);
	}
	return (NULL);
}

void	simualtor(t_monitor *monitor, void *(*coder_routine)(void *))
{
	int	i;
	
	i = 0;
	monitor->start_time = get_time_ms();
	while (i < monitor->nb_coders)
	{
		pthread_create(&monitor->coders[i].thread, NULL, coder_routine,
			&monitor->coders[i]);
			i++;
	}
	pthread_create(&monitor->monitor_thread, NULL, monitor_routine,
		monitor);
	i = 0;
	while (i < monitor->nb_coders)
	{
		pthread_join(monitor->coders[i].thread, NULL);
		pthread_mutex_destroy(&monitor->dongles[i].mutex);
		pthread_cond_destroy(&monitor->dongles[i].dongle_cond);
		i++;
	}
	pthread_join(monitor->monitor_thread, NULL);
	pthread_mutex_destroy(&monitor->monitor_mutex);
	pthread_cond_destroy(&monitor->monitor_cond);
}

int	main(int argc, char **argv)
{
	t_monitor	*monitor;

	if (!full_arg_checker(argc, argv))
		return (1);
	monitor = monitor_initializer(argv);
	if (!monitor)
		return (1);
	simualtor(monitor, coder_routine);
return (0);
}
