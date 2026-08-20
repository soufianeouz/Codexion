/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cleanup.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: selouizg <selouizg@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 11:34:14 by selouizg          #+#    #+#             */
/*   Updated: 2026/08/20 12:59:14 by selouizg         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	free_function(t_config *config)
{
	destroy_config_mutexes(config);
	free(config->threads);
	free(config->all_dongles);
	free(config->all_codes);
}

void	destroy_dongle_mutexes(t_dongle *dongles, int count)
{
	while (count > 0)
	{
		count--;
		pthread_mutex_destroy(&dongles[count].mutex);
	}
}

void	destroy_config_mutexes(t_config *config)
{
	pthread_mutex_destroy(&config->mutex_for_stop);
	pthread_mutex_destroy(&config->mutex_for_printing);
}
