#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main() {
    printf("Создаем зомби-процессы...\n");
    
    // Создаем несколько дочерних процессов
    for (int i = 0; i < 3; i++) {
        if (fork() == 0) {
            printf("Дочерний процесс %d завершается (PID: %d)\n", i, getpid());
            exit(0);  // Становимся зомби
        }
    }
    
    printf("Родительский процесс спит 30 секунд...\n");
    printf("Проверка зомби!\n");
    sleep(30);
    
    return 0;
}