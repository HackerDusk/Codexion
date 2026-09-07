/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_access.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 02:48:44 by srandro           #+#    #+#             */
/*   Updated: 2026/09/07 11:11:25 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	continue_after_first_dongle_access(t_coder *coder)
{
	struct timespec	ts;

	while (
		(!coder->first->is_free || get_time_ms() < coder->first->available_at)
		&& !is_simulation_stopped(coder->monitor))
	{
		if (get_time_ms() < coder->first->available_at)
		{
			ms_to_timespec(coder->first->available_at, &ts);
			pthread_cond_timedwait(
				&coder->first->dongle_cond,
				&coder->first->dongle_mutex, &ts);
		}
		else
			pthread_cond_wait(&coder->first->dongle_cond,
				&coder->first->dongle_mutex);
	}
	if (!taking_first_dongle(coder))
		return (0);
	return (1);
}

int	one_coder_case(t_coder *coder)
{
	if (coder->monitor->nb_coders == 1)
	{
		pthread_mutex_unlock(&coder->first->dongle_mutex);
		while (!is_simulation_stopped(coder->monitor))
			usleep(1000);
		return (1);
	}
	return (0);
}

int	continue_after_second_dongle_access(t_coder *coder)
{
	struct timespec	ts;

	while (
		(!coder->second->is_free || get_time_ms() < coder->second->available_at)
		&& !is_simulation_stopped(coder->monitor))
	{
		if (get_time_ms() < coder->second->available_at)
		{
			ms_to_timespec(coder->second->available_at, &ts);
			pthread_cond_timedwait(
				&coder->second->dongle_cond, &coder->second->dongle_mutex, &ts);
		}
		else
			pthread_cond_wait(&coder->second->dongle_cond,
				&coder->second->dongle_mutex);
	}
	if (!taking_second_dongle(coder))
		return (0);
	return (1);
}

int	access_dongle(t_coder *coder)
{
	coffman_circular_wait_breaker(coder);
	pthread_mutex_lock(&coder->first->dongle_mutex);
	if (!continue_after_first_dongle_access(coder))
	{
		pthread_mutex_unlock(&coder->first->dongle_mutex);
		return (0);
	}
	if (one_coder_case(coder))
		return (0);
	pthread_mutex_unlock(&coder->first->dongle_mutex);
	pthread_mutex_lock(&coder->second->dongle_mutex);
	if (!continue_after_second_dongle_access(coder))
		return (0);
	pthread_mutex_unlock(&coder->second->dongle_mutex);
	return (1);
}

void	release_dongles(t_coder *coder)
{
	pthread_mutex_lock(&coder->left_dongle->dongle_mutex);
	coder->left_dongle->available_at = (get_time_ms()
			+ coder->left_dongle->dongle_cooldown);
	coder->left_dongle->is_free = 1;
	pthread_cond_broadcast(&coder->left_dongle->dongle_cond);
	pthread_mutex_unlock(&coder->left_dongle->dongle_mutex);
	pthread_mutex_lock(&coder->right_dongle->dongle_mutex);
	coder->right_dongle->available_at = (get_time_ms()
			+ coder->right_dongle->dongle_cooldown);
	coder->right_dongle->is_free = 1;
	pthread_cond_broadcast(&coder->right_dongle->dongle_cond);
	pthread_mutex_unlock(&coder->right_dongle->dongle_mutex);
}
