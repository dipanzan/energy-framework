#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <math.h>
#include <pthread.h>

#define PERF_CONSTANT 2.3283064365386962890625e-10
#define ENERGY_CONSTANT 100
#define HWMON_PATH "/sys/class/hwmon/hwmon5/energy%i_input"

static unsigned long long read_energy(int core)
{
    core = core + 1;
    unsigned long long reading = 0;
    char *str = malloc(strlen(HWMON_PATH) * sizeof(char));
    if (!str)
    {
        return -1;
    }

    sprintf(str, HWMON_PATH, core);
    FILE *file = fopen(str, "r");
    char energy_reading[50];
    fgets(energy_reading, 50, file);
    sscanf(energy_reading, "%llu", &reading);
    fclose(file);

    return reading;
}

static int load(int n)
{

    for (int i = 0; i < n; i++)
    {
        // syscall(SYS_write, 1, "hello\n", 8);
        printf("hello\n");
    }
}

cpu_set_t mask;

void set_cpu_affinity(unsigned int core)
{
    CPU_ZERO(&mask);
    CPU_SET(core, &mask);
    sched_setaffinity(0, sizeof(cpu_set_t), &mask);
}

void *func(void *args)
{
    sleep(5);
}
#define NUM_THREADS 5

int main(int argc, char *argv[])
{
    int pid = getpid();
    printf("pid: %d\n", pid);
    int core = atoi(argv[1]);
    int n = atoi(argv[2]);

    // set_cpu_affinity(core);

    pthread_t threads[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_create(&threads[i], NULL, func, NULL);
    }

    

    unsigned long long before = read_energy(core);

    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }
    load(n);
    unsigned long long after = read_energy(core);

    double result = (after - before) /* / pow(10, 6) */ * PERF_CONSTANT;
    printf("=====================C======================\n");
    printf("Core: %i energy consumed: %fJ\n", core, result);
    printf("=====================C======================\n");
}