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

static cpu_set_t mask;
static pthread_t *threads;

static void init_threads(int nr_threads);
static void wait_threads(int nr_threads);
static void cancel_threads(int nr_threads);

static unsigned long long read_energy()
{
    unsigned long long reading = 0;
    char *str = malloc(strlen(HWMON_PATH) * sizeof(char));
    if (!str)
    {
        return -1;
    }

    sprintf(str, HWMON_PATH, 1);
    FILE *file = fopen(str, "r");
    char energy_reading[50];
    fgets(energy_reading, 50, file);
    sscanf(energy_reading, "%llu", &reading);
    fclose(file);

    return reading;
}

void set_cpu_affinity(unsigned int core)
{
    CPU_ZERO(&mask);
    CPU_SET(core, &mask);
    sched_setaffinity(0, sizeof(cpu_set_t), &mask);
}

void *func(void *data)
{
    while (1)
    {
        // syscall(SYS_write, 1, "hello\n", 8);
        printf("hello, world: %ld\n", pthread_self());
    }
}

static void wait_threads(int nr_threads)
{
    for (int i = 0; i < nr_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }
}

static void init_threads(int nr_threads)
{
    threads = malloc(nr_threads * sizeof(pthread_t));

    for (int i = 0; i < nr_threads; i++)
    {
        pthread_create(&threads[i], NULL, func, NULL);
    }
}


static void cancel_threads(int nr_threads)
{
    for (int i = 0; i < nr_threads; i++)
    {
        pthread_cancel(threads[i]);
    }
}

int main(int argc, char *argv[])
{
    int pid = getpid();
    printf("pid: %d\n", pid);

    int nr_threads = atoi(argv[1]);
    int time = atoi(argv[2]);

    init_threads(nr_threads);
    unsigned long long before = read_energy();
    wait_threads(nr_threads);
    sleep(time);
    cancel_threads(nr_threads);
    unsigned long long after = read_energy();

    double result = (after - before) /* / pow(10, 6) */ * PERF_CONSTANT;
    printf("=====================C======================\n");
    printf("energy consumed: %fJ\n", result);
    printf("=====================C======================\n");
}