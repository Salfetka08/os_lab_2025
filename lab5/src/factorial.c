#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

unsigned long long result = 1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* calculate_part(void* arg) {
    int* params = (int*)arg;
    int start = params[0];
    int end = params[1];
    int mod = params[2];
    
    unsigned long long partial = 1;
    for (int i = start; i <= end; i++) {
        partial = (partial * i) % mod;
    }
    
    pthread_mutex_lock(&mutex);
    result = (result * partial) % mod;
    pthread_mutex_unlock(&mutex);
    
    return NULL;
}

int main(int argc, char* argv[]) {
    int k = 10, pnum = 4, mod = 1000;
    
    // Простой парсинг аргументов
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {
            k = atoi(argv[++i]);
        } else if (strncmp(argv[i], "--pnum=", 7) == 0) {
            pnum = atoi(argv[i] + 7);
        } else if (strncmp(argv[i], "--mod=", 6) == 0) {
            mod = atoi(argv[i] + 6);
        }
    }
    
    printf("Вычисление %d! mod %d в %d потоках\n", k, mod, pnum);
    
    pthread_t threads[pnum];
    int thread_params[pnum][3];
    
    int chunk = k / pnum;
    int start = 1;
    
    for (int i = 0; i < pnum; i++) {
        int end = (i == pnum - 1) ? k : start + chunk - 1;
        
        thread_params[i][0] = start;
        thread_params[i][1] = end;
        thread_params[i][2] = mod;
        
        pthread_create(&threads[i], NULL, calculate_part, thread_params[i]);
        start = end + 1;
    }
    
    for (int i = 0; i < pnum; i++) {
        pthread_join(threads[i], NULL);
    }
    
    pthread_mutex_destroy(&mutex);
    
    printf("Результат: %llu\n", result);
    return 0;
}