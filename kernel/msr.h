#ifndef _MSR_H
#define _MSR_H

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
	struct energy_accumulator *accumulator;

	mutex_lock(&data->lock);
	u64 value = read_msr_on_cpu(cpu, ENERGY_CORE_MSR);

	if (!data->do_not_accum)
	{
		accumulator = &data->accumulators[channel];
		if (value >= accumulator->prev_value)
		{
			value += accumulator->energy_ctr - accumulator->prev_value;
		}
		else
		{
			value += UINT_MAX - accumulator->prev_value + accumulator->energy_ctr;
		}
	}
	energy_consumed_ujoules(data, value, val);
	mutex_unlock(&data->lock);
}

static void set_energy_unit(energy_t *data)
{
	u64 energy_unit;

	rdmsrl_safe(ENERGY_PWR_UNIT_MSR, &energy_unit);
	data->energy_unit = (energy_unit & AMD_ENERGY_UNIT_MASK) >> 8;
}

static u64 read_msr_unsafe_on_cpu(int cpu, u32 reg)
{
	u64 value;
	rdmsrl_on_cpu(cpu, reg, &value);
	value &= AMD_ENERGY_MASK;
	return value;
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

static void handle_ctr_overflow(energy_t *data, int channel, u64 value)
{
	energy_accum_t *accumulator = &data->accumulators[channel];
	if (value >= accumulator->prev_value)
	{
		accumulator->energy_ctr += value - accumulator->prev_value;
	}
	else
	{
		accumulator->energy_ctr += UINT_MAX - accumulator->prev_value + value;
	}
	accumulator->prev_value = value;
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
		struct energy_accumulator *accum = &data->accumulators[channel];
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

static long long read_pkg_energy_WIP(energy_t *data)
{
	u64 value = read_msr_unsafe_on_cpu(0, ENERGY_PKG_MSR);
	return div64_ul(value * 1000000UL, BIT(data->energy_unit));
}

#endif /* _MSR_H */