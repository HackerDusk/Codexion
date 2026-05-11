/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/11 14:45:38 by srandro           #+#    #+#             */
/*   Updated: 2026/05/11 14:46:49 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	free_sim(t_sim *sim)
{
	if (sim->dongles)
		free(sim->dongles);
	if (sim->coders)
		free(sim->coders);
}

int	setup(t_sim *sim, pthread_t **threads, char **argv)
{
	if (!init_sim(sim, argv))
		return (0);
	*threads = malloc(sizeof(pthread_t) * sim->params.number_of_coders);
	if (!*threads)
	{
		free_sim(sim);
		return (0);
	}
	return (1);
}
