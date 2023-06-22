#ifndef _FTRACE_H
#define _FTRACE_H

/* https://www.apriorit.com/dev-blog/546-hooking-linux-functions-2s */

#define SCHEDULE_FUNC "schedule"

static void notrace ftrace_callback(unsigned long ip, unsigned long parent_ip, struct ftrace_ops *op, struct ftrace_regs *regs)
{
    // pr_alert("%s() called\n", SCHEDULE_FUNC);
    // pr_alert("ip: %p, parent_ip: %p\n", ip, parent_ip);
}

struct ftrace_ops ops = {
    .func = ftrace_callback,
    .flags = FTRACE_OPS_FL_RECURSION,
    .private = NULL,
};


// ftrace_set_filter() MUST be called before registering ftrace ops!
static void setup_ftrace_filter(void)
{
    ftrace_set_filter(&ops, SCHEDULE_FUNC, strlen(SCHEDULE_FUNC), 0);
}

#endif /* _FTRACE_H */