/* void ftrace_callback_func(unsigned long ip, unsigned long parent_ip, struct ftrace_ops *op, struct ftrace_regs *regs)
{
    pr_info("task_tick() hook");

    int ret = ftrace_set_filter(&ops, "task_tick", strlen("task_tick"), 0);
    register_ftrace_function(&ops);
}

static struct ftrace_ops ops = {
    .func = ftrace_callback_func,
    .flags = NULL,
    .private = NULL,
}; */