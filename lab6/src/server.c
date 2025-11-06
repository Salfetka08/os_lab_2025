// server.c - Улучшенная версия с поддержкой внешних подключений
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include <getopt.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include "pthread.h"
#include "mult_modulo.h"

struct FactorialArgs {
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
};

uint64_t Factorial(const struct FactorialArgs *args) {
    uint64_t ans = 1;
    
    for (uint64_t i = args->begin; i <= args->end; i++) {
        ans = MultModulo(ans, i, args->mod);
    }
    
    printf("Вычислен факториал для диапазона [%lu, %lu] mod %lu = %lu\n",
           args->begin, args->end, args->mod, ans);
    
    return ans;
}

void *ThreadFactorial(void *args) {
    struct FactorialArgs *fargs = (struct FactorialArgs *)args;
    uint64_t result = Factorial(fargs);
    return (void*)(size_t)result;
}

void PrintServerInfo(int port, const char* bind_address) {
    printf("╔══════════════════════════════════════╗\n");
    printf("║           СЕРВЕР ЗАПУЩЕН            ║\n");
    printf("╠══════════════════════════════════════╣\n");
    printf("║ Порт: %-30d ║\n", port);
    printf("║ Адрес: %-29s ║\n", bind_address);
    printf("║ Внешние подключения: ДА             ║\n");
    printf("╚══════════════════════════════════════╝\n");
}

int main(int argc, char **argv) {
    int tnum = -1;
    int port = -1;
    char bind_ip[16] = "0.0.0.0"; // По умолчанию принимаем со всех интерфейсов

    while (true) {
        static struct option options[] = {
            {"port", required_argument, 0, 0},
            {"tnum", required_argument, 0, 0},
            {"bind", required_argument, 0, 0},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);

        if (c == -1)
            break;

        switch (c) {
        case 0: {
            switch (option_index) {
            case 0:
                port = atoi(optarg);
                break;
            case 1:
                tnum = atoi(optarg);
                break;
            case 2:
                strncpy(bind_ip, optarg, sizeof(bind_ip) - 1);
                break;
            default:
                printf("Индекс %d вне диапазона опций\n", option_index);
            }
        } break;

        case '?':
            printf("Неизвестный аргумент\n");
            break;
        default:
            fprintf(stderr, "getopt вернул код символа 0%o?\n", c);
        }
    }

    if (port == -1 || tnum == -1) {
        fprintf(stderr, "Использование: %s --port 20001 --tnum 4 [--bind 0.0.0.0]\n", argv[0]);
        fprintf(stderr, "Опции bind:\n");
        fprintf(stderr, "  0.0.0.0 - все интерфейсы (внешние + локальные)\n");
        fprintf(stderr, "  127.0.0.1 - только локальные подключения\n");
        fprintf(stderr, "  ваш_ip - конкретный интерфейс\n");
        return 1;
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        fprintf(stderr, "Не удалось создать сокет сервера!\n");
        return 1;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons((uint16_t)port);
    
    if (inet_pton(AF_INET, bind_ip, &server.sin_addr) <= 0) {
        fprintf(stderr, "Неверный адрес bind: %s\n", bind_ip);
        close(server_fd);
        return 1;
    }

    int opt_val = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

    int err = bind(server_fd, (struct sockaddr *)&server, sizeof(server));
    if (err < 0) {
        fprintf(stderr, "Не удалось привязаться к сокету! Адрес: %s:%d\n", bind_ip, port);
        fprintf(stderr, "Убедитесь, что порт доступен и фаервол настроен.\n");
        close(server_fd);
        return 1;
    }

    err = listen(server_fd, 128);
    if (err < 0) {
        fprintf(stderr, "Не удалось слушать на сокете\n");
        close(server_fd);
        return 1;
    }

    PrintServerInfo(port, bind_ip);

    while (true) {
        struct sockaddr_in client;
        socklen_t client_len = sizeof(client);
        int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);

        if (client_fd < 0) {
            fprintf(stderr, "Не удалось установить новое соединение\n");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("Новое подключение от: %s:%d\n", client_ip, ntohs(client.sin_port));

        while (true) {
            unsigned int buffer_size = sizeof(uint64_t) * 3;
            char from_client[buffer_size];
            int read_bytes = recv(client_fd, from_client, buffer_size, 0);

            if (!read_bytes)
                break;
            if (read_bytes < 0) {
                fprintf(stderr, "Ошибка чтения от клиента\n");
                break;
            }
            if (read_bytes < (int)buffer_size) {
                fprintf(stderr, "Клиент отправил данные в неверном формате\n");
                break;
            }

            pthread_t threads[tnum];

            uint64_t begin = 0;
            uint64_t end = 0;
            uint64_t mod = 0;
            memcpy(&begin, from_client, sizeof(uint64_t));
            memcpy(&end, from_client + sizeof(uint64_t), sizeof(uint64_t));
            memcpy(&mod, from_client + 2 * sizeof(uint64_t), sizeof(uint64_t));

            printf("Получена задача: начало=%lu, конец=%lu, mod=%lu\n", begin, end, mod);

            struct FactorialArgs args[tnum];
            uint64_t chunk_size = (end - begin + 1) / tnum;
            uint64_t current = begin;

            for (int i = 0; i < tnum; i++) {
                args[i].begin = current;
                args[i].end = (i == tnum - 1) ? end : current + chunk_size - 1;
                args[i].mod = mod;
                current = args[i].end + 1;

                printf("Поток %d: диапазон [%lu, %lu]\n", i, args[i].begin, args[i].end);

                if (pthread_create(&threads[i], NULL, ThreadFactorial, (void *)&args[i])) {
                    fprintf(stderr, "Ошибка: pthread_create не удался!\n");
                    break;
                }
            }

            uint64_t total = 1;
            for (int i = 0; i < tnum; i++) {
                void* result_ptr;
                pthread_join(threads[i], &result_ptr);
                uint64_t result = (uint64_t)(size_t)result_ptr;
                total = MultModulo(total, result, mod);
            }

            printf("Вычисленный результат: %lu\n", total);

            char buffer[sizeof(total)];
            memcpy(buffer, &total, sizeof(total));
            err = send(client_fd, buffer, sizeof(total), 0);
            if (err < 0) {
                fprintf(stderr, "Не удалось отправить данные клиенту\n");
                break;
            }
        }

        shutdown(client_fd, SHUT_RDWR);
        close(client_fd);
        printf("Подключение закрыто\n");
    }

    return 0;
}