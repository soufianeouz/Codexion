/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: selouizg <selouizg@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 13:11:00 by selouizg          #+#    #+#             */
/*   Updated: 2026/08/16 13:11:35 by selouizg         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	main(int argc, char **argv)
{
	int			i;
	t_config	config;

	if (!parse_arguments(argc, argv, &config))
		return (1);
	if (!init_program(&config))
		return (1);
	i = 0;
	while (i < config.number_of_coders)
	{
		pthread_join(config.threads[i], NULL);
		i++;
	}
	pthread_join(config.monitor_thread, NULL);
	free_function(&config);
	return (0);
}
