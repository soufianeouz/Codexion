/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cleanup.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: selouizg <selouizg@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 11:34:14 by selouizg          #+#    #+#             */
/*   Updated: 2026/08/21 01:50:49 by selouizg         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	free_function(t_config *config)
{
	destroy_config_mutexes(config);
	destroy_dongle_mutexes(config->all_dongles, config->number_of_coders);
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

void	stop_and_join_threads(t_config *config, int count)
{
	pthread_mutex_lock(&config->mutex_for_stop);
	config->stop = 1;
	pthread_mutex_unlock(&config->mutex_for_stop);
	while (--count >= 0)
		pthread_join(config->threads[count], NULL);
}

