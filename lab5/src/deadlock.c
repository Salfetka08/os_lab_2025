#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t lockA = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockB = PTHREAD_MUTEX_INITIALIZER;

void* process1(void* arg) {
    printf("Процесс 1: беру ресурс A\n");
    pthread_mutex_lock(&lockA);
    sleep(1);  // Даем время другому процессу взять ресурс B
    
    printf("Процесс 1: жду ресурс B...\n");
    pthread_mutex_lock(&lockB);  // ДЕДЛОК!
    
    printf("Процесс 1: получил оба ресурса\n");
    pthread_mutex_unlock(&lockB);
    pthread_mutex_unlock(&lockA);
    return NULL;
}

void* process2(void* arg) {
    printf("Процесс 2: беру ресурс B\n");
    pthread_mutex_lock(&lockB);
    sleep(1);  // Даем время другому процессу взять ресурс A
    
    printf("Процесс 2: жду ресурс A...\n");
    pthread_mutex_lock(&lockA);  // ДЕДЛОК!
    
    printf("Процесс 2: получил оба ресурса\n");
    pthread_mutex_unlock(&lockA);
    pthread_mutex_unlock(&lockB);
    return NULL;
}

int main() {
    pthread_t t1, t2;
    
    printf("ДЕДЛОК \n");
    printf("Запускаю два процесса с обратным порядком блокировок...\n\n");
    
    pthread_create(&t1, NULL, process1, NULL);
    pthread_create(&t2, NULL, process2, NULL);
    
    // Ждем немного и проверяем
    sleep(3);
    printf("\nПрограмма зависла!\n");
    printf("\n");
    
    pthread_join(t1, NULL);  // Эта строка никогда не выполнится
    pthread_join(t2, NULL);
    
    return 0;
}