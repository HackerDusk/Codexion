/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   argument_checker.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/25 19:54:45 by srandro           #+#    #+#             */
/*   Updated: 2026/08/26 11:02:55 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	fifo_or_edf(char *s)
{
	return (strcmp(s, "fifo") != 0 && strcmp(s, "edf") != 0);
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
	return (n > 0);
}

int	full_arg_checker(int argc, char **argv)
{
	int	i;

	if (argc != 9)
	{
		fprintf(stderr, "Missing arguments, got only %d/8 args.\n", argc - 1);
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
		fprintf(stderr, "Scheduler must be exactly one of: fifo or edf.\n");
		return (0);
	}
	return (1);
}
