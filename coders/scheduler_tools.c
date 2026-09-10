/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   scheduler_tools.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 22:23:21 by srandro           #+#    #+#             */
/*   Updated: 2026/09/10 10:14:10 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	cmp_fifo(t_coder *cd_a, t_coder *cd_b)
{
	return (cd_a->request_time < cd_b->request_time);
}

int	cmp_edf(t_coder *cd_a, t_coder *cd_b)
{
	long long	deadline_a;
	long long	deadline_b;

	deadline_a = cd_a->last_compile_start + cd_a->time_to_burnout;
	deadline_b = cd_b->last_compile_start + cd_b->time_to_burnout;
	if (deadline_a != deadline_b)
		return (deadline_a < deadline_b);
	else
		return (cd_a->id < cd_b->id);
}
