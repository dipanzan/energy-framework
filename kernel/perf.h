#ifndef _PERF_H
#define _PERF_H

#define DEAD "PERF_EVENT_STATE_DEAD [-4]\n"
#define EXIT "PERF_EVENT_STATE_EXIT [-3]\n"
#define ERROR "PERF_EVENT_STATE_ERROR [-2]\n"
#define OFF "PERF_EVENT_STATE_OFF [-1]\n"
#define INACTIVE "PERF_EVENT_STATE_INACTIVE [0]\n"
#define ACTIVE "PERF_EVENT_STATE_ACTIVE [1]\n"

static struct perf_event_attr energy_attr = {
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
	attr->read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING | PERF_FORMAT_ID;
	attr->disabled = 1;
	attr->aux_output = 0;
}

static int alloc_perf_event_attrs(struct device *dev)
{
	energy_t *data = dev_get_drvdata(dev);
	for (unsigned int cpu = 0; cpu < data->nr_cpus_perf; cpu++)
	{
		// TODO:
		// struct perf_event_attr *attr = devm_kcalloc(dev, 1, sizeof(struct perf_event_attr), GFP_KERNEL);
		struct perf_event_attr *attr = kmalloc(sizeof(struct perf_event_attr), GFP_KERNEL);
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
	pr_info("\n");
	energy_t *data = dev_get_drvdata(dev);

	for (unsigned int cpu = 0; cpu < data->nr_cpus_perf; cpu++)
	{
		struct perf_event_attr *attr = data->attrs[cpu];
		struct perf_event *event = perf_event_create_kernel_counter(attr, cpu, NULL, perf_overflow_handler, NULL);
		if (IS_ERR(event))
		{
			pr_alert("perf_event_create failed on CPU[%d] with error: %ld\n", cpu, PTR_ERR(event));
			return PTR_ERR(event);
		}
		data->events[cpu] = event;
		print_perf_cpu(event);
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
	pr_info("\n");
	energy_t *data = dev_get_drvdata(dev);
	for (unsigned int cpu = 0; cpu < data->nr_cpus_perf; cpu++)
	{
		if (!cpu_online(cpu))
		{
			continue;
		}
		struct perf_event *event = data->events[cpu];
		perf_event_enable(event);
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
			continue;
		}
		struct perf_event *event = data->events[cpu];
		perf_event_disable(event);
	}

	return 0;
}

static int release_perf_event_kernel_counters(struct device *dev)
{
	pr_alert("\n");
	int ret = 0;
	energy_t *data = dev_get_drvdata(dev);
	for (unsigned int cpu = 0; cpu < data->nr_cpus_perf; cpu++)
	{
		struct perf_event *event = data->events[cpu];
		ret |= perf_event_release_kernel(event);
	}
	return ret;
}

#endif /* _PERF_H */