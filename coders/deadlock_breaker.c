/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   deadlock_breaker.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 02:48:38 by srandro           #+#    #+#             */
/*   Updated: 2026/09/08 13:43:08 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	coffman_circular_wait_breaker(t_coder *coder)
{
	if (coder->id % 2 == 0)
	{
		usleep(1000);
		coder->first = coder->left_dongle;
		coder->second = coder->right_dongle;
	}
	else
	{
		coder->first = coder->right_dongle;
		coder->second = coder->left_dongle;
	}
}
