#include "codexion.h"


int valid_int(char *str){
    int i = 0;

    if (str[0] == '\0')
        return 0;
    while (str[i] != '\0')
    {
        if (!(str[i] <= '9' && str[i] >= '0'))
            return 0;
        i++;
    }

    return 1;
}

int check_arguments(int argc, char **argv){
    int i;

    i = 1;
    if (argc != 9){
        fprintf(stderr, "Error, number of argument shoulld be 8\n");
        return 0;
    }
    if (strcmp(argv[argc - 1], "fifo") && strcmp(argv[argc - 1], "edf"))
    {
        fprintf(stderr, "Error, the last output shoulld be fifo or edf\n");
        return 0;
    }
    while (i < argc - 1)
    {
        if (valid_int(argv[i]) == 0){
            fprintf(stderr, "Error: invalide input");
            return 0;
        }
        if (i == 1 && atoi(argv[i]) == 0){
            fprintf(stderr, "Error: number_of_coders must be greater than 0\n");
            return 0;
        }
        i++;
    }
    return 1;
}


int parse_arguments(int argc, char **argv, t_config *config){

    if (check_arguments(argc, argv) == 0)
        return (0);
    config->number_of_coders = atoi(argv[1]);
    config->time_to_burnout = atoi(argv[2]);
    config->time_to_compile = atoi(argv[3]);
    config->time_to_debug = atoi(argv[4]);
    config->time_to_refactor = atoi(argv[5]);
    config->number_of_compiles_required = atoi(argv[6]);
    config->dongle_cooldown = atoi(argv[7]);
    
    config->scheduler = argv[8];

    // pthread_mutex_init(&config->mutex_for_stop, NULL);
    // pthread_mutex_init(&config->mutex_for_printing, NULL);
    config->stop = 0;
    
    config->all_codes = NULL;
    config->all_dongles = NULL;

    return (1);
}