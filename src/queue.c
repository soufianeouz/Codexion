/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   queue.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: selouizg <selouizg@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 13:13:27 by selouizg          #+#    #+#             */
/*   Updated: 2026/08/21 03:10:40 by selouizg         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	queue_is_first(t_dongle *dongle, t_coder *coder)
{
	if (dongle->queue[0] == coder)
		return (1);
	return (0);
}

void	queue_pop(t_dongle *dongle)
{
	dongle->queue[0] = dongle->queue[1];
	dongle->queue[1] = NULL;
}
