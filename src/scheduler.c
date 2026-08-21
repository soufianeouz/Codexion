/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   scheduler.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: selouizg <selouizg@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 13:14:22 by selouizg          #+#    #+#             */
/*   Updated: 2026/08/20 18:38:17 by selouizg         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	fifo_request(t_dongle *dongle, t_coder *coder)
{
	int	i;

	i = 0;
	while (i < 2)
	{
		if (dongle->queue[i] == coder)
			return ;
		i++;
	}
	if (dongle->queue[0] == NULL)
	{
		dongle->queue[0] = coder;
	}
	else if (dongle->queue[1] == NULL)
	{
		dongle->queue[1] = coder;
	}
}

int	deplucate_check(t_dongle *dongle, t_coder *coder)
{
	int	i;

	i = 0;
	while (i < 2)
	{
		if (dongle->queue[i] == coder)
			return (0);
		i++;
	}
	return (1);
}

void	edf_request(t_dongle *dongle, t_coder *coder)
{
	long long	deadline1;
	long long	deadline2;

	if (deplucate_check(dongle, coder) == 0)
		return ;
	if (dongle->queue[0] == NULL)
	{
		dongle->queue[0] = coder;
	}
	else
	{
		deadline1 = dongle->queue[0]->last_compile_start
			+ dongle->queue[0]->config->time_to_burnout;
		deadline2 = coder->last_compile_start
			+ coder->config->time_to_burnout;
		if (deadline2 < deadline1)
		{
			dongle->queue[1] = dongle->queue[0];
			dongle->queue[0] = coder;
		}
		else
			dongle->queue[1] = coder;
	}
}

void	request_dongles(t_coder *coder)
{
	if (strcmp(coder->config->scheduler, "fifo") == 0)
	{
		fifo_request(coder->left, coder);
		fifo_request(coder->right, coder);
	}
	else if (strcmp(coder->config->scheduler, "edf") == 0)
	{
		edf_request(coder->left, coder);
		edf_request(coder->right, coder);
	}
}
