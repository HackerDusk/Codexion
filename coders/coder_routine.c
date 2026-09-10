/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder_routine.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 02:48:33 by srandro           #+#    #+#             */
/*   Updated: 2026/09/09 03:20:48 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	end_of_simulation(t_coder *coder)
{
	pthread_mutex_lock(&coder->monitor->monitor_mutex);
	if (is_simulation_stopped(coder->monitor)
		|| coder->compiles_done == coder->number_of_compiles_required)
	{
		pthread_mutex_unlock(&coder->monitor->monitor_mutex);
		return (1);
	}
	pthread_mutex_unlock(&coder->monitor->monitor_mutex);
	return (0);
}

void	*coder_routine(void *arg)
{
	long long		debug_timestamp;
	t_coder			*coder;

	coder = (t_coder *)arg;
	debug_timestamp = 0;
	while (1)
	{
		if (end_of_simulation(coder))
			break ;
		if (!access_dongle(coder))
			break ;
		debug_timestamp = coder_is_compiling(coder, debug_timestamp);
		coder_is_debugging(coder, debug_timestamp);
		coder_is_refactoring(coder);
	}
	return (NULL);
}
