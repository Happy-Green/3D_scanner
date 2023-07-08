obj-m += vl53l1x.o

all: module dt
	echo Builded Device Tree Overlay and kernel module

module:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
dt: vl53l1x.dts
	dtc -@ -I dts -O dtb -o vl53l1x.dtbo vl53l1x.dts
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf vl53l1x.dtbo
