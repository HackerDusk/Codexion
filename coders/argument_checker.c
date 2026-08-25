/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   argument_checker.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/25 19:54:45 by srandro           #+#    #+#             */
/*   Updated: 2026/08/26 01:46:29 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	fifo_or_edf(char *s)
{
	return (strcmp(s, "fifo") != 0 && strcmp(s, "edf") != 0);
}

static int	is_valid_integer(char *str)
{
	long long	num;

	if (!str)
		return (0);
	if (*str == '+')
		str++;
	if (!*str)
		return (0);
	if (*str == '0' && *(str + 1) == '\0')
		return (0);
	num = 0;
	while (*str)
	{
		if (*str < '0' || *str > '9')
			return (0);
		num = num * 10 + (*str - '0');
		if (num > INT_MAX)
			return (0);
		str++;
	}
	return (1);
}

int	full_arg_checker(int argc, char **argv)
{
	int	i;

	if (argc != 9)
	{
		fprintf(stderr, "Missing arguments, got only %d/8 args.", argc - 1);
		return (0);
	}
	i = 1;
	while (i < argc - 1)
	{
		if (!is_valid_integer(argv[i]))
		{
			fprintf(stderr, "%s must a be POSITIVE INTEGER.", argv[i]);
			return (0);
		}
		i++;
	}
	if (fifo_or_edf(argv[i]))
	{
		fprintf(stderr, "Scheduler must be exactly one of: fifo or edf.");
		return (0);
	}
	return (1);
}
