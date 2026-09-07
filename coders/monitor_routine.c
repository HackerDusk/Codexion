/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   monitor_routine.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 19:24:43 by srandro           #+#    #+#             */
/*   Updated: 2026/09/06 23:57:48 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

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
		if (ret == ETIMEDOUT)
		{
			if (is_real_burnout(monitor))
			{
				pthread_mutex_unlock(&monitor->monitor_mutex);
				break ;
			}
		}
		if (is_routine_finished(monitor))
		{
			set_simulation_stopped(monitor);
			pthread_cond_broadcast(&monitor->monitor_cond);
			wake_scheduler_up(monitor);
			wake_coders_up(monitor);
			pthread_mutex_unlock(&monitor->monitor_mutex);
			break ;
		}
		pthread_mutex_unlock(&monitor->monitor_mutex);
	}
	return (NULL);
}
