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
    pr_alert("IN: [%s (PID: %d, CPU: %d)]\n", current->comm, current->pid, cpu);
    // if (cpu_energy_pd == NULL)
    // {
    //     pr_alert("cpu_energy_pd is NULL :(\n");
    // }
    // else
    // {
    //     pr_alert("cpu_energy_pd is NOT NULL :D :D\n");
    // }
}

static void __sched_out(struct preempt_notifier *notifier, struct task_struct *next)
{
    pr_alert("OUT: [%s (PID: %d, CPU: %d)], NEXT: [%s (PID: %d, CPU: %d)]\n",
             current->comm, current->pid, current->thread_info.cpu,
             next->comm, next->pid, next->thread_info.cpu);
}

static bool is_preempt_notifier_registered(struct task_struct *t)
{
    if (!t->preempt_notifiers.first)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

static inline void register_preempt_notifier(struct preempt_notifier *notifier, struct task_struct *p)
{
    notifier->ops = &p_ops;
    INIT_HLIST_HEAD(&p->preempt_notifiers);
    preempt_notifier_inc();
    hlist_add_head(&notifier->link, &p->preempt_notifiers);

    pr_alert("%s (pid: %d): notifier registered\n", p->comm, p->pid);
}

static void __init_preempt_notifier(struct task_struct *p)
{
    struct preempt_notifier *notifier = kmalloc(sizeof(struct preempt_notifier), GFP_KERNEL);

    if (!notifier)
    {
        pr_alert("%s: preempt_notifier alloc failed.\n", p->comm);
        return;
    }
    register_preempt_notifier(notifier, p);
}

static void init_preempt_notifiers(struct task_struct *p)
{
    // special-condition of tgid already registered?
    // likely will optimize branch check significantly

    rcu_read_lock();
    struct task_struct *t = p;
    while_each_thread(p, t)
    {
        if (!is_preempt_notifier_registered(t))
        {
            __init_preempt_notifier(t);
        }
    }
    rcu_read_unlock();
}

static void release_preempt_notifiers(struct task_struct *p)
{
    struct task_struct *t = p;
    while_each_thread(p, t)
    {
        if (is_preempt_notifier_registered(t))
        {
            // free notifier
        }
    }
}

static void _preempt_notifier_unregister(struct preempt_notifier *notifier, struct task_struct *t)
{
    preempt_notifier_dec();
    hlist_del(&notifier->link);
    pr_alert("[%s (PID: %d)] notifier unregistered\n", t->comm, t->pid);
}

static int alloc_preempt_notifiers(struct device *dev)
{
    energy_t *data = dev_get_drvdata(dev);
    struct preempt_notifier *notifiers = devm_kcalloc(dev, data->nr_cpus_perf, sizeof(struct preempt_notifier), GFP_KERNEL);
    if (!notifiers)
    {
        pr_alert("%s(): failed!\n", __FUNCTION__);
        return -ENOMEM;
    }
    data->notifiers = notifiers;
    return 0;
}

static int init_preempt_callbacks(struct device *dev)
{
    energy_t *data = dev_get_drvdata(dev);
    for (unsigned int cpu = 0; cpu < data->nr_cpus_perf; cpu++)
    {
        struct preempt_notifier notifier = data->notifiers[cpu];
        notifier.ops = &p_ops;
    }
    return 0;
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
        // preempt_notifier_register(notifier);
        hlist_add_head(&notifier->link, &p->preempt_notifiers);

        status = RUNNING;
    }
}

static void __preempt_notifier_unregister(struct task_struct *p)
{
    if (status == COMPLETE)
    {
        pr_alert("%s() =======================\n", __FUNCTION__);
        preempt_notifier_dec();
        // preempt_notifier_unregister(notifier);
        hlist_del(&notifier->link);
        status = NOT_STARTED;
    }
}

#endif /* _PREEMPT_H */