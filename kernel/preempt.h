#ifndef _PREEMPT_H
#define _PREEMPT_H

static void __sched_in(struct preempt_notifier *notifier, int cpu);
static void __sched_out(struct preempt_notifier *notifier, struct task_struct *next);

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
    pr_alert("preempt: sched_in(): cpu: %d\n", cpu);
}

static void __sched_out(struct preempt_notifier *notifier, struct task_struct *next)
{
    pr_alert("preempt: sched_out(), next_task: %s\n", next->comm);
}

struct preempt_notifier *pn;
int status = 0;

static void init_preempt_notifier(void)
{
    pn = kmalloc(sizeof(struct preempt_notifier *), GFP_KERNEL);
    pn->ops = &p_ops;
}

static void release_preempt_notifier(void)
{
    kfree(pn);
}

static void __preempt_notifier_register(struct task_struct *p)
{
    // pr_alert("%s() called\n", __FUNCTION__);

    // pr_alert("status: %d\n", status);
    if (status == 1)
    {
        status = 2;
        // pr_alert("marking status COMPLETE\n");
    }

    if (status == 0)
    {
        INIT_HLIST_HEAD(&p->preempt_notifiers);
        preempt_notifier_inc();
        preempt_notifier_register(pn);
        status = 1;
        // pr_alert("marking status IN PROGRESS\n");
    }
}

static void __preempt_notifier_unregister(struct task_struct *p)
{
    // pr_alert("%s() called\n", __FUNCTION__);

    if (status == 2)
    {
        preempt_notifier_dec();
        preempt_notifier_unregister(pn);

        status = 0;
        // pr_alert("marking status NOT RUNNING\n");
    }
    pr_alert("==========================================\n");
}

#endif /* _PREEMPT_H */