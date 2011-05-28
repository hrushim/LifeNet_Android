NEW_KERNEL=1
EXTRA_CFLAGS += -DNEW_KERNEL=$(NEW_KERNEL)

DEBUG_ON=0
EXTRA_CFLAGS += -DDEBUG_ON=$(DEBUG_ON)

ifdef MYMANET_ALPHA
EXTRA_CFLAGS += -DMYMANET_ALPHA=$(MYMANET_ALPHA)
endif

ifdef MYMANET_BETA
EXTRA_CFLAGS += -DMYMANET_BETA=$(MYMANET_BETA)
endif

MYMANET_KERNEL_VERSION_6_30_PLUS=1
EXTRA_CFLAGS += -DMYMANET_KERNEL_VERSION_6_30_PLUS=$(MYMANET_KERNEL_VERSION_6_30_PLUS)

MYMANET_STORE_PATH=1
EXTRA_CFLAGS += -DMYMANET_STORE_PATH=$(MYMANET_STORE_PATH)

obj-m += manifold.o
manifold-objs := main_module.o string_functions.o proc_functions.o distance_list_functions.o stat_list_functions.o timer_functions.o timestamp_list_functions.o path_list_functions.o

all:
	make -C /home/hrushi/Code/android/source2 M=$(PWD) modules

clean:
	make -C /home/hrushi/Code/android/source2 M=$(PWD) clean
	rm -f *.ko
	rm -f *.o
	rm -f *.mod.*
	rm -f Module.symvers
	rm -f Module.markers
	rm -f modules.order

