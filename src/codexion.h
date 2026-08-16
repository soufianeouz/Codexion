/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: selouizg <selouizg@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 18:32:11 by selouizg          #+#    #+#             */
/*   Updated: 2026/08/16 18:32:18 by selouizg         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H

# include <pthread.h>
# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <sys/time.h>
# include <string.h>

typedef struct s_coder	t_coder;
typedef struct s_dongle	t_dongle;
typedef struct s_config	t_config;

typedef enum e_state
{
	WAITING,
	COMPILING,
	DEBUGGING,
	REFACTORING,
	BURNED_OUT
}	t_state;

struct s_coder
{
	int			id;
	int			compile_count;
	long long	last_compile_start;
	t_state		state;
	t_dongle	*left;
	t_dongle	*right;
	t_config	*config;
};

struct s_dongle
{
	pthread_mutex_t	mutex;
	long long		last_released;
	int				waiter_count;
	t_coder			*queue[2];
};

struct s_config
{
	int				number_of_coders;
	int				time_to_burnout;
	int				time_to_compile;
	int				time_to_debug;
	int				time_to_refactor;
	int				number_of_compiles_required;
	int				dongle_cooldown;
	char			*scheduler;
	pthread_mutex_t	mutex_for_stop;
	pthread_mutex_t	mutex_for_printing;
	int				stop;
	long long		start_time;
	t_coder			*all_codes;
	t_dongle		*all_dongles;
	pthread_t		*threads;
	pthread_t		monitor_thread;
};

void		lock_dongles(t_coder *coder);
void		unlock_dongles(t_coder *coder);
int			init_mutexes(t_config *config);
long long	get_elapsed_time(t_config *config);
void		*coder_thread(void *arg);
void		*monitor(void *arg);
long long	get_time_ms(void);
void		wait_dongles(t_coder *coder);
void		take_dongles(t_coder *coder);
void		coder_compile(t_coder *coder);
void		debug(t_coder *coder);
void		refactor(t_coder *coder);
void		edf_request(t_dongle *dongle, t_coder *coder);
void		fifo_request(t_dongle *dongle, t_coder *coder);
int			init_dongles(t_config *config);
int			init_coders(t_config *config);
int			init_threads(t_config *config);
int			init_program(t_config *config);
int			check_burnout(t_config *config);
int			check_completion(t_config *config);
int			parse_arguments(int argc, char **argv, t_config *config);
int			queue_is_first(t_dongle *dongle, t_coder *coder);
void		queue_pop(t_dongle *dongle);
void		request_dongles(t_coder *coder);
void		destroy_config_mutexes(t_config *config);
void		destroy_dongle_mutexes(t_dongle *dongles, int count);
void		free_function(t_config *config);

#endif
