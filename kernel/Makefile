# SPDX-License-Identifier: GPL-2.0
#
# Makefile for kernel energy driver

# If KDIR is not specified, assume the development source link
# is in the modules directory for the running kernel
KDIR ?= /lib/modules/`uname -r`/build
MODULE_NAME=energy

CC:=x86_64-linux-gnu-gcc
CFLAGS_energy.o:=-I$(src)

mode:=-1
pid:=-1

default:
	export CONFIG_SENSOR_AMD_ENERGY=m;	\
	$(MAKE) CC=$(CC) -C $(KDIR) M=$$PWD modules

modules: default

modules_install:
	$(MAKE) -C $(KDIR) M=$$PWD modules_install

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean

init:
	sudo insmod $(MODULE_NAME).ko mode=0 multi=0

initp:
	sudo insmod $(MODULE_NAME).ko mode=1 multi=0

initpmt:
	sudo insmod $(MODULE_NAME).ko mode=1 multi=1

initpr:
	sudo insmod $(MODULE_NAME).ko mode=0 pid=$(pid)

dinit:
	sudo rmmod $(MODULE_NAME).ko

dmesg:
	sudo dmesg --clear
	sudo dmesg -w

help:
	@echo "\nThe following make targets are supported:\n"
	@echo "default\t\tBuild the driver module (or if no make target is supplied)"
	@echo "modules\t\tSame as default"
	@echo "modules_install\tBuild and install the driver module"
	@echo "clean"
	@echo

.PHONY: default modules modules_install clean help