obj-m := servoDriver.o

SRC := $(shell pwd)

KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build/

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules
	$(shell dtc -@ -I dts -O dtb -o servooverlay.dtbo servooverlay.dts)
	
modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules_install

clean:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.c *.mod *.dtbo
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.symvers
