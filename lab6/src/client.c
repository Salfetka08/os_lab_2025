// client.c - Улучшенная версия с лучшей обработкой ошибок
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "mult_modulo.h"

struct Server {
    char ip[255];
    int port;
};

struct ThreadArgs {
    struct Server server;
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
    uint64_t result;
    bool success;
};

bool ConvertStringToUI64(const char *str, uint64_t *val) {
    char *end = NULL;
    unsigned long long i = strtoull(str, &end, 10);
    if (errno == ERANGE) {
        fprintf(stderr, "Выход за пределы диапазона uint64_t: %s\n", str);
        return false;
    }
    if (errno != 0)
        return false;
    *val = i;
    return true;
}

void* ServerThread(void* args) {
    struct ThreadArgs* thread_args = (struct ThreadArgs*)args;
    thread_args->success = false;
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(thread_args->server.port);
    
    if (inet_pton(AF_INET, thread_args->server.ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Неверный адрес: %s\n", thread_args->server.ip);
        thread_args->result = 0;
        return NULL;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        fprintf(stderr, "Ошибка создания сокета!\n");
        thread_args->result = 0;
        return NULL;
    }

    // Установка таймаута на подключение (5 секунд)
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    printf("Подключаюсь к серверу %s:%d...\n", thread_args->server.ip, thread_args->server.port);
    
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "Ошибка подключения к %s:%d\n", 
                thread_args->server.ip, thread_args->server.port);
        close(sock);
        thread_args->result = 0;
        return NULL;
    }

    printf("Успешно подключен к %s:%d\n", thread_args->server.ip, thread_args->server.port);

    char task[sizeof(uint64_t) * 3];
    memcpy(task, &thread_args->begin, sizeof(uint64_t));
    memcpy(task + sizeof(uint64_t), &thread_args->end, sizeof(uint64_t));
    memcpy(task + 2 * sizeof(uint64_t), &thread_args->mod, sizeof(uint64_t));

    if (send(sock, task, sizeof(task), 0) < 0) {
        fprintf(stderr, "Ошибка отправки на %s:%d\n", 
                thread_args->server.ip, thread_args->server.port);
        close(sock);
        thread_args->result = 0;
        return NULL;
    }

    char response[sizeof(uint64_t)];
    if (recv(sock, response, sizeof(response), 0) < 0) {
        fprintf(stderr, "Ошибка получения от %s:%d\n", 
                thread_args->server.ip, thread_args->server.port);
        close(sock);
        thread_args->result = 0;
        return NULL;
    }

    memcpy(&thread_args->result, response, sizeof(uint64_t));
    thread_args->success = true;
    
    printf("✅ Сервер %s:%d вернул: %lu для диапазона [%lu, %lu]\n",
           thread_args->server.ip, thread_args->server.port,
           thread_args->result, thread_args->begin, thread_args->end);

    close(sock);
    return NULL;
}

int ReadServers(const char* filename, struct Server** servers) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Ошибка открытия файла серверов");
        return -1;
    }

    int capacity = 10;
    int count = 0;
    *servers = malloc(sizeof(struct Server) * capacity);

    char line[255];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        
        if (strlen(line) == 0) continue; // Пропускаем пустые строки
        
        char* colon = strchr(line, ':');
        if (!colon) {
            fprintf(stderr, "Неверный формат сервера: %s\n", line);
            continue;
        }
        
        *colon = '\0';
        int port = atoi(colon + 1);
        
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Неверный порт в строке: %s\n", line);
            continue;
        }
        
        if (count >= capacity) {
            capacity *= 2;
            *servers = realloc(*servers, sizeof(struct Server) * capacity);
        }
        
        strncpy((*servers)[count].ip, line, sizeof((*servers)[count].ip) - 1);
        (*servers)[count].port = port;
        count++;
    }

    fclose(file);
    return count;
}

void PrintClientInfo(uint64_t k, uint64_t mod, const char* servers_file) {
    printf("╔══════════════════════════════════════╗\n");
    printf("║           КЛИЕНТ ЗАПУЩЕН            ║\n");
    printf("╠══════════════════════════════════════╣\n");
    printf("║ k (факториал): %-21lu ║\n", k);
    printf("║ Модуль: %-28lu ║\n", mod);
    printf("║ Файл серверов: %-22s ║\n", servers_file);
    printf("╚══════════════════════════════════════╝\n");
}

int main(int argc, char **argv) {
    uint64_t k = 0;
    uint64_t mod = 0;
    char servers_file[255] = {'\0'};

    while (true) {
        static struct option options[] = {
            {"k", required_argument, 0, 0},
            {"mod", required_argument, 0, 0},
            {"servers", required_argument, 0, 0},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);

        if (c == -1) break;

        switch (c) {
        case 0: {
            switch (option_index) {
            case 0:
                ConvertStringToUI64(optarg, &k);
                break;
            case 1:
                ConvertStringToUI64(optarg, &mod);
                break;
            case 2:
                strncpy(servers_file, optarg, sizeof(servers_file) - 1);
                break;
            default:
                printf("Индекс %d вне диапазона опций\n", option_index);
            }
        } break;

        case '?':
            printf("Ошибка аргументов\n");
            break;
        default:
            fprintf(stderr, "getopt вернул код символа 0%o?\n", c);
        }
    }

    if (k == 0 || mod == 0 || !strlen(servers_file)) {
        fprintf(stderr, "Использование: %s --k 1000 --mod 5 --servers /путь/к/файлу\n",
                argv[0]);
        return 1;
    }

    struct Server* servers = NULL;
    int servers_num = ReadServers(servers_file, &servers);
    if (servers_num <= 0) {
        fprintf(stderr, "Не найдено валидных серверов в файле\n");
        return 1;
    }

    PrintClientInfo(k, mod, servers_file);
    printf("Найдено серверов: %d\n\n", servers_num);

    pthread_t threads[servers_num];
    struct ThreadArgs thread_args[servers_num];

    uint64_t numbers_per_server = k / servers_num;
    uint64_t current_start = 1;

    for (int i = 0; i < servers_num; i++) {
        thread_args[i].server = servers[i];
        thread_args[i].begin = current_start;
        thread_args[i].end = (i == servers_num - 1) ? k : current_start + numbers_per_server - 1;
        thread_args[i].mod = mod;
        thread_args[i].result = 1;
        thread_args[i].success = false;

        printf("Сервер %d (%s:%d): числа от %lu до %lu\n", 
               i, thread_args[i].server.ip, thread_args[i].server.port,
               thread_args[i].begin, thread_args[i].end);

        current_start = thread_args[i].end + 1;

        if (pthread_create(&threads[i], NULL, ServerThread, &thread_args[i])) {
            fprintf(stderr, "Ошибка создания потока для сервера %d\n", i);
            continue;
        }
    }

    printf("\n⏳ Ожидание результатов от серверов...\n\n");
    
    uint64_t total_result = 1;
    int successful_servers = 0;
    
    for (int i = 0; i < servers_num; i++) {
        pthread_join(threads[i], NULL);
        if (thread_args[i].success && thread_args[i].result != 0) {
            total_result = MultModulo(total_result, thread_args[i].result, mod);
            successful_servers++;
        } else {
            printf("❌ Сервер %d (%s:%d) не вернул результат\n", 
                   i, thread_args[i].server.ip, thread_args[i].server.port);
        }
    }

    printf("\n══════════════════════════════════════\n");
    printf("Успешных серверов: %d из %d\n", successful_servers, servers_num);
    printf("Финальный результат: %lu! mod %lu = %lu\n", k, mod, total_result);
    printf("══════════════════════════════════════\n");

    free(servers);
    return 0;
}