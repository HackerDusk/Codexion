/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mandresy <mandresy@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/25 17:04:13 by srandro           #+#    #+#             */
/*   Updated: 2026/09/06 02:19:16 by mandresy         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H

# include <stdio.h>
# include <stddef.h>
# include <string.h>
# include <unistd.h>
# include <stdlib.h>
# include <pthread.h>
# include <limits.h>
# include <sys/time.h>
# include <errno.h>

typedef struct s_dongle
{
	int				id;
	int				dongle_cooldown;
	int				is_free;
	long long		available_at;
	pthread_mutex_t	dongle_mutex;
	pthread_cond_t	dongle_cond;
	
}	t_dongle;

typedef struct s_monitor	t_monitor;

typedef struct s_coder
{
	int				id;
	int				time_to_burnout;
	int				time_to_compile;
	int				time_to_debug;
	int				time_to_refactor;
	int				turn;
	int				compiles_done;
	int				number_of_compiles_required;
	long long		last_compile_start;
	long long		request_time;
	t_dongle		*left_dongle;
	t_dongle		*right_dongle;
	t_dongle		*first;
	t_dongle		*second;
	pthread_t		coder_thread;
	t_monitor		*monitor;
	pthread_cond_t	turn_cond;
}	t_coder;

typedef struct s_heap
{
	t_coder		**arr;
	size_t		size;
	size_t		capacity;
	int			(*cmp)(t_coder *, t_coder *);
}	t_heap;

typedef struct s_monitor
{
	int					nb_coders;
	long long			start_time;
	int					stop_simulation;
	t_heap				*heap;
	t_coder				*coders;
	t_dongle			*dongles;
	char				*scheduler_type;
	pthread_t			monitor_thread;
	pthread_t			scheduler_thread;
	pthread_mutex_t		monitor_mutex;
	pthread_mutex_t		scheduler_mutex;
	pthread_mutex_t		print_mutex;
	pthread_mutex_t		stop_mutex;
	pthread_cond_t		monitor_cond;
	pthread_cond_t		scheduler_cond;
}	t_monitor;

int			full_arg_checker(int argc, char **argv);
int			heap_initializer(t_monitor *monitor);
t_monitor	*monitor_initializer(char **argv);
void		book_a_slot(t_coder *coder);
void		heap_push(t_heap *heap, t_coder *coder);
t_coder		*heap_pop(t_heap *heap);
int			cmp_fifo(t_coder *cd_a, t_coder *cd_b);
int			cmp_edf(t_coder *cd_a, t_coder *cd_b);
int			is_simulation_stopped(t_monitor *monitor);
void		set_simulation_stopped(t_monitor *monitor);
long long   get_time_ms(void);
void		ms_to_timespec(long long time_in_ms, struct timespec *ts);
int			keep_going_after_waiting(t_coder *coder);
void		coffman_circular_wait_breaker(t_coder *coder);
int			continue_after_first_dongle_access(t_coder *coder ,struct timespec ts);
int			one_coder_case(t_coder *coder);
int			continue_after_second_dongle_access(t_coder *coder, struct timespec ts);
long long	coder_is_compiling(t_coder *coder, long long debug_timestamp);
void	coder_is_debugging(t_coder *coder, long long debug_timestamp);
void	coder_is_refactoring(t_coder *coder);

void		routine_simulator(t_monitor *monitor, void *(*coder_routine)(void *));
int			access_dongle(t_coder *coder, struct timespec ts);
void		release_dongles(t_coder *coder);
void		*coder_routine(void *arg);
void		*monitor_routine(void *arg);
long long	get_closest_deadline(t_monitor *monitor);
void		awake_coders(t_monitor *monitor);
int			is_routine_finished(t_monitor *monitor);
int			check_alert(t_monitor *monitor);
t_coder		*get_next_coder(t_monitor *monitor);
void		*scheduler_routine(void *arg);
void		wake_scheduler(t_monitor *monitor);

#endif
