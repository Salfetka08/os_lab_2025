#include <stdlib.h>
#include "utils.h"

void GenerateArray(int *array, unsigned int array_size, unsigned int seed) {
    srand(seed);
    for (unsigned int i = 0; i < array_size; i++) {
        array[i] = rand() % 100;  // Генерируем числа от 0 до 99
    }
}