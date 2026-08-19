/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: selouizg <selouizg@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 11:41:46 by selouizg          #+#    #+#             */
/*   Updated: 2026/08/19 12:27:12 by selouizg         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int check_the_stop(t_coder *coder){
	if (coder->config->stop == 1)
	{
		return 0;
		pthread_mutex_unlock(&coder->config->mutex_for_stop);
	}
	
	return 1;
	
}


int	compile_cycle(t_coder *coder)
{
	long long	timing;

	take_dongles(coder);
	if (check_the_stop(coder) == 0)
		return 0;
	coder_compile(coder);
	if (check_the_stop(coder) == 0)
		return 0;
	queue_pop(coder->left);
	queue_pop(coder->right);
	timing = get_time_ms();
	coder->right->last_released = timing;
	coder->left->last_released = timing;
	unlock_dongles(coder);
	pthread_mutex_lock(&coder->config->mutex_for_stop);
	coder->compile_count++;
	pthread_mutex_unlock(&coder->config->mutex_for_stop);
	pthread_mutex_lock(&coder->config->mutex_for_stop);
	if (check_the_stop(coder) == 0)
		return 0;
	
	pthread_mutex_unlock(&coder->config->mutex_for_stop);
	debug(coder);
	if (check_the_stop(coder) == 0)
		return 0;
	refactor(coder);
	return (1);
}

int	dongles_ready(t_coder *coder)
{
	long long	timing;

	timing = get_time_ms();
	if (timing - coder->left->last_released
		< coder->config->dongle_cooldown)
		return (0);
	if (timing - coder->right->last_released
		< coder->config->dongle_cooldown)
		return (0);
	return (1);
}

int	check_stop_and_acquire(t_coder *coder)
{
	pthread_mutex_lock(&coder->config->mutex_for_stop);
	if (coder->config->stop == 1)
	{
		pthread_mutex_unlock(&coder->config->mutex_for_stop);
		return (0);
	}
	pthread_mutex_unlock(&coder->config->mutex_for_stop);
	lock_dongles(coder);
	request_dongles(coder);
	return (1);
}

void	*coder_thread(void *arg)
{
	t_coder	*a_coder;

	a_coder = (t_coder *)arg;
	while (1)
	{
		if (check_stop_and_acquire(a_coder) == 0)
			break ;
		if (queue_is_first(a_coder->left, a_coder)
			&& queue_is_first(a_coder->right, a_coder))
		{
			if (dongles_ready(a_coder) == 1)
			{
				if (compile_cycle(a_coder) == 0)
					break ;
			}
			else
				wait_dongles(a_coder);
		}
		else
		{
			unlock_dongles(a_coder);
			usleep(1000);
		}
	}
	return (NULL);
}
