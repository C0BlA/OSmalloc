#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>
#include <semaphore.h>
/*TODO: add global variables and synchronization variables */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t sem;

char * keyword = NULL ;

int is_text_file (char * filename)
{
        if (strlen(filename) < 4)
                return 0 ;

        if (strcmp(".txt", filename + strlen(filename) - 4) != 0)
                return 0 ;
        return 1 ;
}

void rtrim (char * s)
{
        size_t l = strlen(s) ;
        if (l == 0)
                return ;
        char * p = s + strlen(s) - 1 ;
        while (s != p && isspace(*p)) {
                *p = '\0' ;
                p-- ;
        }
}

void * search_one_file (void * a)
{
        sem_wait(&sem) ;

        char * filename = (char *) a ;
        FILE * fp = NULL ;

        if (!(fp = fopen(filename, "r"))){
            sem_post(&sem) ;
            free(filename);
            fprintf(stderr, "Error opening file: %s\n", filename);
            return NULL ;
        }
                

        int linenum = 1 ;
        char * buf = malloc(sizeof(char) * 2048) ;
        size_t buf_size = 2048 ;
        while (!feof(fp)) {
                ssize_t l = getline(&buf, &buf_size, fp) ;
                rtrim(buf) ;

                if (l < 0)
                        break ;
                if (strstr(buf, keyword)) {
                        pthread_mutex_lock(&mutex);
                        printf("%s:%d  %s\n", filename, linenum, buf) ;
                        pthread_mutex_unlock(&mutex);
                }

                linenum++ ;
        }
        if (buf != NULL)
                free(buf) ;

        if (fp != NULL)
                fclose(fp) ;
        sem_post(&sem) ;
        free(filename);
        return NULL ;
}

int main (int argc, char ** args)
{
        if (argc != 2)
                exit(1) ;

        keyword = args[1] ;

        sem_init(&sem, 0, 8) ;
        int thread_count = 0 ;
        int thread_capacity = 8 ;
        pthread_t *threads = malloc(sizeof(pthread_t) * thread_capacity) ;

        DIR * dir;
        if (dir = opendir(".")) {
                struct dirent *dent;
                while ((dent = readdir(dir))!=NULL) {
                        if (is_text_file(dent->d_name)) {
                                if (thread_count >= thread_capacity) {
                                        thread_capacity *= 2 ;
                                        threads = realloc(threads, sizeof(pthread_t) * thread_capacity) ;
                                        // printf("remalloc! capacity size is %d\n", thread_capacity) ;
                                }
                                char *filename = strdup(dent->d_name);
                                pthread_create(&threads[thread_count++], NULL, search_one_file, filename);
                        }
                }
                closedir(dir);
        }

        for(int i = 0; i < thread_count; i++) {
                pthread_join(threads[i], NULL);
        }

        free(threads);
        sem_destroy(&sem);
        pthread_mutex_destroy(&mutex);




        exit(0);
}

