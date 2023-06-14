#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/kallsyms.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/sched.h>

#include <linux/perf_event.h>

#define MAX_CPUS 1024
#define MAX_PACKAGES 16
#define PID_NEGATIVE_ONE -1

#define NUM_RAPL_DOMAINS 1
#define TOTAL_PACKAGES 1
#define TOTAL_CORES 16
#define TYPE 12
#define CONFIG 0x02
#define SCALE 2.3283064365386962890625e-10
#define UNIT "Joule"

#define KALLSYMS_LOOKUP_NAME "kallsyms_lookup_name"
#define SYS_CALL_TABLE "sys_call_table"

typedef long (*sys_call_ptr_t)(const struct pt_regs*);
typedef unsigned long (*kln)(const char *name);
kln _kallsyms_lookup_name = NULL;

static struct kprobe kp = {
    .symbol_name = KALLSYMS_LOOKUP_NAME
};

static void kln_init(void)
{
    register_kprobe(&kp);
    if (!kp.addr)
    {
        return;
    }

    _kallsyms_lookup_name = (kln) kp.addr;
    sys_call_ptr_t **syscall_table = (sys_call_ptr_t**) _kallsyms_lookup_name(SYS_CALL_TABLE);

    syscall_table[__NR_perf_event_open](.)



    // pr_alert("sys_call_table@0x%px\n", syscall_table);
}

static int __init energy_init(void)
{
    pr_info("detect-process installed\n");
    kln_init();
    return 0;
}

static void __exit energy_exit(void)
{
    unregister_kprobe(&kp);
    pr_info("detect-process uninstalled\n");
}

MODULE_AUTHOR("Dipanzan Islam <dipanzan@live.com>");
MODULE_DESCRIPTION("Kernel module for measuring energy consumption of process/thread");
MODULE_LICENSE("GPL");

module_init(energy_init);
module_exit(energy_exit);
