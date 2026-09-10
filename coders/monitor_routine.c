/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   monitor_routine.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 19:24:43 by srandro           #+#    #+#             */
/*   Updated: 2026/09/09 22:00:04 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

long long	change_curr_deadline(t_coder *coder)
{
	return (coder->last_compile_start + coder->time_to_burnout);
}

int	is_routine_finished(t_monitor *monitor)
{
	int	i;

	i = 0;
	while (i < monitor->nb_coders)
	{
		if ((
				monitor->coders[i].compiles_done
				!= monitor->coders[i].number_of_compiles_required))
			return (0);
		i++;
	}
	return (1);
}

static int	is_end_of_routine(t_monitor *monitor, int ret)
{
	if (ret == ETIMEDOUT)
	{
		if (is_real_burnout(monitor))
		{
			pthread_mutex_unlock(&monitor->monitor_mutex);
			return (1);
		}
	}
	if (is_routine_finished(monitor))
	{
		set_simulation_stopped(monitor);
		pthread_cond_broadcast(&monitor->monitor_cond);
		wake_coders_up(monitor);
		pthread_mutex_unlock(&monitor->monitor_mutex);
		return (1);
	}
	return (0);
}

void	*monitor_routine(void *arg)
{
	t_monitor		*monitor;
	struct timespec	ts;
	long long		closest_deadline;
	int				ret;

	monitor = (t_monitor *)arg;
	ret = 0;
	while (1)
	{
		pthread_mutex_lock(&monitor->monitor_mutex);
		closest_deadline = get_closest_deadline(monitor);
		ms_to_timespec(closest_deadline, &ts);
		ret = pthread_cond_timedwait(&monitor->monitor_cond,
				&monitor->monitor_mutex, &ts);
		if (is_end_of_routine(monitor, ret))
			break ;
		pthread_mutex_unlock(&monitor->monitor_mutex);
	}
	return (NULL);
}
