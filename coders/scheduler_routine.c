/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   scheduler_routine.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 02:48:59 by srandro           #+#    #+#             */
/*   Updated: 2026/09/08 13:45:10 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	*scheduler_routine(void *arg)
{
	t_monitor	*monitor;

	monitor = (t_monitor *)arg;
	while (!is_simulation_stopped(monitor))
	{
		pthread_mutex_lock(&monitor->scheduler_mutex);
		while (!monitor->heap->size && !is_simulation_stopped(monitor))
			pthread_cond_wait(&monitor->scheduler_cond,
				&monitor->scheduler_mutex);
		if (is_simulation_stopped(monitor))
		{
			pthread_mutex_unlock(&monitor->scheduler_mutex);
			break ;
		}
		get_next_coder(monitor);
		pthread_mutex_unlock(&monitor->scheduler_mutex);
	}
	return (NULL);
}
