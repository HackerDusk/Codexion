/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   simulator_tools.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/07 00:16:43 by srandro           #+#    #+#             */
/*   Updated: 2026/09/09 14:09:16 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	start_time_initializer(t_monitor *monitor)
{
	int	i;

	i = 0;
	monitor->start_time = get_time_ms();
	while (i < monitor->nb_coders)
		monitor->coders[i++].last_compile_start = monitor->start_time;
}

void	thread_creator(t_monitor *monitor, void *(*coder_routine)(void *))
{
	int	i;

	pthread_create(&monitor->monitor_thread, NULL, monitor_routine,
		monitor);
	i = 0;
	while (i < monitor->nb_coders)
	{
		pthread_create(&monitor->coders[i].coder_thread, NULL, coder_routine,
			&monitor->coders[i]);
		i++;
	}
}

void	thread_joiner(t_monitor *monitor)
{
	int	i;

	i = 0;
	while (i < monitor->nb_coders)
	{
		pthread_join(monitor->coders[i].coder_thread, NULL);
		i++;
	}
	pthread_join(monitor->monitor_thread, NULL);
}

void	cond_mutex_destroyer(t_monitor *monitor)
{
	int	i;

	i = 0;
	while (i < monitor->nb_coders)
	{
		pthread_mutex_destroy(&monitor->dongles[i].dongle_mutex);
		pthread_cond_destroy(&monitor->dongles[i].dongle_cond);
		i++;
	}
	pthread_mutex_destroy(&monitor->monitor_mutex);
	pthread_mutex_destroy(&monitor->print_mutex);
	pthread_cond_destroy(&monitor->monitor_cond);
	pthread_mutex_destroy(&monitor->stop_mutex);
}
