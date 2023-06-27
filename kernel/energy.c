#include <asm/cpu_device_id.h>

#include <linux/init.h>
#include <linux/bits.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/hwmon.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/kprobes.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/mutex.h>
#include <linux/processor.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/topology.h>
#include <linux/types.h>
#include <linux/perf_event.h>

#include <linux/kprobes.h>

#include <linux/ftrace.h>

#include <linux/trace_events.h>

// #define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#define pr_fmt(fmt) /* KBUILD_MODNAME */ "%s(): " fmt, __func__

/* 	do not move header files above the pr_fmt format statements!
	otherwise the pr_fmt() family of functions don't work.
*/
#include "cpu-info.h"
#include "energy.h"
#include "kprobes.h"
#include "ftrace.h"
#include "lookup_funcs.h"
#include "perf.h"
#include "process.h"
#include "preempt.h"
#include "tracepoint.h"

#define DRIVER_NAME "kernel_energy_driver"
#define DRIVER_MODULE_VERSION "1.0"
#define DRIVER_MODULE_DESCRIPTION "Kernel Energy Reader with Accumulator Support"

MODULE_VERSION(DRIVER_MODULE_VERSION);

#define ENERGY_PWR_UNIT_MSR 0xC0010299
#define ENERGY_CORE_MSR 0xC001029A
#define ENERGY_PKG_MSR 0xC001029B

#define AMD_ENERGY_UNIT_MASK 0x01F00
#define AMD_ENERGY_MASK 0xFFFFFFFF

#define ACCUM_CUSTOM_TIMEOUT 1000

#define ENERGY_ACCUM_THREAD "energy_runner"

static char *name = NULL;
static int pid = -1;
static int cpu = -1;
static int mode = 0;

module_param(name, charp, 0000);
module_param(pid, int, 0000);
module_param(cpu, int, 0000);
module_param(mode, int, 0000);

MODULE_PARM_DESC(name, "[name] is the process name to attach to.");
MODULE_PARM_DESC(pid, "[pid] is the PID to to attach to.");
MODULE_PARM_DESC(cpu, "[cpu] is the target CPU for energy measurement.");
MODULE_PARM_DESC(mode, "[mode] is the backend mode for energy data source. [0]- MSR, [1]- perf");

static void set_energy_unit(energy_t *data)
{
	u64 energy_unit;

	rdmsrl_safe(ENERGY_PWR_UNIT_MSR, &energy_unit);
	data->energy_unit = (energy_unit & AMD_ENERGY_UNIT_MASK) >> 8;
}

static u64 read_msr_on_cpu(int cpu, u32 reg)
{
	u64 value;
	rdmsrl_safe_on_cpu(cpu, reg, &value);
	value &= AMD_ENERGY_MASK;
	return value;
}

static void accumulate_delta_core(energy_t *data, int cpu)
{
	mutex_lock(&data->lock);
	u64 value = read_msr_on_cpu(cpu, ENERGY_CORE_MSR);
	handle_ctr_overflow(data, cpu, value);
	mutex_unlock(&data->lock);
}

static void accumulate_delta_pkg(energy_t *data, int cpu)
{
	mutex_lock(&data->lock);
	u64 value = read_msr_on_cpu(cpu, ENERGY_PKG_MSR);
	handle_ctr_overflow(data, data->nr_cpus, value);
	mutex_unlock(&data->lock);
}

static void read_pkg_energy(energy_t *data)
{
	int socket_node = data->nr_socks - 1;
	int socket_cpu = cpumask_first_and(cpu_online_mask, cpumask_of_node(socket_node));
	accumulate_delta_pkg(data, socket_cpu);
}

static void reset_core_id(energy_t *data)
{
	if (data->core_id >= data->nr_cpus)
	{
		data->core_id = 0;
	}
}

static void read_core_energy(energy_t *data)
{
	int cpu = data->core_id;
	if (cpu_online(cpu))
	{
		accumulate_delta_core(data, cpu);
	}
}

static void increment_core_id(energy_t *data)
{
	data->core_id++;
}

static void read_accumulate(energy_t *data)
{
	read_pkg_energy(data);
	reset_core_id(data);
	read_core_energy(data);
	increment_core_id(data);
}

static void add_delta_core(energy_t *data, int channel, int cpu, long *val)
{
	struct energy_accumulator *accum;

	mutex_lock(&data->lock);
	u64 value = read_msr_on_cpu(cpu, ENERGY_CORE_MSR);

	if (!data->do_not_accum)
	{
		accum = &data->accums[channel];
		if (value >= accum->prev_value)
		{
			value += accum->energy_ctr - accum->prev_value;
		}
		else
		{
			value += UINT_MAX - accum->prev_value + accum->energy_ctr;
		}
	}
	energy_consumed_ujoules(data, value, val);
	mutex_unlock(&data->lock);
}

static void handle_ctr_overflow(energy_t *data, int channel, u64 value)
{
	struct energy_accumulator *accum = &data->accums[channel];
	if (value >= accum->prev_value)
	{
		accum->energy_ctr += value - accum->prev_value;
	}
	else
	{
		accum->energy_ctr += UINT_MAX - accum->prev_value + value;
	}
	accum->prev_value = value;
}

/* Energy consumed = (1/(2^ESU) * RAW * 1000000UL) μJoules */
static void energy_consumed_ujoules(energy_t *data, u64 value, long *val)
{
	*val = div64_ul(value * 1000000UL, BIT(data->energy_unit));
}

static void add_delta_pkg(energy_t *data, int channel, int cpu, long *val)
{
	mutex_lock(&data->lock);
	u64 value = read_msr_on_cpu(cpu, ENERGY_PKG_MSR);

	if (!data->do_not_accum)
	{
		struct energy_accumulator *accum = &data->accums[channel];
		if (value >= accum->prev_value)
		{
			value += accum->energy_ctr - accum->prev_value;
		}
		else
		{
			value += UINT_MAX - accum->prev_value + accum->energy_ctr;
		}
	}
	energy_consumed_ujoules(data, value, val);
	mutex_unlock(&data->lock);
}

// regular user/non-sudo access of counters from hwmon interface
static umode_t read_energy_visibility(const void *drv_data, enum hwmon_sensor_types type, u32 attr, int channel)
{
	return 0444;
}

static unsigned int find_sw_thread_num(unsigned int cpu)
{
	// Linux enumerates if SMT enabled aka hyperthreading, multiply by 2 to get correct hw thread for perf mode
	/*
		0, 1,  2, 3,  4, 5,  6, 7,  8, 9,  10, 11,  12, 13,  14, 15
		-----  -----  -----  -----  -----  -------	-------  -------
		  0      1      2      3	  4		  5		   6		7

		0 -> [0, 1]
		1 -> [2, 3]
		2 -> [4, 5]
		3 -> [6, 7]
		4 -> [8, 9]
		5 -> [10, 11]
		6 -> [12, 13]
		7 -> [14, 15]
	*/
	return cpu * 2;
}

static int read_perf_energy_data(struct device *dev, enum hwmon_sensor_types type, u32 attr, int channel, long *val)
{
	pr_alert("\n");
	energy_t *data = dev_get_drvdata(dev);
	unsigned int cpu;

	if (channel >= data->nr_cpus)
	{
		cpu = cpumask_first_and(cpu_online_mask, cpumask_of_node(channel - data->nr_cpus));
		add_delta_pkg(data, channel, cpu, val);
	}
	else
	{
		cpu = channel;
		if (!cpu_online(cpu))
		{
			return -ENODEV;
		}

		cpu = find_sw_thread_num(cpu);
		struct perf_event *event = data->events[cpu];
		print_perf_cpu(event);

		u64 value, enabled, running;

		rcu_read_lock();
		value = perf_event_read_value(event, &enabled, &running);
		rcu_read_unlock();

		mutex_lock(&data->lock);
		*val = value;
		mutex_unlock(&data->lock);

		// add_delta_core(data, channel, cpu, val);
	}

	return 0;
}

static int read_energy_data(struct device *dev, enum hwmon_sensor_types type, u32 attr, int channel, long *val)
{
	pr_alert("\n");
	energy_t *data = dev_get_drvdata(dev);
	int cpu;

	if (channel >= data->nr_cpus)
	{
		cpu = cpumask_first_and(cpu_online_mask, cpumask_of_node(channel - data->nr_cpus));
		add_delta_pkg(data, channel, cpu, val);
	}
	else
	{
		cpu = channel;
		if (!cpu_online(cpu))
		{
			return -ENODEV;
		}
		add_delta_core(data, channel, cpu, val);
	}

	return 0;
}

static int read_energy_string(struct device *dev, enum hwmon_sensor_types type, u32 attr, int channel, const char **str)
{
	energy_t *data = dev_get_drvdata(dev);
	*str = data->label[channel];
	return 0;
}

static const struct hwmon_ops energy_ops_msr = {
	.is_visible = read_energy_visibility,
	.read = read_energy_data,
	.read_string = read_energy_string,
};

static const struct hwmon_ops energy_ops_perf = {
	.is_visible = read_energy_visibility,
	.read = read_perf_energy_data,
	.read_string = read_energy_string,
};

static int energy_accumulator(void *p)
{
	energy_t *data = (energy_t *)p;
	unsigned int timeout_ms = data->timeout_ms;

	while (!kthread_should_stop())
	{
		read_accumulate(data);

		if (kthread_should_stop())
		{
			break;
		}

		long accum_waited = schedule_timeout_interruptible(msecs_to_jiffies(timeout_ms));
		pr_alert("timeout: %u, accum_waited: %ld\n", timeout_ms, accum_waited);
	}
	return 0;
}

static int num_siblings_per_core(void)
{
	return ((cpuid_ebx(0x8000001E) >> 8) & 0xFF) + 1;
}

static unsigned int get_core_cpu_count(void)
{
	/*
	 * Energy counter register is accessed at core level.
	 * Hence, filterout the siblings.
	 */
	return num_present_cpus() / num_siblings_per_core();
}

static unsigned int get_socket_count(void)
{
	struct cpuinfo_x86 *info = &boot_cpu_data;

	/*
	 * c->x86_max_cores is the linux count of physical cores
	 * total physical cores/ core per socket gives total number of sockets.
	 */
	return get_core_cpu_count() / info->x86_max_cores;
}

static unsigned int get_perf_cpu_count(void)
{
	// perf enumerates hw threads for SMT/hyper-threading, multiply by 2
	return get_core_cpu_count() * 2;
}

static int set_cpu_and_socket(struct device *dev)
{
	energy_t *data = dev_get_drvdata(dev);
	data->nr_cpus = get_core_cpu_count();
	data->nr_socks = get_socket_count();
	data->nr_cpus_perf = get_perf_cpu_count();
	return 0;
}

static void socket_config_with_hwmon_input_and_label(energy_t *data, unsigned int *socket_config)
{
	int i;
	for (i = 0; i < data->nr_cpus + data->nr_socks; i++)
	{
		socket_config[i] = HWMON_E_INPUT | HWMON_E_LABEL;
	}
	socket_config[i] = 0;
}

static void set_hwmon_channel_info(energy_t *data, unsigned int *socket_config)
{
	struct hwmon_channel_info *info = &data->energy_info;
	info->config = socket_config;
	info->type = hwmon_energy;
}

static int alloc_socket_config(struct device *dev)
{
	energy_t *data = dev_get_drvdata(dev);
	unsigned int *socket_config = devm_kcalloc(dev, data->nr_cpus + data->nr_socks + 1, sizeof(u32), GFP_KERNEL);
	if (!socket_config)
	{
		return -ENOMEM;
	}
	socket_config_with_hwmon_input_and_label(data, socket_config);
	set_hwmon_channel_info(data, socket_config);

	return 0;
}

static int alloc_sensor_accumulator(struct device *dev)
{
	energy_t *data = dev_get_drvdata(dev);
	struct energy_accumulator *accums = devm_kcalloc(dev, data->nr_cpus + data->nr_socks, sizeof(struct energy_accumulator), GFP_KERNEL);
	if (!accums)
	{
		return -ENOMEM;
	}
	data->accums = accums;
	return 0;
}

static void set_label_l(energy_t *data, char (*label_l)[10])
{
	data->label = label_l;

	for (int i = 0; i < data->nr_cpus + data->nr_socks; i++)
	{
		if (i < data->nr_cpus)
		{
			scnprintf(label_l[i], 10, "Ecore%03u", i);
		}
		else
		{
			scnprintf(label_l[i], 10, "Esocket%u", (i - data->nr_cpus));
		}
	}
}

static int alloc_label_l(struct device *dev)
{
	energy_t *data = dev_get_drvdata(dev);
	char(*label_l)[10];
	label_l = devm_kcalloc(dev, data->nr_cpus + data->nr_socks, sizeof(*label_l), GFP_KERNEL);
	if (!label_l)
	{
		return -ENOMEM;
	}
	set_label_l(data, label_l);
	return 0;
}

static int custom_match_dev(struct device *dev, void *data)
{
	/* this function implements the comaparison logic. Return not zero if device
	   pointed by dev is the device you are searching for.
	 */

	pr_info("init_name: %s\n", dev->init_name);
	return 0;
}

static struct device *find_device(void)
{
	struct device *parent = bus_find_device_by_name(&platform_bus_type, NULL, DRIVER_NAME);
	if (parent)
	{
		struct device *dev = device_find_child(parent, NULL, /* passed in the second param to custom_match_dev */ custom_match_dev);
		if (dev)
		{
			pr_info("DEVICE FOUND\n!");
			return dev;
		}
		put_device(dev);
	}
	pr_info("DEVICE NOT FOUND\n!");
	return NULL;
}

static int init_perf_backend(struct device *dev)
{
	int ret = 0;

	if (mode == 1)
	{
		ret |= alloc_perf_event_attrs(dev);
		ret |= alloc_perf_event_kernel_counters(dev);
		ret |= enable_perf_events(dev);
	}
	return ret;
}

static int create_energy_sensor(struct device *dev)
{
	int ret = 0;
	ret |= set_cpu_and_socket(dev);
	ret |= alloc_socket_config(dev);
	ret |= alloc_sensor_accumulator(dev);
	ret |= alloc_label_l(dev);

	ret |= init_perf_backend(dev);

	return ret;
}

static const struct x86_cpu_id amd_ryzen_cpu_ids_with_64bit_rapl_counters[] = {
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 0x17, 0x31, NULL),
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 0x19, 0x01, NULL),
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 0x19, 0x30, NULL),
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 0x19, 0x50, NULL), // bit32? double-check please
	{}};

static void set_hwmon_chip_info(energy_t *data)
{
	if (mode == 0)
	{
		pr_alert("[mode]: using default msr mode.\n");
		data->chip.ops = &energy_ops_msr;
	}
	else
	{
		pr_alert("[mode] using perf kernel mode.\n");
		data->chip.ops = &energy_ops_perf;
	}
	data->chip.info = data->info;

	/* Populate per-core energy reporting */
	data->info[0] = &data->energy_info;
}

static energy_t *alloc_energy_data(struct device *dev)
{
	energy_t *data = devm_kzalloc(dev, sizeof(energy_t), GFP_KERNEL);
	if (!data)
	{
		return NULL;
	}
	set_hwmon_chip_info(data);
	dev_set_drvdata(dev, data);

	return data;
}

static void set_timeout_ms(energy_t *data)
{
	/*
	 * On a system with peak wattage of 250W
	 * timeout = 2 ^ 32 / 2 ^ energy_unit / 250 secs
	 */
	data->timeout_ms = 1000 * BIT(min(28, 31 - data->energy_unit)) / 250;
}

static void set_custom_timeout_ms(energy_t *data, const int timeout_ms)
{
	data->timeout_ms = timeout_ms;
}

static struct device *register_hwmon_device(struct device *dev, energy_t *data)
{
	struct device *hwmon_dev = devm_hwmon_device_register_with_info(dev, DRIVER_NAME, data, &data->chip, NULL);
	return hwmon_dev;
}

static struct task_struct *start_energy_thread(struct device *hwmon_dev, energy_t *data)
{
	data->wrap_accumulate = kthread_run(energy_accumulator, data, ENERGY_ACCUM_THREAD, dev_name(hwmon_dev));
	return data->wrap_accumulate;
}

static int energy_probe(struct platform_device *pd)
{
	struct device *dev = &pd->dev;
	energy_t *data = alloc_energy_data(dev);
	if (!data)
	{
		return -ENOMEM;
	}

	int ret = create_energy_sensor(dev);
	if (ret)
	{
		return ret;
	}

	mutex_init(&data->lock);
	set_energy_unit(data);
	set_timeout_ms(data);
	// set_custom_timeout_ms(data, ACCUM_CUSTOM_TIMEOUT_MS);

	struct device *hwmon_dev = register_hwmon_device(dev, data);
	if (IS_ERR(hwmon_dev))
	{
		return PTR_ERR(hwmon_dev);
	}

	/*
	 * For AMD platforms with 64-bit RAPL MSR registers, accumulation
	 * of the energy counters are not necessary.
	 */
	if (!x86_match_cpu(amd_ryzen_cpu_ids_with_64bit_rapl_counters))
	{
		data->do_not_accum = true;
		return 0;
	}

	struct task_struct *energy_thread = start_energy_thread(dev, data);
	return PTR_ERR_OR_ZERO(energy_thread);
}

static int release_perf_counters(struct device *dev)
{
	int ret = 0;
	if (mode == 1)
	{
		ret |= disable_perf_events(dev);
		ret |= release_perf_event_kernel_counters(dev);
	}
	return ret;
}
static int energy_remove(struct platform_device *pd)
{
	int ret = 0;
	struct device *dev = &pd->dev;
	energy_t *data = dev_get_drvdata(dev);
	if (data && data->wrap_accumulate)
	{
		ret |= kthread_stop(data->wrap_accumulate);
	}

	ret |= release_perf_counters(dev);
	return ret;
}

static const struct platform_device_id energy_id_table[] = {
	{
		.name = DRIVER_NAME,
	},
	{}};

MODULE_DEVICE_TABLE(platform, energy_id_table);

static struct platform_driver energy_driver = {
	.probe = energy_probe,
	.remove = energy_remove,
	.id_table = energy_id_table,
	.driver = {
		.name = DRIVER_NAME,
	},
};

static struct platform_device *cpu_energy_pd;

static const struct x86_cpu_id amd_ryzen_cpu_ids[] __initconst = {
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 0x17, 0x31, NULL),
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 0x19, 0x01, NULL),
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 0x19, 0x10, NULL),
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 0x19, 0x30, NULL),
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 0x19, 0x50, NULL), // AMD Ryzen 5800H Cezzane (current processor)
	{}};

MODULE_DEVICE_TABLE(x86cpu, amd_ryzen_cpu_ids);

// careful: 2nd: hooked fn, 3rd: actual fn pointer to pointer!!
// static struct ftrace_hook hooked_functions[] = {
//     HOOK("sys_clone", fh_sys_clone, &real_sys_clone),
//     HOOK("__x64_sys_execveat", fh_sys_execve, &real_sys_execve),
// };

// name, real, modified
// struct ftrace_hook fh = HOOK("__x64_sys_execveat", fh_sys_execve, &real_sys_execve);
struct ftrace_hook fh = HOOK("schedule", &real_schedule, fh_schedule);

static int __init energy_init(void)
{
	int ret;

	dump_cpu_info();
	// check if boot-cpu matches with current online-cpu
	if (!x86_match_cpu(amd_ryzen_cpu_ids))
	{
		return -ENODEV;
	}

	ret = platform_driver_register(&energy_driver);
	if (ret)
	{
		return ret;
	}

	cpu_energy_pd = platform_device_alloc(DRIVER_NAME, 0);
	if (!cpu_energy_pd)
	{
		platform_driver_unregister(&energy_driver);
		return -ENOMEM;
	}

	ret = platform_device_add(cpu_energy_pd);
	if (ret)
	{
		platform_device_put(cpu_energy_pd);
		platform_driver_unregister(&energy_driver);
		return ret;
	}

	// dump_process_info(pid);

	struct task_struct *p = get_process(pid);
	// __preempt_notifier_register(&p_notifier, p);
	// preempt_notifier_register(&p_notifier);

	// init_kprobe();
	// lookup_sched_functions();
	// print_sched_functions();
	// setup_ftrace_filter();
	// register_ftrace_function(&ops);
	// fh_install_hook(&fh);

	struct dynevent_cmd cmd;
	char *buf;

	/* Create a buffer to hold the generated command */
	// buf = kzalloc(MAX_DYNEVENT_CMD_LEN, GFP_KERNEL);

	// /* Before generating the command, initialize the cmd object */
	// synth_event_cmd_init(&cmd, buf, MAX_DYNEVENT_CMD_LEN);

	// ret = synth_event_create("schedtest", sched_fields, ARRAY_SIZE(sched_fields), THIS_MODULE);

	// ret = synth_event_gen_cmd_start(&cmd, "schedtest", THIS_MODULE,
	// 								"pid_t", "next_pid_field",
	// 								"u64", "ts_ns");

	// synth_event_trace();


	pr_alert("energy module loaded!\n");
	return ret;
}

static void __exit energy_exit(void)
{
	platform_device_unregister(cpu_energy_pd);
	platform_driver_unregister(&energy_driver);

	// unregister_ftrace_function(&ops);

	// fh_remove_hook(&fh);

	// release_kprobe();

	// __preempt_notifier_unregister(&p_notifier);
	// preempt_notifier_unregister(&p_notifier);

	pr_alert("energy module unloaded\n");
}

module_init(energy_init);
module_exit(energy_exit);

MODULE_DESCRIPTION("Non-sudo RAPL MSR + perf reading module via hwmon interface");
MODULE_AUTHOR("Dipanzan Islam <dipanzan@live.com>");
MODULE_LICENSE("GPL");