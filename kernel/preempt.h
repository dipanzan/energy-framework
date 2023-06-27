#ifndef _PREEMPT_H
#define _PREEMPT_H

static void __sched_in(struct preempt_notifier *notifier, int cpu);
static void __sched_out(struct preempt_notifier *notifier, struct task_struct *next);

static const struct preempt_ops p_ops = {
    .sched_in = __sched_in,
    .sched_out = __sched_out};


struct preempt_notifier p_notifier = {
    .link = NULL,
    .ops = &p_ops,
};

static void __sched_in(struct preempt_notifier *notifier, int cpu)
{
    pr_alert("preempt: sched_in(): cpu: %d\n", cpu);
}

static void __sched_out(struct preempt_notifier *notifier, struct task_struct *next)
{
    pr_alert("preempt: sched_out()\n");
}

static void __preempt_notifier_register(struct preempt_notifier *notifier, struct task_struct *p)
{
    pr_alert("%s() called\n", __FUNCTION__);
    hlist_add_head(&notifier->link, &p->preempt_notifiers);
}

static void __preempt_notifier_unregister(struct preempt_notifier *notifier)
{
    pr_alert("%s() called\n", __FUNCTION__);
    preempt_notifier_unregister(notifier);
}

#endif /* _XPREEMPT_H */