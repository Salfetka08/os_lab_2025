#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        return 1;
    }
    if (pid == 0) {
        // Дочерний процесс
        execl("./sequential_min_max", "sequential_min_max", "5", "10", "5", "8", "12", "3", NULL);
        perror("Exec failed");
        return 1;
    } else {
        // Родительский процесс
        int status;
        wait(&status);
        if (WIFEXITED(status)) {
            printf("Child process exited with status %d\n", WEXITSTATUS(status));
        }
    }
    return 0;
}