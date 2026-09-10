/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   argument_checker.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/25 19:54:45 by srandro           #+#    #+#             */
/*   Updated: 2026/09/10 02:50:39 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

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
		if (!is_valid_positive_integer(argv[i], argv, i))
			return (0);
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
