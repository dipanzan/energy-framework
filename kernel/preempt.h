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

static void init_preempt_notifier(struct task_struct *p)
{
    if (is_preempt_notifier_registered(p))
    {
        pr_alert("preempt_notifier already registered: %s(%d)\n", p->comm, p->pid);
        return;
    }

    struct preempt_notifier *notifier = kmalloc(sizeof(struct preempt_notifier), GFP_KERNEL);
    if (!notifier)
    {
        pr_alert("preempt_notifier alloc failed: %s(%d)\n", p->comm, p->pid);
        return;
    }
    INIT_HLIST_HEAD(&p->preempt_notifiers);
    preempt_notifier_init(notifier, &p_ops);
    // notifier->ops = &p_ops;
    preempt_notifier_inc();
    hlist_add_head(&notifier->link, &p->preempt_notifiers);
    pr_alert("preempt_notifier registered: %s(%d)\n", p->comm, p->pid);
}

static void init_preempt_notifiers(struct task_struct *p)
{
    struct task_struct *t = p;
    rcu_read_lock();
    init_preempt_notifier(p); // parent (p) is not included in the while_each_thread
    while_each_thread(p, t)
    {
        init_preempt_notifier(t); // all other threads (t) other than parent (p)
    }
    rcu_read_unlock();
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
        kfree(notifier);
        pr_alert("preempt_notifier released: %s(%d)\n", p->comm, p->pid);
    }
}

static void release_preempt_notifiers(struct task_struct *p)
{
    struct task_struct *t = p;

    rcu_read_lock();
    release_preempt_notifier(p); // parent (p) is not included in the while_each_thread
    while_each_thread(p, t)
    {
        release_preempt_notifier(t); // all other threads (t) other than parent (p)
    }
    rcu_read_unlock();
}

static void _preempt_notifier_unregister(struct preempt_notifier *notifier, struct task_struct *t)
{
    preempt_notifier_dec();
    hlist_del(&notifier->link);
    pr_alert("preempt_notifier unregistered: %s(%d)\n", t->comm, t->pid);
}

static enum STATUS status = NOT_STARTED;

static void ___preempt_notifier_register___(struct task_struct *p)
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

static void ___preempt_notifier_unregister___(struct task_struct *p)
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