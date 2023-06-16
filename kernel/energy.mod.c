#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif


static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x5a5a2271, "__cpu_online_mask" },
	{ 0xaad72ea0, "perf_event_read_value" },
	{ 0x92997ed8, "_printk" },
	{ 0xd0da656b, "__stack_chk_fail" },
	{ 0x4dfa8d4b, "mutex_lock" },
	{ 0xa5e55057, "rdmsrl_safe_on_cpu" },
	{ 0x54b1fac6, "__ubsan_handle_load_invalid_value" },
	{ 0xa648e561, "__ubsan_handle_shift_out_of_bounds" },
	{ 0x3213f038, "mutex_unlock" },
	{ 0xfe0edd20, "pv_ops" },
	{ 0x17de3d5, "nr_cpu_ids" },
	{ 0x7aff77a3, "__cpu_present_mask" },
	{ 0x63c4d61f, "__bitmap_weight" },
	{ 0xb6cb556a, "_find_first_and_bit" },
	{ 0x87a21cb3, "__ubsan_handle_out_of_bounds" },
	{ 0x45d246da, "node_to_cpumask_map" },
	{ 0xabba89f, "perf_event_enable" },
	{ 0x94dbad2, "perf_event_disable" },
	{ 0xb3f7646e, "kthread_should_stop" },
	{ 0x7f02188f, "__msecs_to_jiffies" },
	{ 0x54496b4, "schedule_timeout_interruptible" },
	{ 0xf709cb89, "kthread_stop" },
	{ 0x37dbdae4, "perf_event_release_kernel" },
	{ 0x2800222e, "devm_kmalloc" },
	{ 0xd7b002d, "boot_cpu_data" },
	{ 0x96848186, "scnprintf" },
	{ 0xcefb0c9f, "__mutex_init" },
	{ 0x8b5abc15, "devm_hwmon_device_register_with_info" },
	{ 0xcea381dd, "x86_match_cpu" },
	{ 0xac13d953, "kthread_create_on_node" },
	{ 0xd9b11890, "wake_up_process" },
	{ 0x8096f634, "perf_event_create_kernel_counter" },
	{ 0xe9925143, "__platform_driver_register" },
	{ 0x88ed50ea, "platform_device_alloc" },
	{ 0x66b467be, "platform_driver_unregister" },
	{ 0xcfa688f, "platform_device_add" },
	{ 0x58aa6397, "platform_device_put" },
	{ 0x61901594, "platform_device_unregister" },
	{ 0x4fa8f1f1, "param_ops_int" },
	{ 0xb4b19daa, "param_ops_charp" },
	{ 0x541a6db8, "module_layout" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("cpu:type:x86,ven0002fam0017mod0031:feature:*");
MODULE_ALIAS("cpu:type:x86,ven0002fam0019mod0001:feature:*");
MODULE_ALIAS("cpu:type:x86,ven0002fam0019mod0010:feature:*");
MODULE_ALIAS("cpu:type:x86,ven0002fam0019mod0030:feature:*");
MODULE_ALIAS("cpu:type:x86,ven0002fam0019mod0050:feature:*");
MODULE_ALIAS("platform:kernel_energy_driver");

MODULE_INFO(srcversion, "5C85B1BAAE7556225C30D31");
