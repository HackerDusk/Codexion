/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   monitor_tools.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 17:38:38 by srandro           #+#    #+#             */
/*   Updated: 2026/09/08 13:49:31 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

long long	get_closest_deadline(t_monitor *monitor)
{
	long long	curr_deadline;
	int			i;
	int			found;

	i = 0;
	found = 0;
	curr_deadline = 0;
	while (i < monitor->nb_coders)
	{
		if (monitor->coders[i].compiles_done
			< monitor->coders[i].number_of_compiles_required)
		{
			if (!found || curr_deadline
				> (monitor->coders[i].last_compile_start
					+ monitor->coders[i].time_to_burnout))
				curr_deadline = change_curr_deadline(monitor->coders[i]);
			found = 1;
		}
		i++;
	}
	if (!found)
		return (get_time_ms());
	return (curr_deadline);
}

static void	print_burnout_message(t_monitor *monitor,
	long long curr_time, int i)
{
	set_simulation_stopped(monitor);
	pthread_mutex_lock(&monitor->print_mutex);
	printf("%lld %d burned out\n",
		curr_time - monitor->start_time, monitor->coders[i].id);
	pthread_mutex_unlock(&monitor->print_mutex);
	wake_coders_up(monitor);
	wake_scheduler_up(monitor);
	pthread_cond_broadcast(&monitor->monitor_cond);
}

int	is_real_burnout(t_monitor *monitor)
{
	long long	curr_time;
	int			i;

	i = 0;
	curr_time = get_time_ms();
	while (i < monitor->nb_coders)
	{
		if (monitor->coders[i].compiles_done
			< monitor->coders[i].number_of_compiles_required)
		{
			if (curr_time >= (
					monitor->coders[i].last_compile_start
					+ monitor->coders[i].time_to_burnout))
			{
				print_burnout_message(monitor, curr_time, i);
				return (1);
			}
		}
		i++;
	}
	return (0);
}

void	wake_coders_up(t_monitor *monitor)
{
	int	i;

	i = 0;
	while (i < monitor->nb_coders)
	{
		pthread_mutex_lock(&monitor->dongles[i].dongle_mutex);
		pthread_cond_broadcast(&monitor->dongles[i].dongle_cond);
		pthread_mutex_unlock(&monitor->dongles[i].dongle_mutex);
		i++;
	}
	pthread_mutex_lock(&monitor->scheduler_mutex);
	i = 0;
	while (i < monitor->nb_coders)
		pthread_cond_signal(&monitor->coders[i++].turn_cond);
	pthread_mutex_unlock(&monitor->scheduler_mutex);
}

void	wake_scheduler_up(t_monitor *monitor)
{
	pthread_mutex_lock(&monitor->scheduler_mutex);
	pthread_cond_signal(&monitor->scheduler_cond);
	pthread_mutex_unlock(&monitor->scheduler_mutex);
}
