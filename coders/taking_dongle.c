/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   taking_dongle.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/07 11:06:29 by srandro           #+#    #+#             */
/*   Updated: 2026/09/07 11:23:36 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	taking_first_dongle(t_coder *coder)
{
	if (is_simulation_stopped(coder->monitor))
		return (0);
	pthread_mutex_lock(&coder->monitor->print_mutex);
	if (is_simulation_stopped(coder->monitor))
	{
		pthread_mutex_unlock(&coder->monitor->print_mutex);
		return (0);
	}
	coder->first->is_free = 0;
	fprintf(stdout, "%lld %d has taken a dongle\n",
		get_time_ms() - coder->monitor->start_time, coder->id);
	pthread_mutex_unlock(&coder->monitor->print_mutex);
	return (1);
}

int	taking_second_dongle(t_coder *coder)
{
	if (is_simulation_stopped(coder->monitor))
	{
		pthread_mutex_unlock(&coder->second->dongle_mutex);
		pthread_mutex_lock(&coder->first->dongle_mutex);
		coder->first->available_at = (get_time_ms()
				+ coder->first->dongle_cooldown);
		coder->first->is_free = 1;
		pthread_cond_broadcast(&coder->first->dongle_cond);
		pthread_mutex_unlock(&coder->first->dongle_mutex);
		return (0);
	}
	pthread_mutex_lock(&coder->monitor->print_mutex);
	if (is_simulation_stopped(coder->monitor))
	{
		pthread_mutex_unlock(&coder->monitor->print_mutex);
		return (0);
	}
	coder->second->is_free = 0;
	fprintf(stdout, "%lld %d has taken a dongle\n",
		get_time_ms() - coder->monitor->start_time, coder->id);
	pthread_mutex_unlock(&coder->monitor->print_mutex);
	return (1);
}
