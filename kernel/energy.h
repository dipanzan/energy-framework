#ifndef _ENERGY_H
#define _ENERGY_H

#define DEBUG 1
#define ENERGY_TYPE 12
#define ENERGY_CONFIG 0x02
#define SCALE 2.3283064365386962890625e-10

#define NR_CPUS_PERF 16

#define LABEL_SIZE 10

typedef struct energy_accumulator
{
    u64 energy_ctr;
    u64 prev_value;
} energy_accum_t;

typedef struct perf_data
{
    struct perf_event_attr *attr;
    struct perf_event *event;
} perf_t;

typedef struct energy_data
{
    struct hwmon_channel_info energy_info;
    const struct hwmon_channel_info *info[2];
    struct hwmon_chip_info chip;
    struct task_struct *wrap_accumulate;

    /* Lock around the accumulator */
    struct mutex lock;
    /* An accumulator for each core and socket */
    energy_accum_t *accums;
    unsigned int timeout_ms;

    int energy_unit;
    int nr_cpus;
    int nr_socks;
    int nr_cpus_perf;
    int core_id;
    char (*label)[LABEL_SIZE];
    bool do_not_accum;

    /* perf kernel event data */
    struct perf_event_attr *attrs[NR_CPUS_PERF];

    /*
        do not move events elsewhere, needs to be
        at the last position due to unknown array size
        size is nr_cpus.
     */
    struct perf_event *events[NR_CPUS_PERF];

} energy_t;

static void set_energy_unit(energy_t *data);
static u64 read_msr_on_cpu(int cpu, u32 reg);

static void handle_ctr_overflow(energy_t *data, int channel, u64 value);
static void accumulate_delta_core(energy_t *data, int cpu);
static void accumulate_delta_pkg(energy_t *data, int cpu);

static void read_pkg_energy(energy_t *data);
static void reset_core_id(energy_t *data);
static void read_core_energy(energy_t *data);
static void increment_core_id(energy_t *data);
static void read_accumulate(energy_t *data);

static void add_delta_core(energy_t *data, int ch, int cpu, long *val);
static void add_delta_pkg(energy_t *data, int ch, int cpu, long *val);

static umode_t read_energy_visibility(const void *drv_data, enum hwmon_sensor_types type, u32 attr, int channel);
static int read_energy_data(struct device *dev, enum hwmon_sensor_types type, u32 attr, int channel, long *val);
static int read_energy_string(struct device *dev, enum hwmon_sensor_types type, u32 attr, int channel, const char **str);

static int energy_accumulator(void *p);

static int num_siblings_per_core(void);
static unsigned int get_core_cpu_count(void);
static unsigned int get_socket_count(void);
static int set_cpu_and_socket(struct device *dev);

static void socket_config_with_hwmon_input_and_label(energy_t *data, unsigned int *socket_config);
static void set_hwmon_channel_info(energy_t *data, unsigned int *socket_config);
static int alloc_socket_config(struct device *dev);

static int alloc_sensor_accumulator(struct device *dev);

static void set_label_l(energy_t *data, char (*label_l)[10]);
static int alloc_label_l(struct device *dev);

static int create_energy_sensor(struct device *dev);
static void set_hwmon_chip_info(energy_t *data);
static energy_t *alloc_energy_data(struct device *dev);

static void set_timeout_ms(energy_t *data);
static void set_custom_timeout_ms(energy_t *data, const int timeout_ms);

static struct device *register_hwmon_device(struct device *dev, energy_t *data);
static struct task_struct *start_energy_thread(struct device *hwmon_dev, energy_t *data);

static int energy_probe(struct platform_device *pd);
static int energy_remove(struct platform_device *pd);

static void energy_consumed_ujoules(energy_t *data, u64 value, long *val);

// TODO: perf headers
// static int alloc_perf_event_attrs(struct device *dev);
// static int alloc_perf_event_kernel_counters(struct device *dev);

#endif /* _ENERGY_H */
