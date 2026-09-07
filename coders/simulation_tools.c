/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   simulation_tools.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 02:49:03 by srandro           #+#    #+#             */
/*   Updated: 2026/09/07 00:16:13 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	is_simulation_stopped(t_monitor *monitor)
{
	int	v;

	pthread_mutex_lock(&monitor->stop_mutex);
	v = monitor->stop_simulation;
	pthread_mutex_unlock(&monitor->stop_mutex);
	return (v);
}

void	set_simulation_stopped(t_monitor *monitor)
{
	pthread_mutex_lock(&monitor->stop_mutex);
	monitor->stop_simulation = 1;
	pthread_mutex_unlock(&monitor->stop_mutex);
}

int	keep_going_after_waiting_turn(t_coder *coder)
{
	while (!coder->turn && !is_simulation_stopped(coder->monitor))
		pthread_cond_wait(&coder->turn_cond, &coder->monitor->scheduler_mutex);
	if (is_simulation_stopped(coder->monitor))
	{
		pthread_mutex_unlock(&coder->monitor->scheduler_mutex);
		return (0);
	}
	return (1);
}
