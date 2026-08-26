/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/25 17:03:49 by srandro           #+#    #+#             */
/*   Updated: 2026/08/26 15:08:48 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

long long	get_time_ms()
{
	struct timeval	tv;

	gettimeofday(&tv, NULL);
	return ((long long)tv.tv_sec * 1000 + tv.tv_usec / 1000);
	
}

void	*coder_routine(void *arg)
{
	t_coder			*coder;
	int				taken;
	long long		start;
	long long		end;
	struct timespec	ts;
	struct timeval	tv;
	int	ret;

	coder = (t_coder *)arg;
	taken = 0;
	ret = 0;
	while(!taken)
	{
		pthread_mutex_lock(&coder->left_dongle->mutex);
		pthread_mutex_lock(&coder->right_dongle->mutex);
		start = get_time_ms();
		while (!coder->left_dongle->is_free)
		{
			gettimeofday(&tv, NULL);
			ts.tv_sec = tv.tv_sec + coder->time_to_burnout;
			ts.tv_nsec = tv.tv_usec * 1000;
			pthread_cond_wait(&coder->left_dongle->cond, &coder->left_dongle->mutex);
			ret = pthread_cond_timedwait(&coder->left_dongle->cond,
				&coder->left_dongle->mutex, &ts);
			if (ret == ETIMEDOUT)
			{
				end = get_time_ms();
				fprintf(stdout, "%lld %d burned out\n", end - start, coder->id);
				return (NULL);
			}
		}
		while (!coder->right_dongle->is_free)
		{
			gettimeofday(&tv, NULL);
			ts.tv_sec = tv.tv_sec + coder->time_to_burnout;
			ts.tv_nsec = tv.tv_usec * 1000;
			pthread_cond_wait(&coder->right_dongle->cond, &coder->right_dongle->mutex);
			ret = pthread_cond_timedwait(&coder->right_dongle->cond,
				&coder->right_dongle->mutex, &ts);
			if (ret == ETIMEDOUT)
			{
				end = get_time_ms();
				fprintf(stdout, "%lld %d burned out\n", end - start, coder->id);
				return (NULL);
			}
		}
		coder->left_dongle->is_free = 0;
		coder->right_dongle->is_free = 0;
		taken = 1;
		end = get_time_ms();
		fprintf(stdout, "%lld %d taken a dongle\n", end - start, coder->id);
		fprintf(stdout, "%lld %d is compiling\n", end - start, coder->id);
		pthread_mutex_unlock(&coder->right_dongle->mutex);
		pthread_mutex_unlock(&coder->left_dongle->mutex);
	}
	pthread_mutex_lock(&coder->left_dongle->mutex);
	coder->left_dongle->is_free = 1;
	pthread_cond_signal(&coder->left_dongle->cond);
	pthread_mutex_unlock(&coder->left_dongle->mutex);
	pthread_mutex_lock(&coder->right_dongle->mutex);
	coder->right_dongle->is_free = 1;
	pthread_cond_signal(&coder->right_dongle->cond);
	pthread_mutex_unlock(&coder->right_dongle->mutex);
	return (NULL);
}
void	simualtor(t_monitor *monitor, void *(*coder_routine)(void *))
{
	int	i;

	i = 0;
	while (i < monitor->nb_coders)
	{
		pthread_create(&monitor->coders[i].thread, NULL, coder_routine,
			&monitor->coders[i].thread);
		i++;
	}
	i = 0;
	while (i < monitor->nb_coders)
	{
		pthread_join(monitor->coders[i].thread, NULL);
		pthread_mutex_destroy(&monitor->dongles[i].mutex);
		pthread_cond_destroy(&monitor->dongles[i].cond);
		pthread_mutex_destroy(&monitor->sim_mutex);
		i++;
	}
}
int	main(int argc, char **argv)
{
	t_monitor	*monitor;

	if (!full_arg_checker(argc, argv))
		return (1);
	monitor = monitor_initializer(argv);
	if (!monitor)
		return (1);
	simualtor(monitor, coder_routine);
	return (0);
}
