/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mandresy <mandresy@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/25 17:03:49 by srandro           #+#    #+#             */
/*   Updated: 2026/09/05 00:57:03 by mandresy         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	main(int argc, char **argv)
{
	t_monitor	*monitor;

	if (!full_arg_checker(argc, argv))
		return (1);
	monitor = monitor_initializer(argv);
	if (!monitor)
		return (1);
	routine_simulator(monitor, coder_routine);
	return (0);
}
