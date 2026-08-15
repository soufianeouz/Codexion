#include "codexion.h"



void lock_dongles(t_coder *coder){

    if (coder->config->number_of_coders == 1)
    {
        pthread_mutex_lock(&coder->right->mutex);
        return;
    }
    if (coder->id % 2 == 0){
        pthread_mutex_lock(&coder->left->mutex);
        pthread_mutex_lock(&coder->right->mutex);
    }
    else{
        pthread_mutex_lock(&coder->right->mutex);
        pthread_mutex_lock(&coder->left->mutex);
    }
}

void unlock_dongles(t_coder *coder){

    if (coder->config->number_of_coders == 1)
    {
        pthread_mutex_unlock(&coder->right->mutex);
        return;
    }
     pthread_mutex_unlock(&coder->left->mutex);
     pthread_mutex_unlock(&coder->right->mutex);
}


int compile_cycle(t_coder *coder)
{
    long long timing;

    take_dongles(coder);
    coder_compile(coder);
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
    if (coder->config->stop == 1)
    {
        pthread_mutex_unlock(&coder->config->mutex_for_stop);
        return (0);
    };
    pthread_mutex_unlock(&coder->config->mutex_for_stop);
    debug(coder);
    refactor(coder);
    return (1);
}



int dongles_ready(t_coder *coder)
{
    long long timing;

    timing = get_time_ms();
    if (timing - coder->left->last_released
        < coder->config->dongle_cooldown)
        return (0);
    if (timing - coder->right->last_released
        < coder->config->dongle_cooldown)
        return (0);
    return (1);
}

void	*coder_thread(void *arg)
{
	t_coder	*a_coder;

	a_coder = (t_coder *)arg;
	while (1)
	{
        pthread_mutex_lock(&a_coder->config->mutex_for_stop);
        if (a_coder->config->stop == 1)
        {
            pthread_mutex_unlock(&a_coder->config->mutex_for_stop);
            break;
        }
    pthread_mutex_unlock(&a_coder->config->mutex_for_stop);
		lock_dongles(a_coder);
		request_dongles(a_coder);
		if (queue_is_first(a_coder->left, a_coder)
			&& queue_is_first(a_coder->right, a_coder))
		{
			if (dongles_ready(a_coder) == 1)
			{
				if (compile_cycle(a_coder) == 0)
					break;
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