#ifndef _PREEMPT_H
#define _PREEMPT_H

static void __sched_in(struct preempt_notifier *notifier, int cpu);
static void __sched_out(struct preempt_notifier *notifier, struct task_struct *next);

enum STATUS
{
    NOT_STARTED = 0,
    RUNNING = 1,
    COMPLETE = 2
};

static struct preempt_notifier *notifier;
static enum STATUS *statuses;

static const struct preempt_ops p_ops = {
    .sched_in = __sched_in,
    .sched_out = __sched_out};

static void lock_process_on_cpu(pid_t pid, unsigned int cpu)
{
    pr_alert("%s called for pid: %d on cpu: %d\n", __FUNCTION__, pid, cpu);
    struct cpumask mask;
    cpumask_clear(&mask);
    cpumask_set_cpu(cpu, &mask);
    sched_setaffinity_func(pid, &mask);
}

static void __sched_in(struct preempt_notifier *notifier, int cpu)
{
    pr_alert("%s(): current: %s(%d), cpu: %d\n", __FUNCTION__, current->comm, current->pid, cpu);
}

static void __sched_out(struct preempt_notifier *notifier, struct task_struct *next)
{
    pr_alert("%s(): current: %s(%d), next: %s(%d)\n", __FUNCTION__, current->comm, current->pid, next->comm, next->pid);
}

static struct preempt_notifier *alloc_preempt_notifiers(struct device *dev, unsigned int cores)
{
    energy_t *data = dev_get_drvdata(dev);
    struct preempt_notifier *notifiers = devm_kcalloc(dev, cores, sizeof(struct preempt_notifier), GFP_KERNEL);
    if (!notifiers)
    {
        return NULL;
    }
    data->notifiers = notifiers;
    return data->notifiers;
}
static int init_preempt_notifier(struct platform_device *pd)
{
    struct device *dev = &pd->dev;
    energy_t *data = dev_get_drvdata(dev);

    notifier = devm_kzalloc(dev, sizeof(struct preempt_notifier), GFP_KERNEL);
    statuses = devm_kzalloc(dev, data->nr_cpus_perf * sizeof(enum STATUS), GFP_KERNEL);

    if (!notifier || !statuses)
    {
        pr_alert("%s() failed\n", __FUNCTION__);
        return -ENOMEM;
    }
    notifier->ops = &p_ops;
    return 0;
}

static enum STATUS status = NOT_STARTED;

static void __preempt_notifier_register(struct task_struct *p)
{
    if (status == RUNNING)
    {
        status = COMPLETE;
        return;
    }

    if (status == NOT_STARTED)
    {
        pr_alert("%s() =======================\n", __FUNCTION__);
        INIT_HLIST_HEAD(&p->preempt_notifiers);
        preempt_notifier_inc();
        preempt_notifier_register(notifier);

        status = RUNNING;
    }
}

static void __preempt_notifier_unregister(struct task_struct *p)
{
    if (status == COMPLETE)
    {
        pr_alert("%s() =======================\n", __FUNCTION__);
        preempt_notifier_dec();
        preempt_notifier_unregister(notifier);

        status = NOT_STARTED;
    }
}

#endif /* _PREEMPT_H */