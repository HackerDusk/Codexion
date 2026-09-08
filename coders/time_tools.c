/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   time_tools.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 02:49:07 by srandro           #+#    #+#             */
/*   Updated: 2026/09/08 13:45:10 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

long long	get_time_ms(void)
{
	struct timeval	tv;

	gettimeofday(&tv, NULL);
	return ((long long)tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

void	ms_to_timespec(long long time_in_ms, struct timespec *ts)
{
	ts->tv_sec = time_in_ms / 1000;
	ts->tv_nsec = (time_in_ms % 1000) * 1000000;
}
