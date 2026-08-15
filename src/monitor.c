#include "codexion.h"

int check_burnout(t_config *config){
    int i;
    long long timing;    

    i = 0;
    while (i < config->number_of_coders)
    {
        timing = get_elapsed_time(config);;
        if (timing - config->all_codes[i].last_compile_start
            >= config->time_to_burnout)
        {
            pthread_mutex_lock(&config->mutex_for_printing);
            printf("%lld %d burned out\n",
                timing,
                config->all_codes[i].id);
            pthread_mutex_unlock(&config->mutex_for_printing);
            config->stop = 1;
            return 0;
        }
        i++;
    }
    return 1;
}


int check_completion(t_config *config)
{
    int i;

    i = 0;
    while (i < config->number_of_coders)
    {
        if (config->all_codes[i].compile_count
            < config->number_of_compiles_required)
        {
            config->stop = 0;
            return (0);
        }
        i++;
    }
    return (1);
}

void *monitor(void *arg)
{
    t_config *config = (t_config *)arg;

    while (1)
    {
        pthread_mutex_lock(&config->mutex_for_stop);
        if (check_burnout(config) == 0)
        {
            pthread_mutex_unlock(&config->mutex_for_stop);
            return (NULL);
        }
        config->stop = 1;
        check_completion(config);

        if (config->stop == 1)
        {
            pthread_mutex_unlock(&config->mutex_for_stop);
            return NULL;
        }

        pthread_mutex_unlock(&config->mutex_for_stop);
        usleep(1000);
    }
    return NULL;
}
