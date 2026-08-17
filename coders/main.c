/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/11 11:26:12 by srandro           #+#    #+#             */
/*   Updated: 2026/08/18 01:01:08 by srandro          ###   ########.fr       */
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


int	main(int argc, char **argv)
{
	if (argc != 9)
	{
		printf("Error: Got %d/9 arguments \n", argc);
		return (1);
	}
	if (!check_args(argv))
		return (1);
}
