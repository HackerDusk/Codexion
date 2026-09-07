/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder_routine.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 02:48:33 by srandro           #+#    #+#             */
/*   Updated: 2026/09/07 03:46:46 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	*coder_routine(void *arg)
{
	long long		debug_timestamp;
	t_coder			*coder;
	struct timespec	ts;

	coder = (t_coder *)arg;
	debug_timestamp = 0;
	while (1)
	{
		pthread_mutex_lock(&coder->monitor->monitor_mutex);
		if (is_simulation_stopped(coder->monitor)
			|| coder->compiles_done == coder->number_of_compiles_required
		)
		{
			pthread_mutex_unlock(&coder->monitor->monitor_mutex);
			break ;
		}
		pthread_mutex_unlock(&coder->monitor->monitor_mutex);
		pthread_mutex_lock(&coder->monitor->scheduler_mutex);
		if (!coder->turn)
			book_a_slot(coder);
		if (!keep_going_after_waiting_turn(coder))
			break ;
		coder->turn = 0;
		pthread_mutex_unlock(&coder->monitor->scheduler_mutex);
		if (!access_dongle(coder, ts))
			break ;
		debug_timestamp = coder_is_compiling(coder, debug_timestamp);
		coder_is_debugging(coder, debug_timestamp);
		coder_is_refactoring(coder);
	}
	return (NULL);
}
