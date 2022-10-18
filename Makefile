obj-m += config-device.o

PWD := $(CURDIR)

all: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

install:
	insmod ./config-device.ko

uninstall:
	rmmod config_device

logs:
	journalctl --since "1 hour ago" | grep "config-device"

kernel_logs:
	journalctl --since "1 hour ago" | grep "kernel"