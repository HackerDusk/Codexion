/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/26 13:59:09 by srandro           #+#    #+#             */
/*   Updated: 2026/09/07 14:27:25 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	dongles_initializer(t_monitor *monitor, char **argv)
{
	int	i;

	monitor->dongles = malloc(sizeof(t_dongle) * (monitor->nb_coders));
	if (!monitor->dongles)
		return (0);
	i = 0;
	while (i < monitor->nb_coders)
	{
		monitor->dongles[i].id = i + 1;
		monitor->dongles[i].dongle_cooldown = atoi(argv[7]);
		monitor->dongles[i].is_free = 1;
		monitor->dongles[i].available_at = 0;
		pthread_mutex_init(&monitor->dongles[i].dongle_mutex, NULL);
		pthread_cond_init(&monitor->dongles[i].dongle_cond, NULL);
		i++;
	}
	return (1);
}

static void	add_coder_values(t_coder *coder, int i, char **argv)
{
	coder->id = i + 1;
	coder->time_to_burnout = atoi(argv[2]);
	coder->time_to_compile = atoi(argv[3]);
	coder->time_to_debug = atoi(argv[4]);
	coder->time_to_refactor = atoi(argv[5]);
	coder->turn = 0;
	coder->compiles_done = 0;
	coder->number_of_compiles_required = atoi(argv[6]);
	coder->request_time = 0;
	coder->last_compile_start = 0;
}

static int	coders_initializer(t_monitor *monitor, char	**argv)
{
	int	i;

	monitor->coders = malloc(sizeof(t_coder) * (monitor->nb_coders));
	if (!monitor->coders)
		return (0);
	i = 0;
	while (i < monitor->nb_coders)
	{
		add_coder_values(&monitor->coders[i], i, argv);
		monitor->coders[i].monitor = monitor;
		monitor->coders[i].left_dongle = &monitor->dongles[i];
		monitor->coders[i].right_dongle = &monitor->dongles[
			(i + 1) % monitor->nb_coders];
		pthread_cond_init(&monitor->coders[i].turn_cond, NULL);
		i++;
	}
	return (1);
}

static void	monitor_val_initializer(t_monitor *monitor, char **argv)
{
	monitor->nb_coders = atoi(argv[1]);
	monitor->stop_simulation = 0;
	if (!strcmp(argv[8], "fifo"))
		monitor->scheduler_type = "fifo";
	else if (!strcmp(argv[8], "edf"))
		monitor->scheduler_type = "edf";
	pthread_mutex_init(&monitor->monitor_mutex, NULL);
	pthread_mutex_init(&monitor->scheduler_mutex, NULL);
	pthread_mutex_init(&monitor->print_mutex, NULL);
	pthread_cond_init(&monitor->monitor_cond, NULL);
	pthread_cond_init(&monitor->scheduler_cond, NULL);
	pthread_mutex_init(&monitor->stop_mutex, NULL);
}

t_monitor	*monitor_initializer(char **argv)
{
	t_monitor	*monitor;

	monitor = malloc(sizeof(t_monitor));
	if (!monitor)
		return (NULL);
	monitor->dongles = NULL;
	monitor->coders = NULL;
	monitor->heap = NULL;
	if (atoi(argv[1]) <= 0)
	{
		fprintf(stderr, "number_of_coders must be greater than 0.\n");
		return (NULL);
	}
	monitor_val_initializer(monitor, argv);
	if (!heap_initializer(monitor) || !dongles_initializer(monitor, argv)
		|| !coders_initializer(monitor, argv))
	{
		if (monitor->dongles)
			free(monitor->dongles);
		free(monitor);
		return (NULL);
	}
	return (monitor);
}
