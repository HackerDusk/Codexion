/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder_actions.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 02:48:28 by srandro           #+#    #+#             */
/*   Updated: 2026/09/06 02:48:29 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "codexion.h"

long long	coder_is_compiling(t_coder *coder, long long debug_timestamp)
{
	struct timespec	ts;
	long long		target;
	long long		timestamp;

	timestamp = get_time_ms() - coder->monitor->start_time;
	pthread_mutex_lock(&coder->monitor->print_mutex);
	if (!is_simulation_stopped(coder->monitor))
		fprintf(stdout, "%lld %d is compiling\n",
		timestamp, coder->id);
	pthread_mutex_unlock(&coder->monitor->print_mutex);
	pthread_mutex_lock(&coder->monitor->monitor_mutex);
	pthread_mutex_lock(&coder->monitor->scheduler_mutex);
	coder->last_compile_start  = get_time_ms();
	pthread_mutex_unlock(&coder->monitor->scheduler_mutex);
	target = coder->last_compile_start + coder->time_to_compile;
	while (get_time_ms() < target && !is_simulation_stopped(coder->monitor))
	{
		ms_to_timespec( target, &ts);
		pthread_cond_timedwait(&coder->monitor->monitor_cond, &coder->monitor->monitor_mutex, &ts);
	}
	coder->compiles_done += 1;
	debug_timestamp = get_time_ms() - coder->monitor->start_time;
	pthread_cond_broadcast(&coder->monitor->monitor_cond);
	pthread_mutex_unlock(&coder->monitor->monitor_mutex);
	return (debug_timestamp);
}

void	coder_is_debugging(t_coder *coder, long long debug_timestamp)
{
	struct timespec	ts;
	long long		target;

	pthread_mutex_lock(&coder->monitor->print_mutex);
	if (!is_simulation_stopped(coder->monitor))
		fprintf(stdout, "%lld %d is debugging\n",
		debug_timestamp, coder->id);
	pthread_mutex_unlock(&coder->monitor->print_mutex);
	release_dongles(coder);
	pthread_mutex_lock(&coder->monitor->monitor_mutex);
	target = get_time_ms() + coder->time_to_debug;
	while (get_time_ms() < target && !is_simulation_stopped(coder->monitor))
	{
		ms_to_timespec(target, &ts);
		pthread_cond_timedwait(&coder->monitor->monitor_cond, &coder->monitor->monitor_mutex, &ts);
	}
	pthread_mutex_unlock(&coder->monitor->monitor_mutex);
}

void	coder_is_refactoring(t_coder *coder)
{
	struct timespec	ts;
	long long		target;
	long long		timestamp;

	timestamp = get_time_ms() - coder->monitor->start_time;
	pthread_mutex_lock(&coder->monitor->print_mutex);
	if (!is_simulation_stopped(coder->monitor))
		fprintf(stdout, "%lld %d is refactoring\n",
		timestamp, coder->id);
	pthread_mutex_unlock(&coder->monitor->print_mutex);
	pthread_mutex_lock(&coder->monitor->monitor_mutex);
	target = get_time_ms() + coder->time_to_refactor;
	while (get_time_ms() < target && !is_simulation_stopped(coder->monitor))
	{
		ms_to_timespec( target, &ts);
		pthread_cond_timedwait(&coder->monitor->monitor_cond, &coder->monitor->monitor_mutex, &ts);
	}
	pthread_mutex_unlock(&coder->monitor->monitor_mutex);
}
