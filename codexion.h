/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/08 14:30:29 by srandro           #+#    #+#             */
/*   Updated: 2026/05/11 14:45:03 by srandro          ###   ########.fr       */
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
	char	*scheduler;
	int		number_of_coders;
	int		time_to_burnout;
	int		time_to_compile;
	int		time_to_debug;
	int		time_to_refactor;
	int		number_of_compiles_required;
	int		dongle_cooldown;
}	t_params;

typedef struct s_dongle
{
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
	long			cooldown_end;
	long			dongle_cooldown;
	int				available;

}	t_dongle;

typedef struct s_sim	t_sim;
typedef struct s_coder
{
	t_dongle	*left_dongle;
	t_dongle	*right_dongle;
	t_sim		*sim;
	long		last_compile_time;
	int			n_coder;
	int			n_compilation;
}	t_coder;

typedef struct s_sim
{
	pthread_mutex_t	log_mutex;
	t_params		params;
	t_dongle		*dongles;
	t_coder			*coders;
	long			start_time;
}	t_sim;

int		init_sim(t_sim *sim, char **argv);
void	free_sim(t_sim *sim);
int		setup(t_sim *sim, pthread_t **threads, char **argv);

#endif
