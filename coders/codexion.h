/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/25 17:04:13 by srandro           #+#    #+#             */
/*   Updated: 2026/08/25 22:07:12 by srandro          ###   ########.fr       */
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
// typedef struct s_dongle
// {
//     pthread_mutex_t mutex;
//     int     id;
//     int     dongle_cooldown;
//     int     is_free;
//     struct s_dongle *nex_dongle; 
// }   t_dongle;
// typedef struct s_coder
// {
//     pthread_t   thread;
//     int     id;
//     int     time_to_burnout;
//     int     time_to_compile;
//     int     time_to_debug;
//     int     time_to_refactor;
//     int     number_of_compiles_required;    
// }   t_coder;
int	full_arg_checker(int argc, char **argv);
#endif
