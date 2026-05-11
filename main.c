/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/11 11:26:12 by srandro           #+#    #+#             */
/*   Updated: 2026/05/11 16:13:16 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <unistd.h>

static int	is_positive_int(char *str)
{
	if (!str || !*str)
		return (0);
	if (*str == '0' && *(str + 1) == '\0')
		return (0);
	while (*str)
	{
		if (*str < '0' || *str > '9')
			return (0);
		str++;
	}
	return (1);
}

static int	check_args(char **argc)
{
	int	i;

	i = 1;
	while (*(argc + i) && i < 8)
	{
		if (!is_positive_int(*(argc + i)))
		{
			write (2, "Error: top 7 arguments must be an integer\n", 42);
			return (0);
		}
		i++;
	}
	if (!(!strcmp(argc[i], "fifo")) && !(!strcmp(argc[i], "edf")))
	{
		write (2, "Error: 8th argument must be 'fifo' or 'edf'.\n", 44);
		return (0);
	}
	return (1);
}

void	*routine(void *args)
{
	(void)args;
	return (NULL);
}

static int	launch_threads(t_sim *sim, pthread_t *threads)
{
	int	i;

	i = 0;
	while (i < sim->params.number_of_coders)
	{
		pthread_create(&threads[i], NULL, routine, &sim->coders[i]);
		i++;
	}
	i = 0;
	while (i < sim->params.number_of_coders)
	{
		pthread_join(threads[i], NULL);
		i++;
	}
	return (0);
}

int	main(int argc, char **argv)
{
	struct timeval	start;
	pthread_t		*threads;
	t_sim			sim;

	if (argc != 9)
	{
		printf("Error: Got %d/9 arguments \n", argc);
		return (1);
	}
	if (!check_args(argv))
		return (1);
	if (!setup(&sim, &threads, argv))
	{
		if (!sim.dongles || !sim.coders)
			free_sim(&sim);
		return (1);
	}
	gettimeofday(&start, NULL);
	sim.start_time = start.tv_sec * 1000 + start.tv_usec / 1000;
	launch_threads(&sim, threads);
	free(threads);
	free_sim(&sim);
	return (0);
}
