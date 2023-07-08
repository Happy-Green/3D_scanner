/**
 *
 * Simplified VL53L1X device driver.
 *
 * Supports multiple instances. Significantly based on multiple drivers included in
 * sources of Linux and driver examples by prof. Wojciech Zabołotny (WEITI PW) | https://github.com/wzab
 * @authors Albert Bogdanovic, Konrad Kacper Domian (WEITI PW)
 * @copyright (C) 2023 by Albert Bogdanovic, Konrad Domian
 * License GPL v2
 * konrad.domian.stud<at>pw.edu.pl | konrad.kacper.domian@gmail.com
 * albert.bogdanovic.stud<at>pw.edu.pl | albertbog@gmail.com
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/idr.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <asm/io.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/kfifo.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>

#define SUCCESS 0
#define DEVICE_NAME "vl53l1x"
#define I2C_BUS_AVAILABLE 1
#define ST_TOF_IOCTL_WFI 1
#define VL53L1X_MAX_DEVICES 3

static struct cdev *my_cdev = NULL;
static struct class VL53L1X_class = {
    .name = "bogdom_VL53L1X_class",
};
static int VL53L1X_major = 0;
static DEFINE_IDR(VL53L1X_idr);
static DEFINE_MUTEX(idr_lock);
static bool VL53L1X_class_registered = 0;
static int intr_ready_flag = -1;

static int VL53L1X_open(struct inode *inode, struct file *file);
static int VL53L1X_release(struct inode *inode, struct file *file);
ssize_t VL53L1X_read(struct file *filp,
                     char __user *buf, size_t count, loff_t *off);
ssize_t VL53L1X_write(struct file *filp,
                      const char __user *buf, size_t count, loff_t *off);
static long VL53L1X_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

/* Queue for reading process */
DECLARE_WAIT_QUEUE_HEAD(readqueue);

struct VL53L1X_device
{
    int irq;
    int minor;
    struct i2c_client *VL53L1X_client;
};

struct file_operations Fops = {
    .owner = THIS_MODULE,
    .read = VL53L1X_read,   /* read */
    .write = VL53L1X_write, /* write */
    .open = VL53L1X_open,
    .release = VL53L1X_release,
    .unlocked_ioctl = VL53L1X_ioctl,
};

static int VL53L1X_get_minor(struct VL53L1X_device *idev)
{
    int retval;

    mutex_lock(&idr_lock);
    retval = idr_alloc(&VL53L1X_idr, idev, 0, VL53L1X_MAX_DEVICES, GFP_KERNEL);
    if (retval >= 0)
    {
        idev->minor = retval;
        retval = 0;
    }
    else if (retval == -ENOSPC)
    {
        dev_err(&idev->VL53L1X_client->dev, "too many VL53L1X devices\n");
        retval = -EINVAL;
    }
    mutex_unlock(&idr_lock);
    return retval;
}

irqreturn_t VL53L1X_irq(int irq, void *dev_id)
{
    intr_ready_flag = 1;
    printk("IRQ handler called\n");
    wake_up_interruptible(&readqueue);
    return IRQ_HANDLED;
};

void VL53L1X_remove(struct i2c_client *client)
{

    struct VL53L1X_device *VL53L1X_dev;
    VL53L1X_dev = i2c_get_clientdata(client);
    dev_info(&client->dev, "Remove function called\n");
    printk(KERN_INFO "REMOVE VL53L1X_dev MKDEV: %d\n", MKDEV(VL53L1X_major, VL53L1X_dev->minor));
    device_destroy(&VL53L1X_class, MKDEV(VL53L1X_major, VL53L1X_dev->minor));
    if (my_cdev)
        cdev_del(my_cdev);
    my_cdev = NULL;
    printk("<1>drv_VL53L1X removed!\n");
}

static int VL53L1X_open(struct inode *inode, struct file *file)
{
    int res = 0;
    struct VL53L1X_device *VL53L1X_dev;
    unsigned long irqflags;

    irqflags = IRQF_TRIGGER_RISING | IRQF_ONESHOT;

    mutex_lock(&idr_lock);
    VL53L1X_dev = idr_find(&VL53L1X_idr, iminor(inode));
    mutex_unlock(&idr_lock);

    nonseekable_open(inode, file);

    res = request_irq(VL53L1X_dev->irq, VL53L1X_irq, irqflags, DEVICE_NAME, VL53L1X_dev);
    if (res)
    {
        printk(KERN_INFO "VL53L1X: I can't connect irq %i error: %d\n", VL53L1X_dev->irq, res);
        VL53L1X_dev->irq = -1;
        return res;
    }

    printk(KERN_INFO "VL53L1X_dev: %lx, inode:%lx, I2c addr: %#8x, irq: %d\n", (unsigned long)VL53L1X_dev, (unsigned long)inode, VL53L1X_dev->VL53L1X_client->addr, VL53L1X_dev->irq);

    file->private_data = VL53L1X_dev;

    return SUCCESS;
}

static int VL53L1X_release(struct inode *inode,
                           struct file *file)
{
    struct VL53L1X_device *VL53L1X_dev;
    mutex_lock(&idr_lock);
    VL53L1X_dev = idr_find(&VL53L1X_idr, iminor(inode));
    mutex_unlock(&idr_lock);
    printk(KERN_INFO "RELEASE VL53L1X_dev: %lx, inode:%lx, irq: %d\n", (unsigned long)VL53L1X_dev, (unsigned long)inode, VL53L1X_dev->irq);
    if (VL53L1X_dev->irq >= 0)
        free_irq(VL53L1X_dev->irq, VL53L1X_dev); // Free interrupt
    return SUCCESS;
}

ssize_t VL53L1X_read(struct file *filp,
                     char __user *buf, size_t count, loff_t *off)
{
    struct VL53L1X_device *dev = filp->private_data;
    struct i2c_client *client = dev->VL53L1X_client;
    int ret;
    static char tmp[1000];

    mutex_lock(&idr_lock);
    ret = i2c_master_recv(client, tmp, count);
    mutex_unlock(&idr_lock);
    if (ret < 0)
    {
        printk(KERN_INFO "%s: i2c_master_recv returned %d\n", __func__, ret);
        return ret;
    }
    if (ret > count)
    {
        printk(KERN_INFO "%s: received too many bytes from i2c (%d)\n",
               __func__, ret);
        return -EIO;
    }

    if (copy_to_user(buf, tmp, ret))
        ret = -EFAULT;

    return ret;
}

ssize_t VL53L1X_write(struct file *filp,
                      const char __user *buf, size_t count, loff_t *off)
{
    struct VL53L1X_device *dev = filp->private_data;
    struct i2c_client *client = dev->VL53L1X_client;
    int ret;
    char *tmp;

    tmp = memdup_user(buf, count);
    if (IS_ERR(tmp))
        return PTR_ERR(tmp);

    mutex_lock(&idr_lock);
    ret = i2c_master_send(client, tmp, count);
    mutex_unlock(&idr_lock);
    if (ret < 0)
    {
        printk(KERN_INFO "%s: write fail!\n\r", __func__);
    }
    kfree(tmp);
    return ret;
}

static long VL53L1X_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int res;
    switch (cmd)
    {
    case ST_TOF_IOCTL_WFI:

        // Interrupts are on, so we should sleep and wait for interrupt
        printk("IOCTL handler called\n");
        res = wait_event_interruptible(readqueue, intr_ready_flag != 0);
        intr_ready_flag = 0;
        if (res)
            return res; // Signal received!
        break;

    default:
        return -EINVAL;
    }
    return 0;
}

static int VL53L1X_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
    int res = 0;
    struct VL53L1X_device *VL53L1X_dev = NULL;
    // check if class already registered
    if (!VL53L1X_class_registered)
        return -EPROBE_DEFER;
    // allocating device
    VL53L1X_dev = kzalloc(sizeof(*VL53L1X_dev), GFP_KERNEL);
    if (!VL53L1X_dev)
        return -ENOMEM;
    // get minor
    res = VL53L1X_get_minor(VL53L1X_dev);
    if (res)
    {
        kfree(VL53L1X_dev);
        return res;
    }
    // get IRQ for device
    VL53L1X_dev->irq = irq_of_parse_and_map(client->dev.of_node, 0);
    if (VL53L1X_dev->irq < 0)
    {
        printk(KERN_ERR "Error reading the IRQ number: %d.\n", VL53L1X_dev->irq);
        res = VL53L1X_dev->irq;
        goto err1;
    }
    printk(KERN_ALERT "Connected IRQ=%d\n", VL53L1X_dev->irq);

    i2c_set_clientdata(client, VL53L1X_dev);
    VL53L1X_dev->VL53L1X_client = client;

    struct device *VL53L1X_device; // to check if device was created successfully
    VL53L1X_device = device_create(&VL53L1X_class, NULL, MKDEV(VL53L1X_major, VL53L1X_dev->minor), NULL, "vl53l1x_%d", VL53L1X_dev->minor);
    if (VL53L1X_device == NULL)
    {
        dev_err(&client->dev, "Failed to create device\n");
        return 1;
    }

    dev_info(&client->dev, "%s The major device number is %d.\n",
             "Successful registration.",
             VL53L1X_major);
    return 0;

err1:
    VL53L1X_remove(client);
    return res;
}

static struct of_device_id VL53L1X_driver_ids[] = {
    {
        .compatible = "st,vl53l1x",
    },
    {},
};
MODULE_DEVICE_TABLE(of, VL53L1X_driver_ids);

static struct i2c_device_id my_VL53L1X[] = {
    {DEVICE_NAME, 0},
    //      { DEVICE_NAME, 1 },
    //      { DEVICE_NAME, 2 },
    {}};
MODULE_DEVICE_TABLE(i2c, my_VL53L1X);

struct i2c_driver VL53L1X_driver = {
    .driver = {
        .name = DEVICE_NAME,
        .of_match_table = VL53L1X_driver_ids,
    },
    .probe = VL53L1X_probe,
    .remove = VL53L1X_remove,
    .id_table = my_VL53L1X,

};

static int VL53L1X_major_init(void)
{
    struct cdev *cdev = NULL;
    dev_t VL53L1X_dev = 0;
    int result;

    result = alloc_chrdev_region(&VL53L1X_dev, 0, VL53L1X_MAX_DEVICES, DEVICE_NAME);
    if (result)
        goto out;

    result = -ENOMEM;
    cdev = cdev_alloc();
    if (!cdev)
        goto out_unregister;

    cdev->owner = THIS_MODULE;
    cdev->ops = &Fops;

    result = cdev_add(cdev, VL53L1X_dev, VL53L1X_MAX_DEVICES);
    if (result)
        goto out_put;

    VL53L1X_major = MAJOR(VL53L1X_dev);
    printk(KERN_INFO "VL53L1X_major: %d, dev_t:%lx\n", VL53L1X_major, (unsigned long)VL53L1X_dev);
    my_cdev = cdev;
    return 0;
out_put:
    kobject_put(&cdev->kobj);
out_unregister:
    unregister_chrdev_region(VL53L1X_dev, VL53L1X_MAX_DEVICES);
out:
    return result;
}

static void VL53L1X_major_cleanup(void)
{
    if (VL53L1X_major)
    {
        unregister_chrdev_region(MKDEV(VL53L1X_major, 0), VL53L1X_MAX_DEVICES);
        if (my_cdev)
            cdev_del(my_cdev);
        VL53L1X_major = 0;
    }
}

static void release_VL53L1X_class(void)
{
    if (VL53L1X_class_registered)
    {
        VL53L1X_class_registered = false;
        class_unregister(&VL53L1X_class);
    };
    VL53L1X_major_cleanup();
}

static int init_VL53L1X_classes(void)
{
    int ret;
    ret = VL53L1X_major_init();
    if (ret)
        goto exit;

    ret = class_register(&VL53L1X_class);
    if (ret)
    {
        printk(KERN_ERR "class_register failed for VL53L1X\n");
        goto exit;
    }
    VL53L1X_class_registered = true;
    return 0;

exit:
    release_VL53L1X_class();
    return ret;
}

static int VL53L1X_init(void)
{
    int res;
    res = init_VL53L1X_classes();
    if (res < 0)
    {
        printk(KERN_ERR "Failed to register classes: %d\n", res);
        return res;
    }
    res = i2c_add_driver(&VL53L1X_driver);
    if (res < 0)
    {
        printk(KERN_ERR "Failed to register my VL53L1X driver: %d\n", res);
        return res;
    }
    printk(KERN_INFO "VL53L1X module added\n");
    return 0;
}

static void VL53L1X_exit(void)
{
    i2c_del_driver(&VL53L1X_driver);
    release_VL53L1X_class();
    idr_destroy(&VL53L1X_idr);
    printk(KERN_INFO "VL53L1X module says goodbye\n");
}

module_init(VL53L1X_init);
module_exit(VL53L1X_exit);

MODULE_AUTHOR("Albert Bogdanovic & Konrad Domian");
MODULE_DESCRIPTION("ST VL53L1X multi sensor driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
