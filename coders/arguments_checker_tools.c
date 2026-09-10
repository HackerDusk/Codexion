/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   arguments_checker_tools.c                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/10 02:51:15 by srandro           #+#    #+#             */
/*   Updated: 2026/09/10 02:54:31 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	fifo_or_edf(char *str)
{
	return (strcmp(str, "fifo") != 0 && strcmp(str, "edf") != 0);
}

long long	is_negative(long long n, char *str)
{
	if (n < 0)
	{
		fprintf(stderr, "%s must be a POSITIVE INTEGER.\n", str);
		return (1);
	}
	return (0);
}

int	check_digits(char *str, char **argv, int i, long long *n)
{
	while (*str)
	{
		if (*str < '0' || *str > '9')
		{
			fprintf(stderr, "%s must be a POSITIVE INTEGER.\n", argv[i]);
			return (0);
		}
		*n = *n * 10 + (*str - '0');
		if (*n > INT_MAX)
		{
			fprintf(stderr, "%s must not be over INTMAX.\n", argv[i]);
			return (0);
		}
		str++;
	}
	return (1);
}

int	is_valid_positive_integer(char *str, char **argv, int i)
{
	long long	n;

	if (!str)
		return (0);
	if (*str == '+')
		str++;
	if (!*str)
	{
		fprintf(stderr, "%s must be a POSITIVE INTEGER.\n", argv[i]);
		return (0);
	}
	n = 0;
	if (!check_digits(str, argv, i, &n))
		return (0);
	return (!is_negative(n, str));
}

void	print_availble_args(int argc)
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
