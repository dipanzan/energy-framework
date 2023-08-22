#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SIZE 1024

void init_srand()
{
    time_t t;
    srand((unsigned)time(&t));
}

int random_num(int min, int max)
{
    return (rand() % (max + 1)) + 1;
}

int *init_random_matrix(int size)
{
    printf("Populating matrix with random values [1-999].\n");

    int *matrix = malloc(sizeof(int[size][size]));

    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            matrix[i][j] = random_num(1, 999);
        }
    }
    printf("Matrix initialization done.\n");
    return matrix;
}

int **init_matrix(int size)
{
    return malloc(sizeof(int[size][size]));
}

void multiply_matrix(int size, int **a, int **b, int **c)
{
    printf("Multiplying matrix.\n");

    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            for (int k = 0; k < size; k++)
            {
                c[i][j] += (long)(a[i][k] * b[k][j]);
            }
        }
    }
    printf("Multiplication done!\n");
}

int main(int argc, char *argv[])
{
    int size = atoi(argv[1]);
    init_srand();

    int *a = init_random_matrix(size);
    // int **b = init_random_matrix(size);
    // int **c = init_matrix(size);

        // init_matrix(size, b);

        // multiply_matrix(size, a, b);

        return 0;
}