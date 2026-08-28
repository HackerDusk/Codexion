/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/25 17:04:13 by srandro           #+#    #+#             */
/*   Updated: 2026/08/26 14:59:56 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H

# include <stdio.h>
# include <unistd.h>
# include <stdlib.h>
# include <string.h>
# include <pthread.h>
# include <limits.h>
# include <sys/time.h>
# include <errno.h>
# include <stddef.h>

typedef struct s_dongle
{
	int				id;
	int				dongle_cooldown;
	int				is_free;
	pthread_mutex_t	mutex;
	pthread_cond_t	dongle_cond;
}	t_dongle;

typedef struct s_monitor	t_monitor;

typedef struct s_coder
{
	pthread_t	thread;
	t_dongle	*left_dongle;
	t_dongle	*right_dongle;
	t_monitor	*monitor;
	int			id;
	int			time_to_burnout;
	int			time_to_compile;
	int			time_to_debug;
	int			time_to_refactor;
	int			number_of_compiles_required;
	int			compliles_done;
	int			curr_time_before_burnout;
}	t_coder;

typedef struct s_monitor
{
	pthread_mutex_t	monitor_mutex;
	t_dongle		*dongles;
	t_coder			*coders;
	int				nb_coders;
	int				scheduler_type;
	int				stop_simulation;
	long long		start_time;
	pthread_cond_t	monitor_cond;
	pthread_t		monitor_thread;
}	t_monitor;

int			full_arg_checker(int argc, char **argv);
t_monitor	*monitor_initializer(char **argv);

#endif
