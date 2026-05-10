/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/08 14:30:29 by srandro           #+#    #+#             */
/*   Updated: 2026/05/10 17:45:07 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H
# include <pthread.h>
# include <stdio.h>
# include <sys/time.h>
# include <string.h>
# include <stdlib.h>

typedef struct s_params
{
	int	number_of_coders;
	int	time_to_burnout;
	int	time_to_compile;
	int	time_to_debug;
	int	time_to_refactor;
	int	number_of_compiles_required;
	int	dongle_cooldown;
	char	*scheduler;
}	t_params;

typedef struct s_dongle
{
	
	int	available;
	long	cooldown_end;
	long	dongle_cooldown;
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;

}	t_dongle;

typedef struct s_sim t_sim;
typedef struct s_coder
{
	int		n_coder;
	int		n_compilation;
	t_dongle    *left_dongle;
	t_dongle    *right_dongle;
	long	last_compile_time;
	t_sim   *sim;
}	t_coder;

typedef struct s_sim
{
    t_params    params;
    t_dongle    *dongles;
	t_coder     *coders;
	pthread_mutex_t log_mutex;
    long        start_time;
}   t_sim;

#endif
