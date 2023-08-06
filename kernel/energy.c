#include <asm/cpu_device_id.h>

#include <linux/init.h>
#include <linux/bits.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/ftrace.h>
#include <linux/hwmon.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/kprobes.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/mutex.h>
#include <linux/perf_event.h>
#include <linux/processor.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/topology.h>
#include <linux/types.h>

#include <linux/trace_events.h>

/*
 * Must include the event header that the custom event will attach to,
 * from the C file, and not in the custom header file.
 */
#include <trace/events/sched.h>

/* Declare CREATE_CUSTOM_TRACE_EVENTS before including custom header */
// #define CREATE_CUSTOM_TRACE_EVENTS

// #include "trace_custom_sched.h"

// #define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#define pr_fmt(fmt) /* KBUILD_MODNAME */ "%s(): " fmt, __func__

/* 	do not move header files above the pr_fmt format statements!
	otherwise the pr_fmt() family of functions don't work.
*/

// WARNING - multiple threads will access this pointer!!!
// FOR NOW DO NOT MOVE THIS UNDER #include directives, this is needed by
// preempt.h to access struct device *dev from platform_device by sched_in/out functions
static struct platform_device *cpu_energy_pd;

#include "constants.h"
#include "cpu-info.h"
#include "energy.h"
#include "kprobes.h"
#include "ftrace.h"
#include "lookup_funcs.h"
#include "perf.h"
#include "process.h"
#include "preempt.h"

#define DRIVER_NAME "kernel_energy_driver"
#define DRIVER_MODULE_VERSION "1.0"
#define DRIVER_MODULE_DESCRIPTION "Kernel Energy Reader with Accumulator Support"

MODULE_VERSION(DRIVER_MODULE_VERSION);

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
	energy_accum_t *accum = &data->accums[channel];
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

// HELLO MARKER
static int read_perf_energy_data(struct device *dev, enum hwmon_sensor_types type, u32 attr, int channel, long *val)
{

	struct task_struct *p = current;
	pr_alert("tgid: %d, pid: %d, comm: %s, thread_info CPU: %d\n", p->tgid, p->pid, p->comm, p->thread_info.cpu);
	// lock_process_on_cpu(p->pid, p->thread_info.cpu);

	// find_threads(p);
	init_preempt_notifiers(dev, p);
	release_preempt_notifiers(dev, p);

	energy_t *data = dev_get_drvdata(dev);
	unsigned int cpu;

	cpu = channel;
	if (!cpu_online(cpu))
	{
		return -ENODEV;
	}

	struct perf_event *event = data->perf[channel].event;
	u64 value, enabled, running;
	value = perf_event_read_value(event, &enabled, &running);

	// pr_alert("%s(): CPU: %d, cpu: %d, value: %ld, enabled: %ld, running: %ld\n", __FUNCTION__, channel, event->cpu, value, enabled, running);
	mutex_lock(&data->lock);
	*val = value;
	mutex_unlock(&data->lock);

	return 0;
}

static int read_perf_energy_data2(struct device *dev, enum hwmon_sensor_types type, u32 attr, int channel, long *val)
{
	rcu_read_lock();
	struct task_struct *p = current;
	// lock_process_on_cpu(p->pid, p->thread_info.cpu);
	// __preempt_notifier_register(p);d
	pr_alert("pid: %d, comm: %s, thread_info CPU: %d\n", p->pid, p->comm, p->thread_info.cpu);

	find_threads(p);
	// __preempt_notifier_unregister(p);
	rcu_read_unlock();

	energy_t *data = dev_get_drvdata(dev);
	unsigned int cpu;

	cpu = channel;
	if (!cpu_online(cpu))
	{
		return -ENODEV;
	}

	struct perf_event *event = data->perf[channel].event;

	u64 value, enabled, running;

	rcu_read_lock();
	value = perf_event_read_value(event, &enabled, &running);
	rcu_read_unlock();

	// pr_alert("%s(): CPU: %d, cpu: %d, value: %ld, enabled: %ld, running: %ld\n", __FUNCTION__, channel, event->cpu, value, enabled, running);
	mutex_lock(&data->lock);
	*val = value;
	mutex_unlock(&data->lock);

	return 0;
}

static int read_energy_data(struct device *dev, enum hwmon_sensor_types type, u32 attr, int channel, long *val)
{
	energy_t *data = dev_get_drvdata(dev);
	int cpu;

	if (channel >= data->nr_cpus)
	{
#if DEBUG
		pr_alert("%s(): SOCKET: %d\n", __FUNCTION__, channel);
#endif

		cpu = cpumask_first_and(cpu_online_mask, cpumask_of_node(channel - data->nr_cpus));
		add_delta_pkg(data, channel, cpu, val);
	}
	else
	{
#if DEBUG
		pr_alert("%s(): CPU: %d\n", __FUNCTION__, channel);
#endif

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

static int alloc_cpu_socket(struct device *dev)
{
	energy_t *data = dev_get_drvdata(dev);
	data->nr_socks = get_socket_count();
	return 0;
}

static int alloc_cpu_cores(struct device *dev)
{
	energy_t *data = dev_get_drvdata(dev);
	data->nr_cpus = get_core_cpu_count() * 2;
	return 0;
}

static void set_socket_config_hwmon(energy_t *data, unsigned int *socket_config)
{
	int cpu;
	for (cpu = 0; cpu < data->nr_cpus + data->nr_socks; cpu++)
	{
		socket_config[cpu] = HWMON_E_INPUT | HWMON_E_LABEL;
	}
	socket_config[cpu] = 0;
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
	set_socket_config_hwmon(data, socket_config);
	set_hwmon_channel_info(data, socket_config);

	return 0;
}

static int alloc_sensor_accumulator(struct device *dev)
{
	energy_t *data = dev_get_drvdata(dev);
	energy_accum_t *accums = devm_kcalloc(dev, data->nr_cpus + data->nr_socks, sizeof(energy_accum_t), GFP_KERNEL);
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

static unsigned int init_cpu_cores(void)
{
	unsigned int cores = 0;
	if (mode == 1)
	{
		cores = get_perf_cpu_count();
	}
	else
	{
		cores = get_core_cpu_count();
	}
	return cores;
}

static int init_perf_backend(struct device *dev)
{
	int ret = 0;
	if (mode == 1)
	{
		ret |= perf_alloc_cpu_cores(dev); // CPU allocation must be done 1st, DO NOT CHANGE ORDER!
		ret |= perf_alloc(dev);
		ret |= perf_alloc_socket_config(dev);
		ret |= perf_alloc_sensor_accumulator(dev);
		ret |= perf_alloc_label_l(dev);

		ret |= alloc_perf_event_attrs(dev);
		ret |= alloc_perf_event_kernel_counters(dev);
		ret |= alloc_perf_energy_values(dev);
		ret |= enable_perf_events(dev);
	}
	return ret;
}

static int init_msr_backend(struct device *dev)
{
	int ret = 0;

	if (mode == 0)
	{
		ret |= alloc_cpu_cores(dev);
		ret |= alloc_socket_config(dev);
		ret |= alloc_sensor_accumulator(dev);
		ret |= alloc_label_l(dev);
	}
	return ret;
}

static int alloc_energy_sensor(struct device *dev)
{
	int ret = 0;

	ret |= alloc_cpu_socket(dev);
	ret |= init_msr_backend(dev);
	ret |= init_perf_backend(dev);
	ret |= init_kprobe(dev);

	// preempt support WIP
	// ret |= alloc_preempt_notifiers(dev);
	// ret |= init_preempt_callbacks(dev);

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
		pr_alert("[mode]: using perf kernel mode.\n");
		data->chip.ops = &energy_ops_perf;
	}
	data->chip.info = data->info;

	/* Populate per-core energy reporting */
	data->info[0] = &data->energy_info;
}

static int alloc_energy_data(struct device *dev)
{
	energy_t *data = devm_kzalloc(dev, sizeof(energy_t), GFP_KERNEL);
	if (!data)
	{
		pr_alert("%s(): failed!\n", __FUNCTION__);
		return -ENOMEM;
	}
	set_hwmon_chip_info(data);
	dev_set_drvdata(dev, data);
	return 0;
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

	int ret;
	ret = alloc_energy_data(dev);
	if (ret)
	{
		return ret;
	}

	ret = alloc_energy_sensor(dev);
	if (ret)
	{
		return ret;
	}

	energy_t *data = dev_get_drvdata(dev);
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

static int energy_remove(struct platform_device *pd)
{
	int ret = 0;
	struct device *dev = &pd->dev;
	energy_t *data = dev_get_drvdata(dev);
	if (data && data->wrap_accumulate)
	{
		ret |= kthread_stop(data->wrap_accumulate);
	}

	// if (data && data->preempt_runner)
	// {
	// 	ret |= kthread_stop(data->preempt_runner);
	// }

	ret |= release_kprobe();
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

static const struct x86_cpu_id amd_ryzen_cpu_ids[] __initconst = {
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 0x17, 0x31, NULL),
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 0x19, 0x01, NULL),
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 0x19, 0x10, NULL),
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 0x19, 0x30, NULL),
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 0x19, 0x50, NULL), // AMD Ryzen 5800H Cezzane (current processor)
	{}};

MODULE_DEVICE_TABLE(x86cpu, amd_ryzen_cpu_ids);

static int __init energy_init(void)
{

	dump_cpu_info();
	// check if boot-cpu matches with current online-cpu
	if (!x86_match_cpu(amd_ryzen_cpu_ids))
	{
		return -ENODEV;
	}

	int ret;
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

	// ret = init_kprobe();
	// if (ret)
	// {
	// 	release_kprobe();
	// 	return ret;
	// }

	lookup_vars();
	lookup_functions();

	// __EXPERIMENT_enable_perf_sched();
	// fh_install_hook(&fh);

	pr_alert("energy module loaded!\n");
	pr_alert("[WARNING]: YOU ARE IN KERNEL MODE - PREEMPTION DISABLED YOU HAVE BEEN WARNED! :#\n");
	return ret;
}

static void __exit energy_exit(void)
{
	platform_device_unregister(cpu_energy_pd);
	platform_driver_unregister(&energy_driver);

	// __EXPERIMENT_disable_perf_sched();
	release_kprobe();

	// fh_remove_hook(&fh);
	pr_alert("energy module unloaded\n");
	pr_alert("[WARNING]: EXITING KERNEL MODE - PREEMPTION ENABLED, SWITCHING TO USER MODE! :)\n");
}

module_init(energy_init);
module_exit(energy_exit);

MODULE_DESCRIPTION("Non-sudo RAPL MSR + perf reading module via hwmon interface");
MODULE_AUTHOR("Dipanzan Islam <dipanzan@live.com>");
MODULE_LICENSE("GPL");