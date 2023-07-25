#ifndef _FTRACE_H
#define _FTRACE_H

static void (*__fire_sched_in_preempt_notifiers_real)(struct task_struct *curr);

static void __fire_sched_in_preempt_notifiers_fh(struct task_struct *curr)
{
    pr_alert("YAAAY\n");
    __fire_sched_in_preempt_notifiers_real(curr);
}

static pid_t (*user_mode_thread_real)(int (*fn)(void *), void *arg, unsigned long flags);

static pid_t user_mode_thread_fh(int (*fn)(void *), void *arg, unsigned long flags)
{
    pid_t pid = user_mode_thread_real(fn, arg, flags);
    pr_alert("%s(): %d\n", __FUNCTION__, pid);
    return pid;
}

static asmlinkage void (*schedule_real)(void);

static asmlinkage void schedule_fh(void)
{
    pr_alert("schedule-in\n");
    schedule_real();
    pr_alert("schedule-out\n");
}

/* https://www.apriorit.com/dev-blog/546-hooking-linux-funhttps://www.apriorit.com/dev-blog/546-hooking-linux-functions-2-actions-2 */

#define HOOK(_name, _original, _function)    \
    {                                        \
        .name = (_name),                     \
        .original = (_original),             \
        .function = (_function),          \ 
      \
    }

struct ftrace_hook
{
    const char *name;
    void *original;
    void *function;

    unsigned long address;
    struct ftrace_ops ops;
};

static void notrace fh_ftrace_thunk(unsigned long ip, unsigned long parent_ip, struct ftrace_ops *ops, struct ftrace_regs *regs)
{
    struct ftrace_hook *hook = container_of(ops, struct ftrace_hook, ops);
    pr_alert("thunk: %s\n", hook->name);

    // do not trace if function got invoked from the module itself
    // can happen with global functions like schedule()
    if (!within_module(parent_ip, THIS_MODULE))
    {
        regs->regs.ip = (unsigned long)hook->function;
    }
}

static int resolve_hook_address(struct ftrace_hook *hook)
{
    hook->address = kallsyms_lookup_name_func(hook->name);
    if (!hook->address)
    {
        pr_alert("unresolved symbol %s\n", hook->name);
        return -ENOENT;
    }

    pr_alert("%s(): %lx found.\n", hook->name, hook->address);
    *((unsigned long *)hook->original) = hook->address;
    return 0;
}

static int fh_install_hook(struct ftrace_hook *hook)
{
    int err;
    err = resolve_hook_address(hook);
    if (err)
    {
        return err;
    }
    hook->ops.func = fh_ftrace_thunk;
    hook->ops.flags = FTRACE_OPS_FL_SAVE_REGS_IF_SUPPORTED | FTRACE_OPS_FL_IPMODIFY; // TODO: recursion?

    err = ftrace_set_filter_ip(&hook->ops, hook->address, 0, 1);
    pr_alert("ftrace_set_filter_ip: %d\n", err);
    if (err)
    {
        pr_alert("ftrace_set_filter_ip() failed: %d\n", err);
        return err;
    }

    err = register_ftrace_function(&hook->ops);
    pr_alert("register_ftrace_function: %d\n", err);

    if (err)
    {
        pr_alert("register_ftrace_function() failed: %d\n", err);

        // disable ftrace filter
        ftrace_set_filter_ip(&hook->ops, hook->address, 1, 0);
        return err;
    }
    return 0;
}

static void fh_remove_hook(struct ftrace_hook *hook)
{
    int err;
    err = unregister_ftrace_function(&hook->ops);
    if (err)
    {
        pr_alert("unregister_ftrace_function() failed: %d\n", err);
    }
    err = ftrace_set_filter(&hook->ops, hook->address, 1, 0);
    if (err)
    {
        pr_alert("ftrace_set_filter_ip() failed: %d\n", err);
    }
}
/*
 * It’s a pointer to the original system call handler execve().
 * It can be called from the wrapper. It’s extremely important to keep the function signature
 * without any changes: the order, types of arguments, returned value,
 * and ABI specifier (pay attention to “asmlinkage”).
 */
static asmlinkage long (*real_sys_execve)(const char __user *filename,
                                          const char __user *const __user *argv,
                                          const char __user *const __user *envp);

/*
 * This function will be called instead of the hooked one. Its arguments are
 * the arguments of the original function. Its return value will be passed on to
 * the calling function. This function can execute arbitrary code before, after,
 * or instead of the original function.
 */
static asmlinkage long fh_sys_execve(const char __user *filename,
                                     const char __user *const __user *argv,
                                     const char __user *const __user *envp)
{
    long ret;

    pr_alert("execve() called: filename=%p argv=%p envp=%pn", filename, argv, envp);
    ret = real_sys_execve(filename, argv, envp);
    pr_alert("execve() returns: %ldn", ret);

    return ret;
}

static char *function_hook = "user_mode_thread";

static void notrace ftrace_callback(unsigned long ip, unsigned long parent_ip, struct ftrace_ops *op, struct ftrace_regs *regs)
{
    pr_alert("%s(): ip: %p, parent_ip: %p\n", __FUNCTION__, ip, parent_ip);
    pr_alert("%s(): %s\n", __FUNCTION__, function_hook, ip, parent_ip);

    if (!within_module(parent_ip, THIS_MODULE))
    {
        pr_alert("%s(): %s\n", __FUNCTION__, function_hook, ip, parent_ip);
    }
}

struct ftrace_ops f_ops = {
    .func = ftrace_callback,
    .flags = FTRACE_OPS_FL_SAVE_REGS_IF_SUPPORTED | FTRACE_OPS_FL_IPMODIFY | FTRACE_OPS_FL_RECURSION,
    .private = NULL,
};

// ftrace_set_filter() MUST be called before registering ftrace ops!
static void setup_ftrace_filter(void)
{
    int ret = ftrace_set_filter(&f_ops, function_hook, strlen(function_hook), 0);
    pr_alert("%s(): ret: %d\n", __FUNCTION__, ret);
}

#endif /* _FTRACE_H */