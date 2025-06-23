#include <stdlib.h>
#include <pthread.h>
#include <sched.h>

typedef struct
{
    int order;
    char *message;
} work_args_t;

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c = PTHREAD_COND_INITIALIZER;

int order = 1;

void *work(void *a)
{
    work_args_t *args = (work_args_t *)a;

    pthread_mutex_lock(&m);

    while (args->order != order)
    {
        pthread_cond_wait(&c, &m);
    }

    // Simulate some work

    puts(args->message);
    order++;
    pthread_cond_broadcast(&c);
    pthread_mutex_unlock(&m);

    return NULL;
}

int main()
{

    work_args_t args[5] = {{3, "Thread 1"}, {1, "Thread 2"}, {2, "Thread 3"}, {4, "Thread 4"}, {5, "Main thread"}};

    for (int i = 0; i < 4; i++)
    {
        pthread_t t;
        pthread_create(&t, NULL, work, (void *)&(args[i]));
    }

    work((void *)&(args[4]));

    exit(0);
}
