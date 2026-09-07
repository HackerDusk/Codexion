/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder_routine_simulator.c                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 22:55:15 by srandro           #+#    #+#             */
/*   Updated: 2026/09/06 23:14:19 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	coder_routine_simulator(t_monitor *monitor)
{
	start_time_initializer(monitor);
	thread_creator(monitor, coder_routine);
	thread_joiner(monitor);
	cond_mutex_destroyer(monitor);
	free_models(monitor);
}
