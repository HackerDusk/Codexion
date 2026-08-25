/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: srandro <srandro@student.42antananarivo    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/25 17:03:49 by srandro           #+#    #+#             */
/*   Updated: 2026/08/26 01:32:46 by srandro          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

// void	models_initializer(char	**argv)
// {
// 	t_dongle	*dongle;
// 	t_coder		*coder;	
// }
int	main(int argc, char **argv)
{
	if (!full_arg_checker(argc, argv))
		return (1);
	// models_initializer(argv);
	return (0);
}
