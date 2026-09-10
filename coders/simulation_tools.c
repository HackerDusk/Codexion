/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   simulation_tools.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 02:49:03 by srandro           #+#    #+#             */
/*   Updated: 2026/09/09 13:40:24 by srandro          ###   ########.fr       */
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

void	free_models(t_monitor *monitor)
{
	int	i;

	i = 0;
	while (monitor->dongles && i < monitor->nb_coders)
	{
		if (monitor->dongles[i].heap)
		{
			free(monitor->dongles[i].heap->arr);
			free(monitor->dongles[i].heap);
			monitor->dongles[i].heap = NULL;
		}
		i++;
	}
	free(monitor->dongles);
	free(monitor->coders);
	free(monitor);
}
