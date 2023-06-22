#ifndef _LOOKUP_FUNCS_H
#define _LOOKUP_FUNCS_H

// sched functions
#define GET_TASK_STATE "get_task_state"
#define SCHED_SETNUMA "sched_setnuma"
#define MIGRATE_TASK_TO "migrate_task_to"
#define MIGRATE_SWAP "migrate_swap"

// ftrace functions
#define SET_FTRACE_FILTER "set_ftrace_filter"
#define TRACE_IGNORE_THIS_TASK "trace_ignore_this_task"

// const char *(*get_task_state_func)(struct task_struct *tsk);
void (*sched_setnuma_func)(struct task_struct *p, int node);
int (*migrate_task_to_func)(struct task_struct *p, int cpu);
int (*migrate_swap_func)(struct task_struct *, struct task_struct *);

int (*set_ftrace_filter_func)(char *str);
bool (*trace_ignore_this_task_func)(struct trace_pid_list *filtered_pids, struct trace_pid_list *filtered_no_pids, struct task_struct *task);

static void lookup_sched_functions(void)
{
    sched_setnuma_func = (void *)kallsyms_lookup_name_func(SCHED_SETNUMA);
    migrate_task_to_func = (void *)kallsyms_lookup_name_func(MIGRATE_TASK_TO);
    migrate_swap_func = (void *)kallsyms_lookup_name_func(MIGRATE_SWAP);
}

static void print_sched_functions(void)
{
    pr_info("sched_setnuma_func: %p\n", sched_setnuma_func);
    pr_info("migrate_task_to_func: %p\n", migrate_task_to_func);
    pr_info("migrate_swap_func: %p\n", migrate_swap_func);
}

static void lookup_ftrace_functions(void)
{
    // set_ftrace_filter_func = (void *)kallsyms_lookup_name_func(SET_FTRACE_FILTER);
    // trace_ignore_this_task_func = (void *)kallsyms_lookup_name_func(TRACE_IGNORE_THIS_TASK);
}

#endif /* _LOOKUP_FUNCS_H */