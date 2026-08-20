/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   actions.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: selouizg <selouizg@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 11:32:16 by selouizg          #+#    #+#             */
/*   Updated: 2026/08/20 01:21:18 by selouizg         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	wait_dongles(t_coder *coder)
{
	pthread_mutex_lock(&coder->config->mutex_for_stop);
	coder->state = WAITING;
	pthread_mutex_unlock(&coder->config->mutex_for_stop);
	unlock_dongles(coder);
	usleep(1000);
}

void	take_dongles(t_coder *coder)
{
	long long	timing;

	timing = get_elapsed_time(coder->config);
	if (coder->config->number_of_coders == 1)
	{
		while (coder->config->stop == 0)
			usleep(1000);
		return ;
	}
	pthread_mutex_lock(&coder->config->mutex_for_printing);
	printf("%lld %d has taken a dongle\n", timing, coder->id + 1);
	if (coder->config->number_of_coders != 1)
		printf("%lld %d has taken a dongle\n", timing, coder->id + 1);
	pthread_mutex_unlock(&coder->config->mutex_for_printing);
}

void	coder_compile(t_coder *coder)
{
	long long	timing;

	timing = get_elapsed_time(coder->config);
	pthread_mutex_lock(&coder->config->mutex_for_stop);
	coder->state = COMPILING;
	coder->last_compile_start = timing;
	timing = get_elapsed_time(coder->config);
	pthread_mutex_unlock(&coder->config->mutex_for_stop);
	pthread_mutex_lock(&coder->config->mutex_for_printing);
	printf("%lld %d is compiling\n", timing, coder->id + 1);
	pthread_mutex_unlock(&coder->config->mutex_for_printing);
	usleep(coder->config->time_to_compile * 1000);
}

void	debug(t_coder *coder)
{
	long long	timing;

	timing = get_elapsed_time(coder->config);
	pthread_mutex_lock(&coder->config->mutex_for_stop);
	coder->state = DEBUGGING;
	pthread_mutex_unlock(&coder->config->mutex_for_stop);
	pthread_mutex_lock(&coder->config->mutex_for_printing);
	printf("%lld %d is debugging\n", timing, coder->id + 1);
	pthread_mutex_unlock(&coder->config->mutex_for_printing);
	usleep(coder->config->time_to_debug * 1000);
}

void	refactor(t_coder *coder)
{
	long long	timing;

	pthread_mutex_lock(&coder->config->mutex_for_stop);
	coder->state = REFACTORING;
	pthread_mutex_unlock(&coder->config->mutex_for_stop);
	timing = get_elapsed_time(coder->config);
	pthread_mutex_lock(&coder->config->mutex_for_printing);
	printf("%lld %d is refactoring\n", timing, coder->id + 1);
	pthread_mutex_unlock(&coder->config->mutex_for_printing);
	usleep(coder->config->time_to_refactor * 1000);
}
