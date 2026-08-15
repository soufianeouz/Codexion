#include "codexion.h"

int init_mutexes(t_config *config)
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

// void destroy_dongle_mutexes(t_dongle *dongles, int count)
// {
//     while (count > 0)
//     {
//         count--;
//         pthread_mutex_destroy(&dongles[count].mutex);
//     }
// }

int init_dongles(t_config *config)
{
    int i;

    t_dongle *my_dongles; 
    my_dongles = malloc(sizeof(t_dongle) * config->number_of_coders);
    if (my_dongles == NULL)
        return 0;
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
    return 1;
}
int init_coders(t_config *config){
    int i;
    long long timing;
    struct timeval tv;

    i = 0;
    t_coder *my_coders = malloc(sizeof(t_coder) * config->number_of_coders);
    if (my_coders == NULL)
    return (0);
    gettimeofday(&tv, NULL);
    timing = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    while (i < config->number_of_coders)
    {
        my_coders[i].id = i;
        my_coders[i].compile_count = 0;
        my_coders[i].last_compile_start = timing;
        my_coders[i].state = WAITING;
        my_coders[i].config = config;
        if (i == 0)
            my_coders[i].left = config->all_dongles +( config->number_of_coders - 1);
        else
            my_coders[i].left = config->all_dongles + (i - 1);
        my_coders[i].right = config->all_dongles + i;
        i++;
    }
    config->all_codes = my_coders;
    return 1;
}


int init_threads(t_config *config)
{
    int i;

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

    pthread_create(&config->monitor_thread,
        NULL, monitor, config);

    return (1);
}

int init_program(t_config *config)
{
    if (!init_mutexes(config))
        return (0);
    if (!init_dongles(config))
        return (0);

    if (!init_coders(config))
    {
        free(config->all_dongles);
        return (0);
    }
    if (!init_threads(config))
    {
        free_function(config);
        return (0);
    }
    return (1);
}