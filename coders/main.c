/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/25 17:03:49 by srandro           #+#    #+#             */
/*   Updated: 2026/08/29 17:18:19 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

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

int	take_a_dongle(t_coder *coder)
{
	t_dongle	*first;
	t_dongle	*second;
	
	if (coder->id % 2 == 0)
	{
		first = coder->left_dongle;
		second = coder->right_dongle;
	}
	else {
		first = coder->right_dongle;
		second = coder->left_dongle;
	}
	pthread_mutex_lock(&first->mutex);
	while (!first->is_free && !coder->monitor->stop_simulation)
		pthread_cond_wait(&first->dongle_cond, &first->mutex);
	if (coder->monitor->stop_simulation)
	{
		pthread_mutex_unlock(&first->mutex);
		return (0);
	}
	if (get_time_ms() < first->available_at)
	{
		pthread_mutex_unlock(&first->mutex);
		usleep((first->available_at - get_time_ms()) * 1000);
	}
	first->is_free = 0;
	pthread_mutex_lock(&coder->monitor->print_mutex);
	fprintf(stdout, "%lld %d has taken a dongle\n",
		get_time_ms() - coder->monitor->start_time, coder->id);
	pthread_mutex_unlock(&coder->monitor->print_mutex);
	pthread_mutex_unlock(&first->mutex);
	pthread_mutex_lock(&second->mutex);
	while (!second->is_free && !coder->monitor->stop_simulation)
		pthread_cond_wait(&second->dongle_cond, &second->mutex);
	if (coder->monitor->stop_simulation)
	{
		pthread_mutex_unlock(&second->mutex);
		return (0);
	}
	if (get_time_ms() < second->available_at)
	{
		pthread_mutex_unlock(&second->mutex);
		usleep((second->available_at - get_time_ms()) * 1000);
	}
	second->is_free = 0;
	pthread_mutex_lock(&coder->monitor->print_mutex);
	fprintf(stdout, "%lld %d has taken a dongle\n",
		get_time_ms() - coder->monitor->start_time , coder->id);
	pthread_mutex_unlock(&coder->monitor->print_mutex);
	pthread_mutex_unlock(&second->mutex);
	return (1);
}

void	release_dongles(t_coder *coder)
{
	pthread_mutex_lock(&coder->left_dongle->mutex);
	coder->left_dongle->available_at = get_time_ms() + coder->left_dongle->dongle_cooldown;
	coder->left_dongle->is_free = 1;
	pthread_cond_broadcast(&coder->left_dongle->dongle_cond);
	pthread_mutex_unlock(&coder->left_dongle->mutex);

	pthread_mutex_lock(&coder->right_dongle->mutex);
	coder->right_dongle->available_at = get_time_ms() + coder->right_dongle->dongle_cooldown;
	coder->right_dongle->is_free = 1;
	pthread_cond_broadcast(&coder->right_dongle->dongle_cond);
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
			coder->compiles_done == coder->number_of_compiles_required
		)
		{
			pthread_mutex_unlock(&coder->monitor->monitor_mutex);
			break;
		}
		pthread_mutex_unlock(&coder->monitor->monitor_mutex);
		if (!take_a_dongle(coder))
			break;

		pthread_mutex_lock(&coder->monitor->print_mutex);
		fprintf(stdout, "%lld %d is compiling\n",
			get_time_ms() - coder->monitor->start_time, coder->id);
		pthread_mutex_unlock(&coder->monitor->print_mutex);
		pthread_mutex_lock(&coder->monitor->monitor_mutex);
		coder->last_compile_start = get_time_ms();
		pthread_mutex_unlock(&coder->monitor->monitor_mutex);
		usleep(coder->time_to_compile * 1000);
		pthread_mutex_lock(&coder->monitor->monitor_mutex);
		coder->compiles_done += 1;
		pthread_mutex_unlock(&coder->monitor->monitor_mutex);
		release_dongles(coder);
		pthread_mutex_lock(&coder->monitor->print_mutex);
		fprintf(stdout, "%lld %d is debugging\n",
		get_time_ms() - coder->monitor->start_time, coder->id);
		pthread_mutex_unlock(&coder->monitor->print_mutex);
		usleep(coder->time_to_debug * 1000);
		pthread_mutex_lock(&coder->monitor->print_mutex);
		fprintf(stdout, "%lld %d is refactoring\n",
		get_time_ms() - coder->monitor->start_time, coder->id);
		pthread_mutex_unlock(&coder->monitor->print_mutex);
		usleep(coder->time_to_refactor * 1000);
	}
	return NULL;
}

long long	get_closest_deadline(t_monitor *monitor)
{
	long long	curr_deadline;
	int	i;

	curr_deadline = (
		monitor->coders[0].last_compile_start + monitor->coders[0].time_to_burnout);
	i = 1;
	while (i < monitor->nb_coders)
	{
		if (
			curr_deadline > (
				monitor->coders[i].last_compile_start +
				monitor->coders[i].time_to_burnout))
				{
			curr_deadline = (
				monitor->coders[i].last_compile_start +
				monitor->coders[i].time_to_burnout);
		}
		i++;
	}
	return (curr_deadline);
}
void	awake_coders(t_monitor *monitor)
{
	int	i;

	i = 0;
	while (i < monitor->nb_coders)
		pthread_cond_broadcast(&monitor->dongles[i++].dongle_cond);
}

int	check_alert(t_monitor *monitor)
{
	int	i;
	long long	curr_time;

	i = 0;
	curr_time = get_time_ms();
	while (i < monitor->nb_coders)
	{
		if (curr_time > (
			monitor->coders[i].last_compile_start +
			monitor->coders[i].time_to_burnout))
			{
				pthread_mutex_lock(&monitor->print_mutex);
				printf("%lld %d burned out\n", curr_time - monitor->start_time, monitor->coders[i].id);
				pthread_mutex_unlock(&monitor->print_mutex);
				monitor->stop_simulation = 1;
				awake_coders(monitor);
				return (1);
			}
		i++;
	}
	return (0);
}

int	is_routine_finished(t_monitor *monitor)
{
	int	i;

	i = 0;
	while (i < monitor->nb_coders)
	{
		if ((
			monitor->coders[i].compiles_done  !=
			monitor->coders[i].number_of_compiles_required))
			return (0);
		i++;
	}
	return (1);
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
			if (is_routine_finished(monitor))
			{
				monitor->stop_simulation = 1;
				pthread_cond_broadcast(&monitor->monitor_cond);
				pthread_mutex_unlock(&monitor->monitor_mutex);
				break;
			}
			
		}
		pthread_mutex_unlock(&monitor->monitor_mutex);
	}
	return (NULL);
}

void	simulator(t_monitor *monitor, void *(*coder_routine)(void *))
{
	int	i;
	
	i = 0;
	monitor->start_time = get_time_ms();
	while (i < monitor->nb_coders)
		monitor->coders[i++].last_compile_start = monitor->start_time;
	i = 0;
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
	pthread_mutex_destroy(&monitor->print_mutex);
	pthread_cond_destroy(&monitor->monitor_cond);
	free(monitor->coders);
	free(monitor->dongles);
	free(monitor);
}

int	main(int argc, char **argv)
{
	t_monitor	*monitor;

	if (!full_arg_checker(argc, argv))
		return (1);
	monitor = monitor_initializer(argv);
	if (!monitor)
		return (1);
	simulator(monitor, coder_routine);
	return (0);
}
