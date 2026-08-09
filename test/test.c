#include <pthread.h>
#include <stdio.h>
#include "codexion.h"

// int counter = 0; // shared variable
// pthread_mutex_t mutex;

// void *increment(void *arg)
// {
//     int i = 0;

//     while (i < 1000000)
//     {
//         pthread_mutex_lock(&mutex);
//         counter++;
//         pthread_mutex_unlock(&mutex);
//         i++;
//     }

//     return NULL;
// }

// int main(void)
// {
    // pthread_t threads[2];
    // pthread_mutex_init(&mutex, NULL);


    // pthread_create(&threads[0], NULL, increment, NULL);
    // pthread_create(&threads[1], NULL, increment, NULL);

    // pthread_join(threads[0], NULL);
    // pthread_join(threads[1], NULL);

    // printf("counter = %d\n", counter);

    // long long timing = tv.tv_usec / 1000 + tv.tv_sec * 1000;
    // gettimeofday(&tv, NULL);
    // struct timeval tv;
    // printf("%ld\n", tv.tv_usec);
    // printf("%ld\n", tv.tv_sec);

    #include <stdio.h>
    #include <sys/time.h>

int main(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);

    printf("tv_sec  = %ld\n", tv.tv_sec);
    printf("tv_usec = %ld\n", tv.tv_usec);

    return (0);
}

//     return 0;
// }