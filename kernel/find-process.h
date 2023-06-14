/* typedef unsigned long long (*func_perf)(unsigned int cpu, struct perf_event *event);
static struct task_struct *find_process(unsigned int);
static inline struct task_struct *found_process(struct task_struct *, unsigned int);

static struct task_struct *find_process(unsigned int target_pid)
{
    struct task_struct *process, *thread;
    char comm[TASK_COMM_LEN];
    rcu_read_lock();
    for_each_process(process)
    {
        // for_each_thread(process, thread)
        // {
        //     pr_info("thread_name: %s\tPID:[%d]\tCPU:[%d]\n",
        //             get_task_comm(comm, thread),
        //             task_pid_nr(thread),
        //             thread->on_cpu);
        // }
        if (process->pid == target_pid)
        {
            break;
        }
    }
    rcu_read_unlock();
    return found_process(process, target_pid);
}

static inline struct task_struct *found_process(struct task_struct *process, unsigned int target_pid)
{
    if (process->pid == target_pid)
    {
        pr_alert("PID: [%d] found!\n", target_pid);
        return process;
    }
    else
    {
        pr_alert("PID: [%d] not found!\n", target_pid);
        return NULL;
    }
}
 */