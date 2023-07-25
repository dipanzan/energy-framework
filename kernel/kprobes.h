#ifndef _KPROBES_H
#define _KPROBES_H

#define KALLSYMS_LOOKUP_NAME "kallsyms_lookup_name"

struct kprobe kp = {
    .symbol_name = KALLSYMS_LOOKUP_NAME,
};

unsigned long (*kallsyms_lookup_name_func)(const char *name);

static int init_kallsyms(void)
{
    kallsyms_lookup_name_func = (void *)kp.addr;
    if (kallsyms_lookup_name_func)
    {
        return 0;
    }
    else
    {
        pr_alert("%s(): init failed.\n", KALLSYMS_LOOKUP_NAME);
        return -1;
    }
}

static int init_kprobe(void)
{
    int ret = 0;
    ret |= register_kprobe(&kp);
    ret |= init_kallsyms();
    return ret;
}

static void release_kprobe(void)
{
    unregister_kprobe(&kp);
}

#endif /* _KPROBES_H */