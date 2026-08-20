/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   functions_init.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: selouizg <selouizg@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 13:08:41 by selouizg          #+#    #+#             */
/*   Updated: 2026/08/19 22:37:00 by selouizg         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	init_mutexes(t_config *config)
{
	if (pthread_mutex_init(&config->mutex_for_stop, NULL) != 0)
		return (0);
	if (pthread_mutex_init(&config->mutex_for_printing, NULL) != 0)
	{
		pthread_mutex_destroy(&config->mutex_for_stop);
		return (0);
	}
	return (1);
}

int	init_threads(t_config *config)
{
	int	i;

	config->threads = malloc(sizeof(pthread_t) * config->number_of_coders);
	if (config->threads == NULL)
		return (0);
	i = 0;
	while (i < config->number_of_coders)
	{
		if (pthread_create(&config->threads[i], NULL,
				coder_thread, &config->all_codes[i]) != 0)
			return (0);
		i++;
	}
	pthread_create(&config->monitor_thread, NULL, monitor, config);
	return (1);
}

int	init_dongles(t_config *config)
{
	int			i;
	t_dongle	*my_dongles;

	my_dongles = malloc(sizeof(t_dongle) * config->number_of_coders);
	if (my_dongles == NULL)
		return (0);
	i = 0;
	while (i < config->number_of_coders)
	{
		if (pthread_mutex_init(&my_dongles[i].mutex, NULL) != 0)
		{
			destroy_dongle_mutexes(my_dongles, i);
			free(my_dongles);
			return (0);
		}
		my_dongles[i].last_released = 0;
		my_dongles[i].waiter_count = 0;
		my_dongles[i].queue[0] = NULL;
		my_dongles[i].queue[1] = NULL;
		i++;
	}
	config->all_dongles = my_dongles;
	return (1);
}

void	helper_init_coder(t_config *config, t_coder *coder)
{
	int			i;
	long long	timing;

	i = 0;
	timing = get_elapsed_time(config);
	while (i < config->number_of_coders)
	{
		coder[i].id = i;
		coder[i].compile_count = 0;
		coder[i].last_compile_start = timing;
		coder[i].state = WAITING;
		coder[i].config = config;
		if (i == 0)
			coder[i].left = config->all_dongles
				+ (config->number_of_coders - 1);
		else
			coder[i].left = config->all_dongles + (i - 1);
		coder[i].right = config->all_dongles + i;
		i++;
	}
}

int	init_coders(t_config *config)
{
	t_coder	*my_coders;

	my_coders = malloc(sizeof(t_coder) * config->number_of_coders);
	if (my_coders == NULL)
		return (0);
	helper_init_coder(config, my_coders);
	config->all_codes = my_coders;
	return (1);
}
