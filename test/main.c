#include "codexion.h"



void *coder_thread(void *arg){


    t_coder *a_coder = (t_coder *)arg;
    struct timeval tv;

    long long timing;
    gettimeofday(&tv, NULL);
    a_coder->last_compile_start = tv.tv_usec / 1000 + tv.tv_sec * 1000;

    while (1)
    {
        
        if (a_coder->id % 2 == 0){
            pthread_mutex_lock(&a_coder->left->mutex);
            pthread_mutex_lock(&a_coder->right->mutex);
        }
        else{
            pthread_mutex_lock(&a_coder->right->mutex);
            pthread_mutex_lock(&a_coder->left->mutex);
        }
        gettimeofday(&tv, NULL);
        timing = tv.tv_usec / 1000 + tv.tv_sec * 1000;
        pthread_mutex_lock(&a_coder->config->mutex_for_printing);
        printf("%lld %d has taken a dongle\n", timing, a_coder->id);
        pthread_mutex_unlock(&a_coder->config->mutex_for_printing);

        gettimeofday(&tv, NULL);
        timing = tv.tv_usec / 1000 + tv.tv_sec * 1000;
        pthread_mutex_lock(&a_coder->config->mutex_for_printing);
        printf("%lld %d has taken a dongle\n",timing ,a_coder->id);
        pthread_mutex_unlock(&a_coder->config->mutex_for_printing);

        a_coder->state = COMPILING;
        gettimeofday(&tv, NULL);
        timing = tv.tv_usec / 1000 + tv.tv_sec * 1000;
        pthread_mutex_lock(&a_coder->config->mutex_for_printing);
        printf("%lld %d is compiling\n",timing ,a_coder->id);
        pthread_mutex_unlock(&a_coder->config->mutex_for_printing);
        usleep(a_coder->config->time_to_compile * 1000);


        pthread_mutex_unlock(&a_coder->right->mutex);
        pthread_mutex_unlock(&a_coder->left->mutex);
        a_coder->compile_count++;

        a_coder->state = DEBUGGING;
        gettimeofday(&tv, NULL);
        timing = tv.tv_usec / 1000 + tv.tv_sec * 1000;
        pthread_mutex_lock(&a_coder->config->mutex_for_printing);
        printf("%lld %d is debugging\n",timing ,a_coder->id);
        pthread_mutex_unlock(&a_coder->config->mutex_for_printing);
        usleep(a_coder->config->time_to_debug * 1000);

        a_coder->state = REFACTORING;
        gettimeofday(&tv, NULL);
        timing = tv.tv_usec / 1000 + tv.tv_sec * 1000;
        pthread_mutex_lock(&a_coder->config->mutex_for_printing);
        printf("%lld %d is refactoring\n",timing ,a_coder->id);
        pthread_mutex_unlock(&a_coder->config->mutex_for_printing);
        usleep(a_coder->config->time_to_refactor * 1000);

        // if (a_coder->compile_count >= a_coder->config->number_of_compiles_required)
        //     break;
        // int i = 0;
        // while (i < a_coder->config->number_of_coders)
        // {
        //     // if (a_coder->compile_count <= a_coder->config->number_of_compiles_required)
        //     if (a_coder->config->all_codes->)
        //     {
        //         a_coder->config->stop = 0;
        //         break;
        //     }
        //     i++;
        // }
        // if (a_coder->config->stop == 1)
        //     break;
        // else
        //     a_coder->config->stop = 1;
    }
    pthread_mutex_lock(&a_coder->config->mutex_for_printing);
    printf("%d\n", a_coder->compile_count);
    pthread_mutex_unlock(&a_coder->config->mutex_for_printing);


    return NULL;
}



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

int main(int argc, char **argv){

    // start parsing
    int i = 1;
    if (argc != 9){
        fprintf(stderr, "Error, number of argument shoulld be 8\n");
        return 1;
    }
    if (strcmp(argv[argc - 1], "fifo") && strcmp(argv[argc - 1], "edf"))
    {
        fprintf(stderr, "Error, the last output shoulld be fifo or edf\n");
        return 1;
    }
    while (i < argc - 1)
    {
        if (valid_int(argv[i]) == 0){
            fprintf(stderr, "Error: invalide input");
            return 1;
        }
        if (i == 1 && atoi(argv[i]) == 0){
            fprintf(stderr, "Error: number_of_coders must be greater than 0\n");
            return 1;
        }
        // printf("%d\n", atoi(argv[i]));
        i++;
    }


    t_config my_confg;
    my_confg.number_of_coders = atoi(argv[1]);
    my_confg.time_to_burnout = atoi(argv[2]);
    my_confg.time_to_compile = atoi(argv[3]);
    my_confg.time_to_debug = atoi(argv[4]);
    my_confg.time_to_refactor = atoi(argv[5]);
    my_confg.number_of_compiles_required = atoi(argv[6]);
    my_confg.dongle_cooldown = atoi(argv[7]);
    
    my_confg.scheduler = argv[8];

    pthread_mutex_init(&my_confg.mutex_for_stop, NULL);
    pthread_mutex_init(&my_confg.mutex_for_printing, NULL);
    my_confg.stop = 1;
    
    my_confg.all_codes = NULL;
    my_confg.all_dongles = NULL;



    // this part is creating dongles
    t_dongle *my_dongles = malloc(sizeof(t_dongle) * my_confg.number_of_coders);
    if (my_dongles == NULL)
        return 0;
    
    i = 0;
    gettimeofday(&tv, NULL);
    long long timing = tv.tv_usec / 1000 + tv.tv_sec * 1000;
    while (i < my_confg.number_of_coders)
    {
        // t_dongle a_dongle;
        pthread_mutex_init(&my_dongles[i].mutex, NULL);
        my_dongles[i].taken = 0;
        my_dongles[i].last_released = 0;
        my_dongles[i].waiter_count = 0;

        // my_dongles[i] = a_dongle;
        i++;
    }
    


    // this part is creating coders
    
    t_coder *my_coders = malloc(sizeof(t_coder) * my_confg.number_of_coders);

    if (my_coders == NULL)
    {
        free(my_dongles);
        return 0;
    }
    i = 0; 
 
    while (i < my_confg.number_of_coders)
    {
        // t_coder a_coder;
        my_coders[i].id = i;
        my_coders[i].compile_count = 0;
        my_coders[i].last_compile_start = timing;
        my_coders[i].state = WAITING;
        my_coders[i].config = &my_confg;
        if (i == 0)
            my_coders[i].left = my_dongles +( my_confg.number_of_coders - 1);
        else
            my_coders[i].left = my_dongles + (i - 1);
        my_coders[i].right = my_dongles + i;
        // my_coders[i] = a_coder;
        i++;
    }


    // end of parsing 


    // creathing threads

    pthread_t *threads = malloc(sizeof(pthread_t) * my_confg.number_of_coders);
    if (threads == NULL)
    {
        free(my_coders);
        free(my_dongles);
        return 0;
    }

    i = 0;
    while (i < my_confg.number_of_coders)
    {
        pthread_create(threads + i, NULL, coder_thread, my_coders + i);
        i++;
    }
    i = 0;
    while (i <  my_confg.number_of_coders)
    {
        pthread_join(threads[i], NULL);
        i++;
    }



    return 0;
}