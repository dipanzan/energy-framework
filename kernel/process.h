#ifndef _PROCESS_H
#define _PROCESS_H

static struct task_struct *find_process(pid_t pid);
static void find_threads(struct task_struct *p);

/* Useful process/thread traversing macros */

// #define do_each_thread(g, t) \
// 	for (g = t = &init_task ; (g = t = next_task(g)) != &init_task ; ) do

// #define while_each_thread(g, t) \
// 	while ((t = next_thread(t)) != g)

// #define __for_each_thread(signal, t)	\
// 	list_for_each_entry_rcu(t, &(signal)->thread_head, thread_node)

// #define for_each_thread(p, t)		\
// 	__for_each_thread((p)->signal, t)

// // Careful: this is a double loop, 'break' won't work as expected.
// #define for_each_process_thread(p, t)	\
// 	for_each_process(p) for_each_thread(p, t)

static void print_threads(struct task_struct *p)
{
    struct task_struct *t = p;
    // do
    // {
    //     pr_info("tgid:[%d]\tpid:[%d]\tthread:%s\tCPU:%d\n", t->tgid, t->pid, get_task_comm(comm, t), t->thread_info.cpu);
    //     t = next_thread(t);

    // } while (t != p);

    char comm[TASK_COMM_LEN];
    while_each_thread(p, t)
    {
        pr_info("tgid:[%d]\tpid:[%d]\tthread:%s\tCPU:%d,\tstate:%d\n", 
            t->tgid, t->pid, 
            get_task_comm(comm, t), 
            t->thread_info.cpu,
            t->__state);
    }
}

static void dump_process_info(pid_t pid)
{
    char comm[TASK_COMM_LEN];

    /* Does not need rcu_read_lock/unlock() primitives, it's internally called */
    struct pid *found_pid = find_get_pid(pid);
    struct task_struct *p = get_pid_task(found_pid, PIDTYPE_PID);

    pr_alert("tgid:[%d]\tpid:[%d]\tthread:%s\tCPU:%d\n", p->tgid, p->pid, get_task_comm(comm, p), p->thread_info.cpu);
    print_threads(p);
}

static struct task_struct *get_process(pid_t pid)
{
    char comm[TASK_COMM_LEN];
    
    struct pid *found_pid = find_get_pid(pid);
    struct task_struct *p = get_pid_task(found_pid, PIDTYPE_PID);

    pr_alert("tgid:[%d]\tpid:[%d]\tthread:%s\tCPU:%d\n", p->tgid, p->pid, get_task_comm(comm, p), p->thread_info.cpu);
    return p;
}

static struct task_struct *find_process(pid_t pid)
{
    char comm[TASK_COMM_LEN];
    struct task_struct *p = NULL;

    rcu_read_lock();
    for_each_process(p)
    {
        if (p->pid == pid)
        {

            pr_alert("tgid:[%d]\tpid:[%d]\tthread:%s\tCPU:%d\n", p->tgid, p->pid, get_task_comm(comm, p), p->thread_info.cpu);
            break;
        }
    }
    rcu_read_unlock();

    return p;
}

static void find_threads(struct task_struct *p)
{
    if (!p)
    {
        return;
    }

    char comm[TASK_COMM_LEN];
    struct task_struct *t = NULL;

    /*  tsk tsk, Linux doesn't diffenriate between process and threads
        threads are just processes which happens to share data ;)
    */
    rcu_read_lock();
    for_each_thread(p, t)
    {
        pr_info("pid:[%d]\tthread:[%s]\tCPU:%d\ttask_running:%d\n", t->pid, get_task_comm(comm, t), t->thread_info.cpu, t->__state);
    }
    rcu_read_unlock();
}

static void dump_process_and_threads(pid_t pid)
{
    struct task_struct *p = find_process(pid);
    find_threads(p);
}

#endif /* _PROCESS_H */