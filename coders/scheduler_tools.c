/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   scheduler_tools.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 22:23:21 by srandro           #+#    #+#             */
/*   Updated: 2026/09/08 14:50:58 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	cmp_fifo(t_coder *cd_a, t_coder *cd_b)
{
	return (cd_a->request_time < cd_b->request_time);
}

int	cmp_edf(t_coder *cd_a, t_coder *cd_b)
{
	return ((cd_a->last_compile_start
			+ cd_a->time_to_burnout) < (
			cd_b->last_compile_start
			+ cd_b->time_to_burnout));
}

void	get_next_coder(t_monitor *monitor)
{
	t_coder	*next;

	next = heap_pop(monitor->heap);
	if (!next)
		return ;
	next->turn = 1;
	pthread_cond_signal(&next->turn_cond);
}

void	book_a_slot(t_coder *coder)
{
	coder->request_time = get_time_ms();
	heap_push(coder->monitor->heap, coder);
	pthread_cond_signal(&coder->monitor->scheduler_cond);
}
