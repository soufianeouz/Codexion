#include "codexion.h"
// void edf_request(t_dongle *dongle, t_coder *coder);


void fifo_request(t_dongle *dongle, t_coder *coder){

    
    if (dongle->queue[0] == NULL){
        dongle->queue[0] = coder;
    }else if (dongle->queue[1] == NULL)
    {

        dongle->queue[1] = coder;
    }
    
}

int fifo_is_first(t_dongle *dongle, t_coder *coder){


    if (dongle->queue[0] == coder)
        return 1;
    return 0;
}

void fifo_pop(t_dongle *dongle){
 

    dongle->queue[0] = dongle->queue[1];
    dongle->queue[1] = NULL;
}


void *monitor(void *arg)
{
    int i;
    t_config *my_config = (t_config *)arg;
    struct timeval tv;
    long long current_time;

    while (1)
    {
        pthread_mutex_lock(&my_config->mutex_for_stop);

        /*
         * 1. Check burnout
         */
        i = 0;
        while (i < my_config->number_of_coders)
        {
            gettimeofday(&tv, NULL);
            current_time = tv.tv_sec * 1000 + tv.tv_usec / 1000;

            if (current_time - my_config->all_codes[i].last_compile_start
                >= my_config->time_to_burnout)
            {
                pthread_mutex_lock(&my_config->mutex_for_printing);
                printf("%lld %d burned out\n",
                    current_time,
                    my_config->all_codes[i].id);
                pthread_mutex_unlock(&my_config->mutex_for_printing);

                my_config->stop = 1;

                pthread_mutex_unlock(&my_config->mutex_for_stop);
                return NULL;
            }
            i++;
        }

        /*
         * 2. Check if ALL coders reached
         *    number_of_compiles_required
         */
        my_config->stop = 1;

        i = 0;
        while (i < my_config->number_of_coders)
        {
            if (my_config->all_codes[i].compile_count
                < my_config->number_of_compiles_required)
            {
                my_config->stop = 0;
                break;
            }
            i++;
        }

        /*
         * Everybody finished.
         */
        if (my_config->stop == 1)
        {
            pthread_mutex_unlock(&my_config->mutex_for_stop);
            return NULL;
        }

        pthread_mutex_unlock(&my_config->mutex_for_stop);

        usleep(1000);
    }

    return NULL;
}




void *coder_thread(void *arg){


    t_coder *a_coder = (t_coder *)arg;
    struct timeval tv;

    long long timing;
    gettimeofday(&tv, NULL);
    a_coder->last_compile_start = tv.tv_usec / 1000 + tv.tv_sec * 1000;

    while (1)
    {
        // check of the dongles its free by timing using dongle_cooldown
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

        if (timing - a_coder->left->last_released >= a_coder->config->dongle_cooldown &&
            timing - a_coder->right->last_released >= a_coder->config->dongle_cooldown)
        {
            gettimeofday(&tv, NULL);
            timing = tv.tv_usec / 1000 + tv.tv_sec * 1000;
            pthread_mutex_lock(&a_coder->config->mutex_for_printing);
            printf("%lld %d has taken a dongle\n", timing, a_coder->id);
            pthread_mutex_unlock(&a_coder->config->mutex_for_printing);

            pthread_mutex_lock(&a_coder->config->mutex_for_printing);
            printf("%lld %d has taken a dongle\n",timing ,a_coder->id);
            pthread_mutex_unlock(&a_coder->config->mutex_for_printing);

            a_coder->state = COMPILING;
            gettimeofday(&tv, NULL);
            timing = tv.tv_usec / 1000 + tv.tv_sec * 1000;

            pthread_mutex_lock(&a_coder->config->mutex_for_stop);
            a_coder->last_compile_start = timing;
            pthread_mutex_unlock(&a_coder->config->mutex_for_stop);

            pthread_mutex_lock(&a_coder->config->mutex_for_printing);
            printf("%lld %d is compiling\n",timing ,a_coder->id);
            pthread_mutex_unlock(&a_coder->config->mutex_for_printing);
            usleep(a_coder->config->time_to_compile * 1000);

            // set the last_released to the dongle
            gettimeofday(&tv, NULL);
            timing = tv.tv_usec / 1000 + tv.tv_sec * 1000; 
            a_coder->right->last_released = timing;
            a_coder->left->last_released = timing;
            

            pthread_mutex_unlock(&a_coder->right->mutex);
            pthread_mutex_unlock(&a_coder->left->mutex);
            pthread_mutex_lock(&a_coder->config->mutex_for_stop);
            a_coder->compile_count++;
            pthread_mutex_unlock(&a_coder->config->mutex_for_stop);

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

            pthread_mutex_lock(&a_coder->config->mutex_for_stop);

            if (a_coder->config->stop == 1)
            {
                pthread_mutex_unlock(&a_coder->config->mutex_for_stop);
                break;
            }

            pthread_mutex_unlock(&a_coder->config->mutex_for_stop);
        }else
        {
            a_coder->state = WAITING;
            pthread_mutex_unlock(&a_coder->left->mutex);
            pthread_mutex_unlock(&a_coder->right->mutex);
            usleep(1000);
        }
    }


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
    my_confg.stop = 0;
    
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
        my_dongles[i].queue[0] = NULL;
        my_dongles[i].queue[1] = NULL;

        // my_dongles[i] = a_dongle;
        i++;
    }
    
    my_confg.all_dongles = my_dongles;


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

    my_confg.all_codes = my_coders;

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

    pthread_t my_monitor;

    pthread_create(&my_monitor, NULL, monitor, &my_confg);
    i = 0;
    while (i <  my_confg.number_of_coders)
    {
        pthread_join(threads[i], NULL);
        i++;
    }
    pthread_join(my_monitor, NULL);



    return 0;
}