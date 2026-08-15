#include "codexion.h"

long long get_time_ms(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}
long long get_elapsed_time(t_config *config)
{
    return (get_time_ms() - config->start_time);
}

void wait_dongles(t_coder *coder)
{
    pthread_mutex_lock(&coder->config->mutex_for_stop);
    coder->state = WAITING;
    pthread_mutex_unlock(&coder->config->mutex_for_stop);
    unlock_dongles(coder);
    usleep(1000);
}

void take_dongles(t_coder *coder)
{
    long long timing;

    timing = get_elapsed_time(coder->config);

    pthread_mutex_lock(&coder->config->mutex_for_printing);

    printf("%lld %d has taken a dongle\n", timing, coder->id);
    printf("%lld %d has taken a dongle\n", timing, coder->id);

    pthread_mutex_unlock(&coder->config->mutex_for_printing);
}

void coder_compile(t_coder *coder)
{
    long long timing;

    timing = get_time_ms();

    pthread_mutex_lock(&coder->config->mutex_for_stop);
    coder->state = COMPILING;
    coder->last_compile_start = timing;
    timing = get_elapsed_time(coder->config);
    pthread_mutex_unlock(&coder->config->mutex_for_stop);

    pthread_mutex_lock(&coder->config->mutex_for_printing);
    printf("%lld %d is compiling\n", timing, coder->id);
    pthread_mutex_unlock(&coder->config->mutex_for_printing);

    usleep(coder->config->time_to_compile * 1000);
}

void debug(t_coder *coder){

    long long timing;

    timing = get_elapsed_time(coder->config);;
    pthread_mutex_lock(&coder->config->mutex_for_stop);
    coder->state = DEBUGGING;
    pthread_mutex_unlock(&coder->config->mutex_for_stop);
    pthread_mutex_lock(&coder->config->mutex_for_printing);
    printf("%lld %d is debugging\n",timing ,coder->id);
    pthread_mutex_unlock(&coder->config->mutex_for_printing);
    usleep(coder->config->time_to_debug * 1000);

}

void refactor(t_coder *coder)
{
    long long timing;

    pthread_mutex_lock(&coder->config->mutex_for_stop);
    coder->state = REFACTORING;
    pthread_mutex_unlock(&coder->config->mutex_for_stop);
    timing = get_elapsed_time(coder->config);;

    pthread_mutex_lock(&coder->config->mutex_for_printing);
    printf("%lld %d is refactoring\n", timing, coder->id);
    pthread_mutex_unlock(&coder->config->mutex_for_printing);

    usleep(coder->config->time_to_refactor * 1000);
}