#ifndef _PERF_H
#define _PERF_H

#define DEAD "PERF_EVENT_STATE_DEAD [-4]\n"
#define EXIT "PERF_EVENT_STATE_EXIT [-3]\n"
#define ERROR "PERF_EVENT_STATE_ERROR [-2]\n"
#define OFF "PERF_EVENT_STATE_OFF [-1]\n"
#define INACTIVE "PERF_EVENT_STATE_INACTIVE [0]\n"
#define ACTIVE "PERF_EVENT_STATE_ACTIVE [1]\n"

struct perf_event_attr energy_attr0 = {
	.type = ENERGY_TYPE,
	.config = ENERGY_CONFIG,
	.exclude_user = 0,
	.exclude_kernel = 0,
	.pinned = 1,
	.inherit = 1,
	.comm = 1,
	.sample_type = PERF_SAMPLE_IDENTIFIER,
	.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING | PERF_FORMAT_ID,
	.disabled = 1,
	.aux_output = 0,
};

struct perf_event_attr energy_attr1 = {
	.type = ENERGY_TYPE,
	.config = ENERGY_CONFIG,
	.exclude_user = 0,
	.exclude_kernel = 0,
	.pinned = 1,
	.inherit = 1,
	.comm = 1,
	.sample_type = PERF_SAMPLE_IDENTIFIER,
	.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING | PERF_FORMAT_ID,
	.disabled = 1,
	.aux_output = 0,
};

static void print_perf_event_state(struct perf_event *event)
{
#if DEBUG
	if (event->state == PERF_EVENT_STATE_DEAD)
	{
		pr_info(DEAD);
	}
	else if (event->state == PERF_EVENT_STATE_EXIT)
	{
		pr_info(EXIT);
	}
	else if (event->state == PERF_EVENT_STATE_ERROR)
	{
		pr_info(ERROR);
	}
	else if (event->state == PERF_EVENT_STATE_OFF)
	{
		pr_info(OFF);
	}
	else if (event->state == PERF_EVENT_STATE_INACTIVE)
	{
		pr_info(INACTIVE);
	}
	else
	{
		pr_info(ACTIVE);
	}
#endif
}

static void config_perf_event_energy_attr(struct perf_event_attr *attr)
{
	attr->type = ENERGY_TYPE;
	attr->config = ENERGY_CONFIG;
	attr->exclude_user = 0;
	attr->exclude_kernel = 0;
	attr->pinned = 1;
	attr->inherit = 1;
	attr->comm = 1;
	attr->sample_type = PERF_SAMPLE_IDENTIFIER;
	attr->read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
	attr->disabled = 1;
	attr->aux_output = 0;
}

static int alloc_perf_event_attrs(struct device *dev)
{
	energy_t *data = dev_get_drvdata(dev);
	for (unsigned int cpu = 0; cpu < data->nr_cpus_perf; cpu++)
	{
		struct perf_event_attr *attr = devm_kcalloc(dev, 1, sizeof(struct perf_event_attr), GFP_KERNEL);
		if (!attr)
		{
			return -ENOMEM;
		}
		config_perf_event_energy_attr(attr);
		data->attrs[cpu] = attr;
	}
	return 0;
}

static void perf_overflow_handler(struct perf_event *event, struct perf_sample_data *data, struct pt_regs *regs)
{
	u64 enabled, running, counter;
	counter = perf_event_read_value(event, &enabled, &running);
	pr_alert("counter overflow: %llu\n", counter);
}

static void print_perf_cpu(struct perf_event *event)
{
	int cpu = event->cpu;
	int oncpu = event->oncpu;
	pr_alert("cpu: %d, oncpu: %d\n", cpu, oncpu);
}

static int alloc_perf_event_kernel_counters(struct device *dev)
{
	energy_t *data = dev_get_drvdata(dev);

	for (int cpu = 0; cpu < data->nr_cpus_perf; cpu++)
	{
		struct perf_event_attr *attr = data->attrs[cpu];
		pr_alert("foorloop cpu: %d\n", cpu);
		struct perf_event *event = perf_event_create_kernel_counter(attr, cpu, NULL, perf_overflow_handler, NULL);
		
		if (IS_ERR(event))
		{
			pr_alert("perf_event_create failed on CPU[%d] with error: %ld\n", cpu, PTR_ERR(event));
			return PTR_ERR(event);
		}
		print_perf_cpu(event);
		data->events[cpu] = event;
	}
	return 0;
}

static void print_perf_pmu(struct perf_event *event, unsigned int cpu)
{
	const char *name = event->pmu->name;
	pr_info("CPU[%d]: pmu: %s\n", cpu, name);
}

static int enable_perf_events(struct device *dev)
{
	energy_t *data = dev_get_drvdata(dev);
	for (unsigned int cpu = 0; cpu < data->nr_cpus_perf; cpu++)
	{
		if (!cpu_online(cpu))
		{
			pr_alert("perf CPU: %d is NOT online, skipping event enable.\n", cpu);
			continue;
		}
		struct perf_event *event = data->events[cpu];
		perf_event_enable(event);

#if DEBUG
		print_perf_event_state(event);
#endif
	}
	return 0;
}

static int disable_perf_events(struct device *dev)
{
	pr_alert("\n");
	energy_t *data = dev_get_drvdata(dev);
	for (unsigned int cpu = 0; cpu < data->nr_cpus_perf; cpu++)
	{
		if (!cpu_online(cpu))
		{
			pr_alert("perf CPU: %d is NOT online, skipping event disable.\n", cpu);
			continue;
		}
		struct perf_event *event = data->events[cpu];
		perf_event_disable(event);

#if DEBUG
		print_perf_event_state(event);
#endif
	}

	return 0;
}

static int release_perf_event_kernel_counters(struct device *dev)
{
	int ret = 0;
	energy_t *data = dev_get_drvdata(dev);
	for (unsigned int cpu = 0; cpu < data->nr_cpus_perf; cpu++)
	{
		struct perf_event *event = data->events[cpu];
		ret |= perf_event_release_kernel(event);
	}
	return ret;
}

/* perf enumerates hw threads for SMT/hyper-threading, multiply by 2 */
static unsigned int get_perf_cpu_count(void)
{
	return get_core_cpu_count() * 2;
}

static int perf_alloc_cpu_cores(struct device *dev)
{
	energy_t *data = dev_get_drvdata(dev);
	data->nr_cpus_perf = get_perf_cpu_count();
	return 0;
}

static void perf_set_socket_config_hwmon(energy_t *data, unsigned int *socket_config)
{
	// TODO: last level socket = 0 ?
	for (unsigned int cpu = 0; cpu < data->nr_cpus_perf; cpu++)
	{
		socket_config[cpu] = HWMON_E_INPUT | HWMON_E_LABEL;
	}
}

static int perf_alloc_socket_config(struct device *dev)
{
	energy_t *data = dev_get_drvdata(dev);
	unsigned int *socket_config = devm_kcalloc(dev, data->nr_cpus_perf, sizeof(u32), GFP_KERNEL);
	if (!socket_config)
	{
		return -ENOMEM;
	}
	perf_set_socket_config_hwmon(data, socket_config);
	set_hwmon_channel_info(data, socket_config);

	return 0;
}


static int perf_alloc_sensor_accumulator(struct device *dev)
{
	energy_t *data = dev_get_drvdata(dev);
	energy_accum_t *accums = devm_kcalloc(dev, data->nr_cpus_perf, sizeof(energy_accum_t), GFP_KERNEL);
	if (!accums)
	{
		return -ENOMEM;
	}
	data->accums = accums;
	return 0;
}

static void perf_set_label_l(energy_t *data, char (*label_l)[10])
{
	data->label = label_l;

	for (int i = 0; i < data->nr_cpus_perf; i++)
	{
		scnprintf(label_l[i], 10, "Pcore%03u", i);
	}
}

static int perf_alloc_label_l(struct device *dev)
{
	energy_t *data = dev_get_drvdata(dev);
	char(*label_l)[10];
	label_l = devm_kcalloc(dev, data->nr_cpus_perf, sizeof(*label_l), GFP_KERNEL);
	if (!label_l)
	{
		return -ENOMEM;
	}
	perf_set_label_l(data, label_l);
	return 0;
}

#endif /* _PERF_H */