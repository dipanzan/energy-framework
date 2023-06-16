#ifndef _PERF_H
#define _PERF_H


static inline void set_perf_event_energy_attrs(energy_t *data, struct perf_event_attr *attrs)
{
	for (unsigned int cpu = 0; cpu < data->nr_cpus; cpu++)
	{
		attrs[cpu].type = ENERGY_TYPE;
		attrs[cpu].config = ENERGY_CONFIG;
		attrs[cpu].exclude_user = 0;
		attrs[cpu].exclude_kernel = 0;
		attrs[cpu].pinned = 1;
		attrs[cpu].inherit = 1;
		attrs[cpu].comm = 1;
		attrs[cpu].sample_type = PERF_SAMPLE_IDENTIFIER;
		attrs[cpu].read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING | PERF_FORMAT_ID;
		attrs[cpu].disabled = 1;
		attrs[cpu].aux_output = 0;
	}
	data->attrs = attrs;
}

static int alloc_perf_event_attrs(struct device *dev)
{
	energy_t *data = dev_get_drvdata(dev);
	struct perf_event_attr *attrs = devm_kcalloc(dev, data->nr_cpus, sizeof(struct perf_event_attr), GFP_KERNEL);

	if (!attrs)
	{
		return -ENOMEM;
	}
	set_perf_event_energy_attrs(data, attrs);
	return 0;
}

static void perf_overflow_handler(struct perf_event *event, struct perf_sample_data *data, struct pt_regs *regs)
{
	u64 enabled, running, counter;
	counter = perf_event_read_value(event, &enabled, &running);
	pr_alert("counter overflow: %llu\n", counter);
}

static int alloc_perf_event_kernel_counters(struct device *dev)
{
	pr_info("\n");
	energy_t *data = dev_get_drvdata(dev);
	struct perf_event_attr *attrs = data->attrs;

	for (unsigned int cpu = 0; cpu < data->nr_cpus; cpu++)
	{
		struct perf_event *event = perf_event_create_kernel_counter(&attrs[cpu], cpu, NULL, perf_overflow_handler, NULL);
		if (IS_ERR(event))
		{
			pr_alert("perf_event_create failed on CPU[%d] with error: %ld\n", cpu, PTR_ERR(event));
			return PTR_ERR(event);
		}
		print_perf_event_state(event);
		data->events[cpu] = event;
	}
	return 0;
}

static inline void print_perf_pmu(struct perf_event *event, unsigned int cpu)
{
	const char *name = event->pmu->name;
	pr_info("CPU[%d]: pmu: %s\n", cpu, name);
}

static int enable_perf_events(struct device *dev)
{
	pr_info("\n");
	energy_t *data = dev_get_drvdata(dev);
	for (unsigned int cpu = 0; cpu < data->nr_cpus; cpu++)
	{
		if (!cpu_online(cpu))
		{
			continue;
		}
		struct perf_event *event = data->events[cpu];
		perf_event_enable(event);
		print_perf_pmu(event, cpu);
		print_perf_event_state(event);
	}
	return 0;
}

static int disable_perf_events(struct device *dev)
{
	pr_alert("\n");
	energy_t *data = dev_get_drvdata(dev);
	for (unsigned int cpu = 0; cpu < data->nr_cpus; cpu++)
	{
		if (!cpu_online(cpu))
		{
			continue;
		}
		struct perf_event *event = data->events[0];
		perf_event_disable(event);
		print_perf_event_state(event);
	}

	return 0;
}

static int release_perf_event_kernel_counters(struct device *dev)
{
	pr_alert("\n");
	int ret = 0;
	energy_t *data = dev_get_drvdata(dev);
	for (unsigned int cpu = 0; cpu < data->nr_cpus; cpu++)
	{
		struct perf_event *event = data->events[cpu];
		ret |= perf_event_release_kernel(event);
	}
	return ret;
}

#endif /* _PERF_H */