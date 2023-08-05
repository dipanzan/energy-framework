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

static enum STATUS status = NOT_STARTED;

static void lock_process_on_cpu(pid_t pid, unsigned int cpu)
{
    struct cpumask mask;
    cpumask_clear(&mask);
    cpumask_set_cpu(cpu, &mask);
    sched_setaffinity_func(pid, &mask);
    pr_alert("%s called for pid: %d on cpu: %d\n", __FUNCTION__, pid, cpu);
}

static void __sched_in(struct preempt_notifier *notifier, int cpu)
{
    // pr_alert("IN: [%s (PID: %d, CPU: %d)]\n", current->comm, current->pid, cpu);
}

static void __sched_out(struct preempt_notifier *notifier, struct task_struct *next)
{
    // pr_alert("OUT: [%s (PID: %d, CPU: %d)], NEXT: [%s (PID: %d, CPU: %d)]\n",
    //          current->comm, current->pid, current->thread_info.cpu,
    //          next->comm, next->pid, next->thread_info.cpu);
}

static bool is_preempt_notifier_registered(struct task_struct *p)
{
    return !hlist_empty(&p->preempt_notifiers);
}

// WARNING: this function HAS TO BE FAST!
// TODO: optimize more!
static void init_preempt_notifier(struct task_struct *p)
{
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
    ops->sched_in = __sched_in;
    ops->sched_out = __sched_out;

    INIT_HLIST_HEAD(&p->preempt_notifiers);
    preempt_notifier_init(notifier, ops);
    preempt_notifier_inc();
    hlist_add_head(&notifier->link, &p->preempt_notifiers);
    pr_alert("preempt_notifier registered: %s(%d)\n", p->comm, p->pid);
}

static int scan_preempt_registration(void *data)
{
    pr_alert("%s() called.", __FUNCTION__);
    struct task_struct *p = (struct task_struct *)data;
    while (!kthread_should_stop())
    {
        rcu_read_lock();
        struct task_struct *t = p;
        while_each_thread(p, t)
        {
            init_preempt_notifier(t); // all other threads (t) other than parent (p)
        }

        // MAJOR WARNING: HAS TO FINISH THIS UNLOCK CALL, or probable SOFT-LOCKUP!
        rcu_read_unlock();

        if (kthread_should_stop())
        {
            break;
        }
        long preempt_running = schedule_timeout_interruptible(msecs_to_jiffies(1000));
    }
    return 0;
}

static inline void start_preempt_scan_thread(struct device *dev, struct task_struct *p)
{
    energy_t *data = dev_get_drvdata(dev);
    data->preempt_runner = kthread_run(scan_preempt_registration, p, PREEMPT_SCAN_THREAD, dev_name(dev));
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

        // rcu_read_lock();
        // struct task_struct *t = p;
        // init_preempt_notifier(p); // parent (p) is not included in the while_each_thread
        // while_each_thread(p, t)
        // {
        //     init_preempt_notifier(t); // all other threads (t) other than parent (p)
        // }
        // rcu_read_unlock();
        status = RUNNING;
    }
}

static void release_preempt_notifier(struct task_struct *p)
{
    if (!is_preempt_notifier_registered(p))
    {
        pr_alert("preempt_notifier cannot release: %s(%d) not registered\n", p->comm, p->pid);
        return;
    }

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
}

static inline void stop_preempt_scan_thread(struct device *dev, struct task_struct *p)
{
    pr_alert("%s() called.", __FUNCTION__);
    energy_t *data = dev_get_drvdata(dev);
    kthread_stop(data->preempt_runner);
}

static void release_preempt_notifiers(struct device *dev, struct task_struct *p)
{
    if (status == COMPLETE)
    {
        rcu_read_lock();
        struct task_struct *t = p;
        release_preempt_notifier(p); // parent (p) is not included in the while_each_thread
        while_each_thread(p, t)
        {
            release_preempt_notifier(t); // all other threads (t) other than parent (p)
        }
        rcu_read_unlock();
        stop_preempt_scan_thread(dev, p);
        status = NOT_STARTED;
    }
    
}

#endif /* _PREEMPT_H */