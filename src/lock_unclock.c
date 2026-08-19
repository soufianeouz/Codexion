/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lock_unclock.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: selouizg <selouizg@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 12:00:53 by selouizg          #+#    #+#             */
/*   Updated: 2026/08/19 15:34:35 by selouizg         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	lock_dongles(t_coder *coder)
{
	if (coder->config->number_of_coders == 1)
	{
		pthread_mutex_lock(&coder->right->mutex);
		return ;
	}
	if (coder->id % 2 == 0)
	{
		pthread_mutex_lock(&coder->left->mutex);
		pthread_mutex_lock(&coder->right->mutex);
	}
	else
	{
		pthread_mutex_lock(&coder->right->mutex);
		pthread_mutex_lock(&coder->left->mutex);
	}
}

void	unlock_dongles(t_coder *coder)
{
	pthread_mutex_unlock(&coder->left->mutex);
	pthread_mutex_unlock(&coder->right->mutex);
}

int	check_the_stop(t_coder *coder)
{
	pthread_mutex_lock(&coder->config->mutex_for_stop);
	if (coder->config->stop == 1)
	{
		pthread_mutex_unlock(&coder->config->mutex_for_stop);
		unlock_dongles(coder);
		return (0);
	}
	pthread_mutex_unlock(&coder->config->mutex_for_stop);
	return (1);
}
