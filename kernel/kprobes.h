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
        pr_alert("%s(): init failed. :(\n", KALLSYMS_LOOKUP_NAME);
        return -1;
    }
}

static int release_kprobe(void)
{
    unregister_kprobe(&kp);
    return 0;
}

static int init_kprobe(struct device *dev)
{
    int ret = 0;
    ret |= register_kprobe(&kp);

    if (ret)
    {
        pr_alert("%s(): init failed. :(\n", __FUNCTION__);
        return ret;
    }
    ret |= init_kallsyms();
    if (ret)
    {
        release_kprobe();
        return ret;
    }

    pr_alert("%s(): init complete! :)\n", __FUNCTION__);
    return ret;
}



#endif /* _KPROBES_H */