#ifndef _PREEMPT_H
#define _PREEMPT_H

static void __sched_in(struct preempt_notifier *notifier, int cpu);
static void __sched_out(struct preempt_notifier *notifier, struct task_struct *next);

/* MAJOR WARNING: DO NOT USE IRQ toggle
   You will undoubtedly mess it up, and LOCK UP the system!
   No amount of precaution checks will help here, you have been warned!
   This is for a future reader. :)
 */
static void irq_toggle(pmu_func func, const struct perf_event *event)
{
    local_irq_disable();
    func(event);
    local_irq_enable();
}

static void rcu_toggle(pmu_func func, const struct perf_event *event)
{
    rcu_read_lock();
    func(event);
    rcu_read_unlock();
}

/* 
    preempt_toogle() is used in the pmu functions from perf.h for start/stop/disable/enable
    This still has a tendency for a kernel oops, if the pmu_* functions doesn't complete on time i.e sleeps/interrupted.
    Please use this with caution!
 */
static u64 preempt_toggle(pmu_func func, const struct perf_event *event)
{
    u64 ret;
    preempt_disable();
    ret = func(event);
    preempt_enable();
    return ret;
}

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
    // pr_alert("%s() called for pid: %d on cpu: %d\n", __FUNCTION__, pid, cpu);
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

static __always_inline bool in_nmi_or_softirq_or_hardirq_context(void)
{
    return in_nmi() || in_hardirq() || in_serving_softirq();
}

// __always_inline MAYBE?
static void ____sched_in(struct preempt_notifier *notifier, int cpu)
{
    /*
        Very important not to reading logic here, everything is in atomic context
        in sched_in(). If in the unlikely chance, you are in nmi, soft/hard irq and
        try to do either read_msr or variants or perf_event_read, it'll crash the system.
        There's no mechanism for sleeping in atomic contexts, so using rcu_read/write locks are
        also not going to work here.

        This is for a future reader - you have been warned!
     */
    if (unlikely(in_nmi_or_softirq_or_hardirq_context()))
    {
        return;
    }
    // pr_alert("IN: [%s (PID: %d, thread_info CPU: %d, notifier CPU: %d, smp CPU: %d)]\n",
    //     current->comm, current->pid, current->thread_info.cpu,
    //     cpu, smp_processor_id());

    volatile struct device *dev = &cpu_energy_pd->dev;
    volatile energy_t *data = dev_get_drvdata(dev);
    volatile struct perf_event *event = data->perf[cpu].event;

    preempt_toggle(enable_pmu, event);
    // preempt_toggle(start_pmu, event);
}

static void ____sched_out(struct preempt_notifier *notifier, struct task_struct *next)
{
    // pr_alert("in_atomic(): %d\n", in_atomic());
    if (unlikely(in_nmi_or_softirq_or_hardirq_context()))
    {
        return;
    }
    // pr_alert("OUT: [%s (PID: %d, CPU: %d)], NEXT: [%s (PID: %d, CPU: %d)]\n",
    //          current->comm, current->pid, current->thread_info.cpu,
    //          next->comm, next->pid, next->thread_info.cpu);

    volatile struct device *dev = &cpu_energy_pd->dev;
    volatile energy_t *data = dev_get_drvdata(dev);
    volatile int cpu = current->thread_info.cpu;
    volatile struct perf_event *event = data->perf[cpu].event;

    preempt_toggle(disable_pmu, event);

    // preempt_toggle(stop_pmu, event);
    // data->perf->energy_counters[cpu] = preempt_toggle(read_pmu, event) - data->perf->energy_counters[cpu];
}

static inline bool is_task_alive(volatile struct task_struct *p)
{
    return p != NULL;
}

static inline bool is_preempt_notifier_registered(volatile struct task_struct *p)
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
    INIT_HLIST_HEAD(&p->preempt_notifiers);
    preempt_notifier_init(notifier, ops);
    // preempt_notifier_inc();
    hlist_add_head(&notifier->link, &p->preempt_notifiers);
    put_task_struct(p);

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
        // find_threads(t);
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

static int energy_runner(void *p)
{
    pr_alert("%s() called\n", __FUNCTION__);
    volatile energy_t *data = (energy_t *)p;
    while (!kthread_should_stop())
    {
        for (unsigned int cpu = 0; cpu < data->nr_cpus_perf; cpu++)
        {
            struct perf_event *event = data->perf[cpu].event;

            u64 energy_counter_old = data->perf->energy_counters[cpu];
            u64 energy_counter_new = read_pmu(event);
            data->perf->energy_counters[cpu] = energy_counter_new - energy_counter_old;
        }
        if (kthread_should_stop())
        {
            break;
        }
        long energy_running = schedule_timeout_uninterruptible(msecs_to_jiffies(1));
    }
    return 0;
}

static void start_energy_runner_thread(struct device *dev)
{
    energy_t *data = dev_get_drvdata(dev);
    data->energy_runner = kthread_create(energy_runner, data, ENERGY_RUNNER_THREAD, dev_name(dev));
    if (IS_ERR(data->energy_runner))
    {
        pr_alert("%s failed to start.\n", ENERGY_RUNNER_THREAD);
        return;
    }
    get_task_struct(data->energy_runner);
    wake_up_process(data->energy_runner);
}

static void start_preempt_scan_thread(struct device *dev, struct task_struct *p)
{
    energy_t *data = dev_get_drvdata(dev);
    // data->preempt_runner = kthread_run(scan_preempt_registration, p, PREEMPT_SCAN_THREAD, dev_name(dev));

    /* DO NOT USE kthread_run()
       The explicit reason for starting the scan thread is because of preempt release mechanism
       In order make sure the refcount for the scan thread is accurate when starting the thread
       prevents it from crashing during release!
     */
    data->preempt_runner = kthread_create(scan_preempt_registration, p, PREEMPT_SCAN_THREAD, dev_name(dev));
    if (IS_ERR(data->preempt_runner))
    {
        pr_alert("%s failed to start.\n", PREEMPT_SCAN_THREAD);
        return;
    }
    // data->preempt_runner = kthread_create_on_cpu(scan_preempt_registration, p, 4, PREEMPT_SCAN_THREAD);
    get_task_struct(data->preempt_runner);
    wake_up_process(data->preempt_runner);
}

static void stop_energy_runner_thread(struct device *dev)
{
    pr_alert("%s() called\n", __FUNCTION__);
    energy_t *data = dev_get_drvdata(dev);

    if (data && data->energy_runner)
    {
        kthread_stop(data->energy_runner);
        put_task_struct(data->energy_runner);
    }
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
        // start_energy_runner_thread(dev);
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

    get_task_struct(p);
    struct hlist_node *node, *temp;
    hlist_for_each_safe(node, temp, &p->preempt_notifiers)
    {
        struct preempt_notifier *notifier = container_of(node, struct preempt_notifier, link);
        hlist_del(&notifier->link);
        // preempt_notifier_dec();
        kfree(notifier->ops);
        kfree(notifier);
        pr_alert("preempt_notifier released: %s(%d)\n", p->comm, p->pid);
    }
    put_task_struct(p);
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

static void print_numbers(energy_t *data)
{
    u64 total = 0;
    for (unsigned int cpu = 0; cpu < data->nr_cpus_perf; cpu++)
    {
        pr_alert("data->perf->energy_counters[%d] = %ld\n", cpu, data->perf->energy_counters[cpu]);
        total += data->perf->energy_counters[cpu];
    }
    pr_alert("TOTAL = %ld\n", total);

}
static void release_preempt_notifiers(struct device *dev, volatile struct task_struct *p)
{
    if (status == COMPLETE)
    {
        energy_t *data = dev_get_drvdata(dev);
        stop_preempt_scan_thread(dev, p);
        // stop_energy_runner_thread(dev);

        rcu_read_lock();
        volatile struct task_struct *t = p;
        release_preempt_notifier(p); // parent (p) is not included in the while_each_thread
        while_each_thread(p, t)
        {
            release_preempt_notifier(t); // all other threads (t) other than parent (p)
        }
        rcu_read_unlock();
        status = NOT_STARTED;
        print_numbers(data);
    }
}

#endif /* _PREEMPT_H */