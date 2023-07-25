#ifndef _LOOKUP_FUNCS_H
#define _LOOKUP_FUNCS_H

// const char *(*get_task_state_func)(struct task_struct *tsk);
void (*sched_setnuma_func)(struct task_struct *p, int node);
int (*migrate_task_to_func)(struct task_struct *p, int cpu);
int (*migrate_swap_func)(struct task_struct *, struct task_struct *);
long (*sched_setaffinity_func)(pid_t pid, const struct cpumask *in_mask);

static void lookup_functions(void)
{
    sched_setnuma_func = (void *)kallsyms_lookup_name_func("sched_setnuma");
    migrate_task_to_func = (void *)kallsyms_lookup_name_func("migrate_task_to");
    migrate_swap_func = (void *)kallsyms_lookup_name_func("migrate_swap");
    sched_setaffinity_func = (void *)kallsyms_lookup_name_func("sched_setaffinity");
}

static void print_available_functions(void)
{
    pr_info("sched_setnuma_func: %p\n", sched_setnuma_func);
    pr_info("migrate_task_to_func: %p\n", migrate_task_to_func);
    pr_info("migrate_swap_func: %p\n", migrate_swap_func);
}
#endif /* _LOOKUP_FUNCS_H */