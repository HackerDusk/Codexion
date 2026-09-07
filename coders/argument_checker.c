/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   argument_checker.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/25 19:54:45 by srandro           #+#    #+#             */
/*   Updated: 2026/09/06 23:08:49 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <stdio.h>

static int	fifo_or_edf(char *str)
{
	return (strcmp(str, "fifo") != 0 && strcmp(str, "edf") != 0);
}

static int	is_valid_positive_integer(char *str)
{
	long long	n;

	if (!str)
		return (0);
	if (*str == '+')
		str++;
	if (!*str)
		return (0);
	n = 0;
	while (*str)
	{
		if (*str < '0' || *str > '9')
			return (0);
		n = n * 10 + (*str - '0');
		if (n > INT_MAX)
			return (0);
		str++;
	}
	return (n >= 0);
}

static void	print_availble_args(int argc)
{
	fprintf(stderr, "Missing arguments, got only %d/8 args.\n", argc - 1);
	fprintf(stderr, "Available arguments in this order:\n");
	fprintf(stderr, "- number_of_coders\n");
	fprintf(stderr, "- time_to_burnout\n");
	fprintf(stderr, "- time_to_compile\n");
	fprintf(stderr, "- time_to_debug\n");
	fprintf(stderr, "- time_to_refactor\n");
	fprintf(stderr, "- number_of_compiles_required\n");
	fprintf(stderr, "- dongle_cooldown\n");
	fprintf(stderr, "- scheduler\n");
}

int	full_arg_checker(int argc, char **argv)
{
	int	i;

	if (argc != 9)
	{
		print_availble_args(argc);
		return (0);
	}
	i = 1;
	while (i < argc - 1)
	{
		if (!is_valid_positive_integer(argv[i]))
		{
			fprintf(stderr, "%s must be a POSITIVE INTEGER.\n", argv[i]);
			return (0);
		}
		i++;
	}
	if (fifo_or_edf(argv[i]))
	{
		fprintf(stderr, "Scheduler must be exactly one of:");
		fprintf(stderr, " \"fifo\" or \"edf\".\n");
		return (0);
	}
	return (1);
}
