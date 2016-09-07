# Makefile for hello world module
#
PWD := $(shell pwd)
KVERSION := 4.7.0-eudyptula-custom
KERNEL_DIR = /home/jacob/kernel-test/kernel/linux/

MODULE_NAME = hello
obj-m      := $(MODULE_NAME).o
CFLAGS_$(MODULE_NAME).o := -DDEBUG


all:
	make -C $(KERNEL_DIR) M=$(PWD) modules
clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean
