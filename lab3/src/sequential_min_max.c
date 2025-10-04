#include <stdio.h>
#include <stdlib.h>
#include "find_min_max.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <size> [numbers...]\n", argv[0]);
        return -1;
    }
    int size = atoi(argv[1]);
    int *array = malloc(sizeof(int) * size);
    for (int i = 0; i < size && i + 2 < argc; i++) {
        array[i] = atoi(argv[i + 2]);
    }
    struct MinMax result = GetMinMax(array, 0, size - 1);
    printf("Min: %d, Max: %d\n", result.min, result.max);
    free(array);
    return 0;
}