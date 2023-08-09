#ifndef _PREEMPT_H
#define _PREEMPT_H

static void __sched_in(struct preempt_notifier *notifier, int cpu);
static void __sched_out(struct preempt_notifier *notifier, struct task_struct *next);

enum STATUS
{
    NOT_STARTED = 0,
    RUNNING = 1,
    COMPLETE = 2,
    READ_ERROR = 3
};

static volatile enum STATUS status = NOT_STARTED;

static const struct preempt_ops p_ops = {
    .sched_in = __sched_in,
    .sched_out = __sched_out};

static void lock_process_on_cpu(pid_t pid, unsigned int cpu)
{
    struct cpumask mask;
    cpumask_clear(&mask);
    cpumask_set_cpu(cpu, &mask);
    sched_setaffinity_func(pid, &mask);
    pr_alert("%s() called for pid: %d on cpu: %d\n", __FUNCTION__, pid, cpu);
}

/**
 * preempt_ops - notifiers called when a task is preempted and rescheduled
 * @sched_in: we're about to be rescheduled:
 *    notifier: struct preempt_notifier for the task being scheduled
 *    cpu:  cpu we're scheduled on
 * @sched_out: we've just been preempted
 *    notifier: struct preempt_notifier for the task being preempted
 *    next: the task that's kicking us out
 *
 * Please note that sched_in and out are called under different
 * contexts.  sched_out is called with rq lock held and irq disabled
 * while sched_in is called without rq lock and irq enabled.  This
 * difference is intentional and depended upon by its users.
 */

static void ____sched_in(struct preempt_notifier *notifier, int cpu)
{
    // pr_alert("in_atomic(): %d\n", in_atomic());
    if (unlikely(in_nmi() || in_hardirq() || in_serving_softirq()))
    {
        return;
    }
    // pr_alert("IN: [%s (PID: %d, thread_info CPU: %d, notifier CPU: %d, smp CPU: %d)]\n",
    //     current->comm, current->pid, current->thread_info.cpu,
    //     cpu, smp_processor_id());

    volatile struct device *dev = &cpu_energy_pd->dev;
    volatile energy_t *data = dev_get_drvdata(dev);
    volatile struct perf_event *event = data->perf[cpu].event;

    data->new_value = read_pmu(event);
    data->perf->new_values[cpu] = data->new_value;

    data->reading_value += data->new_value - data->old_value;

    data->perf->reading_values[cpu] += data->perf->new_values[cpu] - data->perf->old_values[cpu];
}

static void ____sched_out(struct preempt_notifier *notifier, struct task_struct *next)
{
    // pr_alert("in_atomic(): %d\n", in_atomic());
    if (unlikely(in_nmi() || in_hardirq() || in_serving_softirq()))
    {
        return;
    }
    // preempt_disable();
    // pr_alert("OUT: [%s (PID: %d, CPU: %d)], NEXT: [%s (PID: %d, CPU: %d)]\n",
    //          current->comm, current->pid, current->thread_info.cpu,
    //          next->comm, next->pid, next->thread_info.cpu);

    volatile struct device *dev = &cpu_energy_pd->dev;
    volatile energy_t *data = dev_get_drvdata(dev);
    volatile int cpu = current->thread_info.cpu;
    volatile struct perf_event *event = data->perf[cpu].event;

    data->old_value = read_pmu(event);
    data->perf->old_values[cpu] = data->old_value;
}

static inline bool is_task_alive(struct task_struct *p)
{
    return p != NULL;
}

static inline bool is_preempt_notifier_registered(struct task_struct *p)
{
    return !hlist_empty(&p->preempt_notifiers);
}

// WARNING: this function HAS TO BE FAST!
// TODO: optimize more!
static void init_preempt_notifier(volatile struct task_struct *p)
{
    if (unlikely(!is_task_alive(p)))
    {
        pr_alert("TASK DEAD!\n");
        return;
    }

    // branch-optimize: preempt_threads probably already initialized.
    // gets a performance hit for newer registrations, but after that it should be performant
    // provided that no new threads are spawning! ;)
    if (likely(is_preempt_notifier_registered(p)))
    {
        // pr_alert("preempt_notifier already registered: %s(%d)\n", p->comm, p->pid);
        return;
    }

    struct preempt_notifier *notifier = kmalloc(sizeof(struct preempt_notifier), GFP_KERNEL);
    if (!notifier)
    {
        pr_alert("preempt_notifier alloc failed: %s(%d)\n", p->comm, p->pid);
        return;
    }

    /* WARNING: static initializer viable? */
    struct preempt_ops *ops = kmalloc(sizeof(struct preempt_ops), GFP_KERNEL);
    if (!ops)
    {
        pr_alert("preempt_ops alloc failed: %s(%d)\n", p->comm, p->pid);
        return;
    }
    ops->sched_in = ____sched_in;
    ops->sched_out = ____sched_out;

    get_task_struct(p);
    // rcu_read_lock();
    INIT_HLIST_HEAD(&p->preempt_notifiers);
    preempt_notifier_init(notifier, ops);
    preempt_notifier_inc();
    hlist_add_head(&notifier->link, &p->preempt_notifiers);
    put_task_struct(p);
    // rcu_read_unlock();

    pr_alert("preempt_notifier registered: %s(%d)\n", p->comm, p->pid);
}

static int scan_preempt_registration(void *data)
{
    pr_alert("%s() called\n", __FUNCTION__);
    volatile struct task_struct *p = (struct task_struct *)data;
    while (!kthread_should_stop())
    {
        // rcu_read_lock();
        volatile struct task_struct *t = p;

        while_each_thread(p, t)
        {
            init_preempt_notifier(t); // all other threads (t) other than parent (p)
            // struct task_struct *temp = container_of(&t->preempt_notifiers, struct task_struct, psreempt_notifiers);
            // temp ? pr_alert("%s(%d)\n", temp->comm, temp->pid) : pr_alert("NOT VALID!\n");
        }

        // MAJOR WARNING: HAS TO FINISH THIS UNLOCK CALL, or probable SOFT-LOCKUP!
        // rcu_read_unlock();

        if (kthread_should_stop())
        {
            break;
        }
        long preempt_running = schedule_timeout_interruptible(msecs_to_jiffies(1000));
    }
    return 0;
}

static void start_preempt_scan_thread(struct device *dev, struct task_struct *p)
{
    energy_t *data = dev_get_drvdata(dev);
    // data->preempt_runner = kthread_run(scan_preempt_registration, p, PREEMPT_SCAN_THREAD, dev_name(dev));
    data->preempt_runner = kthread_create(scan_preempt_registration, p, PREEMPT_SCAN_THREAD, dev_name(dev));
    get_task_struct(data->preempt_runner);
    wake_up_process(data->preempt_runner);
}

static void init_preempt_notifiers(struct device *dev, struct task_struct *p)
{
    if (status == RUNNING)
    {
        status = COMPLETE;
        return;
    }

    if (status == NOT_STARTED)
    {
        init_preempt_notifier(p);
        start_preempt_scan_thread(dev, p);
        status = RUNNING;
    }
}

static void release_preempt_notifier(volatile struct task_struct *p)
{
    if (unlikely(!is_task_alive(p)))
    {
        pr_alert("TASK DEAD!\n");
        return;
    }

    if (!is_preempt_notifier_registered(p))
    {
        pr_alert("preempt_notifier cannot release: %s(%d) not registered\n", p->comm, p->pid);
        return;
    }

    local_irq_disable();
    get_task_struct(p);
    struct hlist_node *node, *temp;
    hlist_for_each_safe(node, temp, &p->preempt_notifiers)
    {
        struct preempt_notifier *notifier = container_of(node, struct preempt_notifier, link);
        hlist_del(&notifier->link);
        preempt_notifier_dec();
        kfree(notifier->ops);
        kfree(notifier);
        pr_alert("preempt_notifier released: %s(%d)\n", p->comm, p->pid);
    }
    put_task_struct(p);
    local_irq_enable();
}

static void stop_preempt_scan_thread(struct device *dev, volatile struct task_struct *p)
{
    pr_alert("%s() called\n", __FUNCTION__);
    energy_t *data = dev_get_drvdata(dev);

    if (data && data->preempt_runner)
    {
        kthread_stop(data->preempt_runner);
        put_task_struct(data->preempt_runner);
    }
}

static void release_preempt_notifiers(struct device *dev, volatile struct task_struct *p)
{
    if (status == COMPLETE)
    {
        energy_t *data = dev_get_drvdata(dev);
        stop_preempt_scan_thread(dev, p);

        rcu_read_lock();
        volatile struct task_struct *t = p;
        release_preempt_notifier(p); // parent (p) is not included in the while_each_thread
        while_each_thread(p, t)
        {
            release_preempt_notifier(t); // all other threads (t) other than parent (p)
        }
        rcu_read_unlock();
        status = NOT_STARTED;

        long long value = 0;
        for (unsigned int cpu = 0; cpu < data->nr_cpus_perf; cpu++)
        {
            pr_alert("READING VALUE MULTI[CPU: %d]: %ld\n", cpu, data->perf->reading_values[cpu]);
            value += data->perf->reading_values[cpu];
        }

        pr_alert("READING VALUE MULTI AVERAGE: %ld\n", value / 16);
        value = 0;


        for (unsigned int cpu = 0; cpu < data->nr_cpus_perf; cpu++)
        {
            data->perf->old_values[cpu] = 0;
            data->perf->new_values[cpu] = 0;
            data->perf->reading_values[cpu] = 0;
        }

        pr_alert("READING VALUE SINGLE: %ld\n", data->reading_value);
        data->reading_value = 0;
        data->old_value = 0;
        data->new_value = 0;
    }
}

#endif /* _PREEMPT_H */