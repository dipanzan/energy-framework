#ifndef _CPU_INFO_H
#define _CPU_INFO_H

static unsigned int num_siblings_per_core(void)
{
	unsigned int siblings = ((cpuid_ebx(0x8000001E) >> 8) & 0xFF) + 1;
#if DEBUG
	{
		pr_alert("%s: %u\n", __FUNCTION__, siblings);
	}
#endif
	return siblings;
}

static unsigned int get_core_cpu_count(void)
{
	/*
	 * Energy counter register is accessed at core level.
	 * Hence, filterout the siblings.
	 */
	return num_present_cpus() / num_siblings_per_core();
}

static unsigned int get_socket_count(void)
{
	struct cpuinfo_x86 *info = &boot_cpu_data;

	/*
	 * c->x86_max_cores is the linux count of physical cores
	 * total physical cores / core per socket gives total number of sockets.
	 */
	return get_core_cpu_count() / info->x86_max_cores;
}

typedef struct cpuid_leaf
{
	uint32_t a, b, c, d;
} leaf_t;

static void dump_cpu_info(void)
{
	leaf_t
		leaf0 = {0, 0, 0, 0},		 /* Highest Value for Basic Processor Information and the Vendor Identification String */
		leaf1 = {0, 0, 0, 0},		 /* Model, Family, Stepping Information */
		leaf6 = {0, 0, 0, 0},		 /* Thermal and Power Management Features */
		leafA = {0, 0, 0, 0},		 /* Architectural Performance Monitoring Features */
		leaf80000000 = {0, 0, 0, 0}, /* Highest Value for Extended Processor Information */
		leaf80000001 = {0, 0, 0, 0}, /* Extended Processor Signature & Feature Bits */
		leaf80000002 = {0, 0, 0, 0}, /* Processor Brand String 0 */
		leaf80000003 = {0, 0, 0, 0}, /* Processor Brand String 1 */
		leaf80000004 = {0, 0, 0, 0}; /* Processor Brand String 2 */

	/* perform all cpuid reads. */
	cpuid_count(0x00000000, 0, &leaf0.a, &leaf0.b, &leaf0.c, &leaf0.d);
	cpuid_count(0x00000001, 0, &leaf1.a, &leaf1.b, &leaf1.c, &leaf1.d);
	cpuid_count(0x00000006, 0, &leaf6.a, &leaf6.b, &leaf6.c, &leaf6.d);
	cpuid_count(0x0000000A, 0, &leafA.a, &leafA.b, &leafA.c, &leafA.d);
	cpuid_count(0x80000000, 0, &leaf80000000.a, &leaf80000000.b, &leaf80000000.c, &leaf80000000.d);
	cpuid_count(0x80000001, 0, &leaf80000001.a, &leaf80000001.b, &leaf80000001.c, &leaf80000001.d);
	cpuid_count(0x80000002, 0, &leaf80000002.a, &leaf80000002.b, &leaf80000002.c, &leaf80000002.d);
	cpuid_count(0x80000003, 0, &leaf80000003.a, &leaf80000003.b, &leaf80000003.c, &leaf80000003.d);
	cpuid_count(0x80000004, 0, &leaf80000004.a, &leaf80000004.b, &leaf80000004.c, &leaf80000004.d);

	unsigned family = (leaf1.a >> 8) & 0x0F;
	unsigned model = (leaf1.a >> 4) & 0x0F;
	unsigned stepping = (leaf1.a >> 0) & 0x0F;
	unsigned exmodel = (leaf1.a >> 16) & 0x0F;
	unsigned exfamily = (leaf1.a >> 20) & 0xFF;
	unsigned dispFamily = (family != 0x0F) ? family : (family + exfamily);
	unsigned dispModel = (family == 0x06 || family == 0x0F) ? (exmodel << 4 | model) : model;

	uint32_t maxLeaf = leaf0.a;
	uint32_t maxExtendedLeaf = leaf80000000.a;
	char procBrandString[49] = {0};

	memcpy(&procBrandString[0], (const char *)&leaf80000002, 16);
	memcpy(&procBrandString[16], (const char *)&leaf80000003, 16);
	memcpy(&procBrandString[32], (const char *)&leaf80000004, 16);
	if (maxExtendedLeaf < 4)
	{
		strcpy(procBrandString, "(unknown)");
	}

	/* And dump the CPUID info to the ring buffer for debug purposes. */
	if (maxLeaf >= 1)
	{
		printk(KERN_INFO "Kernel Module loading on processor %s (Family %u (%X), Model %u (%03X), Stepping %u (%X))\n",
			   procBrandString, dispFamily, dispFamily, dispModel, dispModel, stepping, stepping);
	}
	else
	{
		printk(KERN_INFO "Kernel Module loading on processor %s\n", procBrandString);
	}
	printk(KERN_INFO "cpuid.0x0.0x0:        EAX=%08x, EBX=%08x, ECX=%08x, EDX=%08x\n",
		   leaf0.a, leaf0.b, leaf0.c, leaf0.d);
	printk(KERN_INFO "cpuid.0x1.0x0:        EAX=%08x, EBX=%08x, ECX=%08x, EDX=%08x\n",
		   leaf1.a, leaf1.b, leaf1.c, leaf1.d);
	printk(KERN_INFO "cpuid.0x6.0x0:        EAX=%08x, EBX=%08x, ECX=%08x, EDX=%08x\n",
		   leaf6.a, leaf6.b, leaf6.c, leaf6.d);
	printk(KERN_INFO "cpuid.0xA.0x0:        EAX=%08x, EBX=%08x, ECX=%08x, EDX=%08x\n",
		   leafA.a, leafA.b, leafA.c, leafA.d);
	printk(KERN_INFO "cpuid.0x80000000.0x0: EAX=%08x, EBX=%08x, ECX=%08x, EDX=%08x\n",
		   leaf80000000.a, leaf80000000.b, leaf80000000.c, leaf80000000.d);
	printk(KERN_INFO "cpuid.0x80000001.0x0: EAX=%08x, EBX=%08x, ECX=%08x, EDX=%08x\n",
		   leaf80000001.a, leaf80000001.b, leaf80000001.c, leaf80000001.d);
	printk(KERN_INFO "cpuid.0x80000002.0x0: EAX=%08x, EBX=%08x, ECX=%08x, EDX=%08x\n",
		   leaf80000002.a, leaf80000002.b, leaf80000002.c, leaf80000002.d);
	printk(KERN_INFO "cpuid.0x80000003.0x0: EAX=%08x, EBX=%08x, ECX=%08x, EDX=%08x\n",
		   leaf80000003.a, leaf80000003.b, leaf80000003.c, leaf80000003.d);
	printk(KERN_INFO "cpuid.0x80000004.0x0: EAX=%08x, EBX=%08x, ECX=%08x, EDX=%08x\n",
		   leaf80000004.a, leaf80000004.b, leaf80000004.c, leaf80000004.d);

	/* Begin sanity checks. */
	if (maxLeaf < 0xA)
	{
		printk(KERN_ERR "ERROR: Processor too old!\n");
	}

	/* Check that CPU has performance monitoring. */
	if (((leaf1.c >> 15) & 1) == 0)
	{
		printk(KERN_ERR "ERROR: Processor does not have Perfmon and Debug Capability!\n");
	}
}

#endif /* _CPU_INFO_H */