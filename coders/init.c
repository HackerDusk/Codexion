/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/26 13:59:09 by srandro           #+#    #+#             */
/*   Updated: 2026/08/29 16:55:38 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	dongle_initializer(t_monitor *monitor, char **argv)
{
	int	i;
	int	id;

	monitor->dongles = malloc(sizeof(t_dongle) * (monitor->nb_coders));
	if (!monitor->dongles)
		return (0);
	i = 0;
	id = 1;
	while (i < monitor->nb_coders)
	{
		monitor->dongles[i].id = id;
		monitor->dongles[i].is_free = 1;
		monitor->dongles[i].dongle_cooldown = atoi(argv[7]);
		monitor->dongles[i].available_at = 0;
		pthread_mutex_init(&monitor->dongles[i].mutex, NULL);
		pthread_cond_init(&monitor->dongles[i].dongle_cond, NULL);
		i++;
		id++;
	}
	return (1);
}

static int	coder_initializer(t_monitor *monitor, char	**argv)
{
	int	i;

	monitor->coders = malloc(sizeof(t_coder) * (monitor->nb_coders));
	if (!monitor->coders)
		return (0);
	i = 0;
	while (i < monitor->nb_coders)
	{
		monitor->coders[i].id = i + 1;
		monitor->coders[i].time_to_burnout = atoi(argv[2]);
		monitor->coders[i].time_to_compile = atoi(argv[3]);
		monitor->coders[i].time_to_debug = atoi(argv[4]);
		monitor->coders[i].time_to_refactor = atoi(argv[5]);
		monitor->coders[i].number_of_compiles_required = atoi(argv[6]);
		monitor->coders[i].monitor = monitor;
		monitor->coders[i].left_dongle = &monitor->dongles[i];
		monitor->coders[i].right_dongle = &monitor->dongles[
			(i + 1) % monitor->nb_coders];
		monitor->coders[i].compiles_done = 0;
		monitor->coders[i].last_compile_start = 0;
		i++;
	}
	return (1);
}

t_monitor	*monitor_initializer(char **argv)
{
	t_monitor	*monitor;

	monitor = malloc(sizeof(t_monitor));
	if (!monitor)
		return (NULL);
	monitor->nb_coders = atoi(argv[1]);
	if (!strcmp(argv[8], "fifo"))
		monitor->scheduler_type = 1;
	else if (!strcmp(argv[8], "edf"))
		monitor->scheduler_type = 2;
	monitor->stop_simulation = 0;
	pthread_cond_init(&monitor->monitor_cond, NULL);
	pthread_mutex_init(&monitor->monitor_mutex, NULL);
	pthread_mutex_init(&monitor->print_mutex, NULL);
	if (!dongle_initializer(monitor, argv) || !coder_initializer(monitor, argv))
	{
		if (monitor->dongles)
			free(monitor->dongles);
		free(monitor);
		return (NULL);
	}
	return (monitor);
}
