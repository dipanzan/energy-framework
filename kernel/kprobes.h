#ifndef _KPROBES_H
#define _KPROBES_H

// sched functions
#define GET_TASK_STATE "get_task_state"
#define SCHED_SETNUMA "sched_setnuma"
#define MIGRATE_TASK_TO "migrate_task_to"
#define MIGRATE_SWAP "migrate_swap"

// ftrace functions
#define SET_FTRACE_FILTER "set_ftrace_filter"
#define TRACE_IGNORE_THIS_TASK "trace_ignore_this_task"




struct kprobe kp = {
    .symbol_name = "kallsyms_lookup_name",
};

// const char *(*get_task_state_func)(struct task_struct *tsk);

unsigned long (*kallsyms_lookup_name_func)(const char *name);
void (*sched_setnuma_func)(struct task_struct *p, int node);
int (*migrate_task_to_func)(struct task_struct *p, int cpu);
int (*migrate_swap_func)(struct task_struct *, struct task_struct *);

int (*set_ftrace_filter_func)(char *str);
bool (*trace_ignore_this_task_func)(struct trace_pid_list *filtered_pids, struct trace_pid_list *filtered_no_pids, struct task_struct *task);

static inline void lookup_sched_functions(void)
{
    sched_setnuma_func = (void *)kallsyms_lookup_name_func(SCHED_SETNUMA);
    migrate_task_to_func = (void *)kallsyms_lookup_name_func(MIGRATE_TASK_TO);
    migrate_swap_func = (void *)kallsyms_lookup_name_func(MIGRATE_SWAP);
    // get_task_state_func = (void *)kallsyms_lookup_name_func(GET_TASK_STATE);

    set_ftrace_filter_func = (void *)kallsyms_lookup_name_func(SET_FTRACE_FILTER);


}

static void init_kallsyms(void)
{
    register_kprobe(&kp);
    kallsyms_lookup_name_func = (void *)kp.addr;

    /* Lookup the address for a symbol. Returns 0 if not found. */
    if (kallsyms_lookup_name_func)
    {
        lookup_sched_functions();
    }
    unregister_kprobe(&kp);

    printk("sched_setnuma_func: %p\n", sched_setnuma_func);
    printk("migrate_task_to_func: %p\n", migrate_task_to_func);
    printk("migrate_swap_func: %p\n", migrate_swap_func);
    printk("set_ftrace_filter_func: %p\n", set_ftrace_filter_func);
}

#endif /* _KPROBES_H */