#ifndef CODEXION_H
#define CODEXION_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

struct timeval tv;
typedef struct s_coder t_coder;
typedef struct s_dongle t_dongle;
typedef struct s_config t_config;


typedef enum e_state
{
    WAITING,
    COMPILING,
    DEBUGGING,
    REFACTORING,
    BURNED_OUT
} t_state;


struct s_coder
{
    int id;
    int compile_count;
    long long last_compile_start;
    t_state state;
    t_dongle *left;
    t_dongle *right;

    t_config *config;
};

struct s_dongle
{
    pthread_mutex_t mutex;
    long long last_released;
    int waiter_count;

    t_coder *queue[2];
    
};
struct s_config
{
    int number_of_coders;
    int time_to_burnout;
    int time_to_compile;
    int time_to_debug;
    int time_to_refactor;
    int number_of_compiles_required;
    int dongle_cooldown;
    char *scheduler;

    pthread_mutex_t mutex_for_stop;
    pthread_mutex_t mutex_for_printing;
    
    int stop;
    t_coder *all_codes;
    t_dongle *all_dongles;
};


#endif