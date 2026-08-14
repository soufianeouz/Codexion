#include "codexion.h"

void *coder_thread(void *arg){

    t_coder *a_coder = (t_coder *)arg;

    long long timing;

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

        // call the fifo and edf algo
        if (strcmp(a_coder->config->scheduler, "fifo") == 0)
        {
            fifo_request(a_coder->left, a_coder);
            fifo_request(a_coder->right, a_coder);
        }
        else if (strcmp(a_coder->config->scheduler, "edf") == 0)
        {
            edf_request(a_coder->left, a_coder);
            edf_request(a_coder->right, a_coder);
        }


        if (queue_is_first(a_coder->left, a_coder)
            && queue_is_first(a_coder->right, a_coder))
        {
            // my turn → take/compile
            timing = get_time_ms();

            if (timing - a_coder->left->last_released >= a_coder->config->dongle_cooldown &&
                timing - a_coder->right->last_released >= a_coder->config->dongle_cooldown)
            {
                take_dongles(a_coder);
                coder_compile(a_coder);

                // remove coder from the queue

                queue_pop(a_coder->left);
                queue_pop(a_coder->right);

                // set the last_released to the dongle

                timing = get_time_ms(); 
                a_coder->right->last_released = timing;
                a_coder->left->last_released = timing;
                

                pthread_mutex_unlock(&a_coder->right->mutex);
                pthread_mutex_unlock(&a_coder->left->mutex);
                pthread_mutex_lock(&a_coder->config->mutex_for_stop);
                a_coder->compile_count++;
                pthread_mutex_unlock(&a_coder->config->mutex_for_stop);

                debug(a_coder);
                refactor(a_coder);

                pthread_mutex_lock(&a_coder->config->mutex_for_stop);

                if (a_coder->config->stop == 1)
                {
                    pthread_mutex_unlock(&a_coder->config->mutex_for_stop);
                    break;
                }

                pthread_mutex_unlock(&a_coder->config->mutex_for_stop);
            }else
            {
                pthread_mutex_lock(&a_coder->config->mutex_for_stop);
                a_coder->state = WAITING;
                pthread_mutex_unlock(&a_coder->config->mutex_for_stop);

                pthread_mutex_unlock(&a_coder->left->mutex);
                pthread_mutex_unlock(&a_coder->right->mutex);
                usleep(1000);
            }
        }else
        {
            pthread_mutex_unlock(&a_coder->left->mutex);
            pthread_mutex_unlock(&a_coder->right->mutex);
            usleep(1000);
        }
    }
    return NULL;
}