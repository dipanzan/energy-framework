#ifndef _LOOKUP_FUNCS_H
#define _LOOKUP_FUNCS_H

// const char *(*get_task_state_func)(struct task_struct *tsk);
void (*sched_setnuma_func)(struct task_struct *p, int node);
int (*migrate_task_to_func)(struct task_struct *p, int cpu);
int (*migrate_swap_func)(struct task_struct *, struct task_struct *);
long (*sched_setaffinity_func)(pid_t pid, const struct cpumask *in_mask);
int (*perf_event_read_func)(struct perf_event *event, bool group);

atomic_t *perf_sched_count_var;
struct static_key_false *perf_sched_events_var;
struct mutex *perf_sched_mutex_var;

// WARNING: extremely unsafe, you
static void lookup_vars(void)
{
    perf_sched_count_var = (void *)kallsyms_lookup_name_func("perf_sched_count");
    perf_sched_events_var = (void *)kallsyms_lookup_name_func("perf_sched_events");
    perf_sched_mutex_var = (void *)kallsyms_lookup_name_func("perf_sched_mutex");
}
static void lookup_functions(void)
{
    sched_setnuma_func = (void *)kallsyms_lookup_name_func("sched_setnuma");
    migrate_task_to_func = (void *)kallsyms_lookup_name_func("migrate_task_to");
    migrate_swap_func = (void *)kallsyms_lookup_name_func("migrate_swap");
    sched_setaffinity_func = (void *)kallsyms_lookup_name_func("sched_setaffinity");
    perf_event_read_func = (void *)kallsyms_lookup_name_func("perf_event_read");

    if (perf_event_read_func)
    {
        pr_alert("perf_event_read_func FOUND: %p\n", perf_event_read_func);
    }
}

static void print_available_functions(void)
{
    pr_info("sched_setnuma_func: %p\n", sched_setnuma_func);
    pr_info("migrate_task_to_func: %p\n", migrate_task_to_func);
    pr_info("migrate_swap_func: %p\n", migrate_swap_func);
}
#endif /* _LOOKUP_FUNCS_H */