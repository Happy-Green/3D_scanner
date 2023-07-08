/**
 *
 * Linux driver for AR360HB3 360 degree servo
 *
 * Significantly based on multiple drivers included in
 * sources of Linux and driver examples by prof. Wojciech Zabołotny (WEITI PW) | https://github.com/wzab
 * @authors Konrad Kacper Domian, Albert Bogdanovic (WEITI PW)
 * @copyright (C) 2023 by Konrad Domian & Albert Bogdanovic
 * License GPL v2
 * konrad.domian.stud<at>pw.edu.pl | konrad.kacper.domian@gmail.com
 * albert.bogdanovic.stud<at>pw.edu.pl
 */

/**Reads libs*/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mod_devicetable.h>
#include <linux/property.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/pwm.h>
#include <linux/proc_fs.h>
#include <linux/string.h>

/**Meta Information*/
MODULE_AUTHOR("Konrad Domian & Albert Bogdanovic");
MODULE_DESCRIPTION("PWM 360 degree Servo driver with irq when servo starts");
MODULE_LICENSE("GPL v2");
/**Meta Information*/

/**
 * @todo move to h file
 */
#define MAX_DEVICES 2

#define IS_DIGIT(char) \
    (char - '0' >= 0 && char - '0' <= 9)

#define MAP_MOVE(str, val)             \
    if (strcmp(inputString, str) == 0) \
    return val

enum MOVE
{
    STOP = 0,
    SLOW = 1,
    FAST = 2
};

enum MOVE mapStringToMove(const char *inputString)
{
    MAP_MOVE("STOP", STOP);
    MAP_MOVE("SLOW", SLOW);
    MAP_MOVE("FAST", FAST);

    return -1;
}

enum LABELS
{
    servo_pwm_xy = 0,
    servo_pwm_z = 1
};

enum LABELS mapStringToLabels(const char *inputString)
{
    MAP_MOVE("servo_pwm_xy", servo_pwm_xy);
    MAP_MOVE("servo_pwm_z", servo_pwm_z);

    return -1;
}
/**MOVE TO .h FILE*/

/**Declate the probe and remove fun*/
static int dt_probe(struct platform_device *pdev);
static int dt_remove(struct platform_device *pdev);

static struct of_device_id servo_driver_ids[] = {
    {
        .compatible = "happygreen, servo",
    },
    {/* sentinel */}};
MODULE_DEVICE_TABLE(of, servo_driver_ids);

static struct platform_driver servo_driver = {
    .probe = dt_probe,
    .remove = dt_remove,
    .driver = {
        .name = "servo_device_driver",
        .of_match_table = servo_driver_ids,
    },
};

struct servoDriver
{
    struct pwm_device *pwm;
    const char *label;
    unsigned int pwm_channel, pwm_freq;
    unsigned int pwm_duty_cycle_stop, pwm_duty_cycle_slow_move, pwm_duty_cycle_fast_move;
    struct proc_dir_entry *proc_file;
};

static struct servoDriver devices[MAX_DEVICES];
/**
 * @brief Write data to buffer
 */
static ssize_t my_write(struct file *File, const char *user_buffer, size_t count, loff_t *offs)
{
    int move = 0;
    char temp_str[4];
    static struct servoDriver device;
    printk("write_servo - Now I am in the my_write func\n");
    if (IS_DIGIT(user_buffer[0]))
        move = user_buffer[0] - '0';
    else
    {
        strncpy(temp_str, user_buffer, 4);
        move = mapStringToMove(temp_str);
    }

    switch (mapStringToLabels(File->f_path.dentry->d_name.name))
    {
    case 0:
        device = devices[0];
        break;
    case 1:
        device = devices[1];
        break;
    default:
        printk("write_servo - Error unkown device label\n");
        break;
    }

    switch (move)
    {
    case 0:
        printk("write_servo - Stop servo\n");
        pwm_config(device.pwm, device.pwm_duty_cycle_stop, device.pwm_freq);
        break;
    case 1:
        printk("write_servo - Move Slow servo\n");
        pwm_config(device.pwm, device.pwm_duty_cycle_slow_move, device.pwm_freq);
        break;
    case 2:
        printk("write_servo - Move Fast servo\n");
        pwm_config(device.pwm, device.pwm_duty_cycle_fast_move, device.pwm_freq);
        break;

    default:
        printk("write_servo - Not corrent data\n");
        break;
    }

    return count;
}

static struct proc_ops fops = {
    .proc_write = my_write,
};

/**
 * @brief This fun is called, when the module is loading into the kernel
 */
static int dt_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    const char *label, *temp_str;
    int ret;
    unsigned int pwm_channel, pwm_freq;
    unsigned int pwm_duty_cycle_stop, pwm_duty_cycle_slow_move, pwm_duty_cycle_fast_move, servo_move;
    static struct servoDriver device;

    printk("dt_servo - Now I am in the probe fun\n");

    // check for device properies
    if (!device_property_present(dev, "label"))
    {
        printk("dt_servo - Error! Device propoerty 'label' not found!\n");
        return -1;
    }
    if (!device_property_present(dev, "pwm_channel"))
    {
        printk("dt_servo - Error! Device propoerty 'pwm_channel' not found!\n");
        return -1;
    }
    if (!device_property_present(dev, "pwm_freq"))
    {
        printk("dt_servo - Error! Device propoerty 'pwm_freq' not found!\n");
        return -1;
    }
    if (!device_property_present(dev, "pwm_duty_cycle_stop"))
    {
        printk("dt_servo - Error! Device propoerty 'pwm_duty_cycle_stop' not found!\n");
        return -1;
    }
    if (!device_property_present(dev, "pwm_duty_cycle_slow_move"))
    {
        printk("dt_servo - Error! Device propoerty 'pwm_duty_cycle_slow_move' not found!\n");
        return -1;
    }
    if (!device_property_present(dev, "pwm_duty_cycle_fast_move"))
    {
        printk("dt_servo - Error! Device propoerty 'pwm_duty_cycle_fast_move' not found!\n");
        return -1;
    }
    if (!device_property_present(dev, "servo_move"))
    {
        printk("dt_servo - Error! Device propoerty 'servo_move' not found!\n");
        return -1;
    }

    // read device properties
    ret = device_property_read_string(dev, "label", &label);
    if (ret)
    {
        printk("dt_servo - Error! Could not read 'label'");
        return -1;
    }
    printk("dt_servo - label: %s\n", label);
    ret = device_property_read_u32(dev, "pwm_channel", &pwm_channel);
    if (ret)
    {
        printk("dt_servo - Error! Could not read 'pwm_channel'");
        return -1;
    }
    printk("dt_servo - pwm_channel: %d\n", pwm_channel);
    ret = device_property_read_u32(dev, "pwm_freq", &pwm_freq);
    if (ret)
    {
        printk("dt_servo - Error! Could not read 'pwm_freq'");
        return -1;
    }
    printk("dt_servo - pwm_freq: %d\n", pwm_freq);
    ret = device_property_read_u32(dev, "pwm_duty_cycle_stop", &pwm_duty_cycle_stop);
    if (ret)
    {
        printk("dt_servo - Error! Could not read 'pwm_duty_cycle_stop'");
        return -1;
    }
    printk("dt_servo - pwm_duty_cycle_stop: %d\n", pwm_duty_cycle_stop);
    ret = device_property_read_u32(dev, "pwm_duty_cycle_slow_move", &pwm_duty_cycle_slow_move);
    if (ret)
    {
        printk("dt_servo - Error! Could not read 'pwm_duty_cycle_slow_move'");
        return -1;
    }
    printk("dt_servo - pwm_duty_cycle_slow_move: %d\n", pwm_duty_cycle_slow_move);
    ret = device_property_read_u32(dev, "pwm_duty_cycle_fast_move", &pwm_duty_cycle_fast_move);
    if (ret)
    {
        printk("dt_servo - Error! Could not read 'pwm_duty_cycle_fast_move'");
        return -1;
    }
    printk("dt_servo - pwm_duty_cycle_fast_move: %d\n", pwm_duty_cycle_fast_move);
    ret = device_property_read_string(dev, "servo_move", &temp_str);
    if (ret)
    {
        printk("dt_servo - Error! Could not read 'servo_move'");
        return -1;
    }
    servo_move = mapStringToMove(temp_str);
    if (servo_move == -1)
    {
        printk("dt_servo - Error! Not correct 'servo_move'");
        return -1;
    }
    printk("dt_servo - servo_move: %d\n", servo_move);

    device.label = label;
    device.pwm_channel = pwm_channel;
    device.pwm_duty_cycle_stop = pwm_duty_cycle_stop;
    device.pwm_freq = pwm_freq;
    device.pwm_duty_cycle_slow_move = pwm_duty_cycle_slow_move;
    device.pwm_duty_cycle_fast_move = pwm_duty_cycle_fast_move;

    // Config PWMs
    sprintf(temp_str, "%s", label);

    device.pwm = pwm_request(device.pwm_channel, temp_str);
    if (IS_ERR(device.pwm))
    {
        printk("PWM - Failed to request\n");
        return PTR_ERR(device.pwm);
    }
    printk("PWM - Succesful request\n");

    // Creating /proc file
    device.proc_file = proc_create(device.label, 0666, NULL, &fops);
    if (device.proc_file == NULL)
    {
        printk("procfs_test - Error creating /proc/servo\n");
        return -ENOMEM;
    }
    printk("procfs_test - Creat /proc/servo\n");

    /**
     * @todo przemysl i to popraw bo troche patoDeveloperka
     */
    switch (servo_move)
    {
    case 0:
        ret = pwm_config(device.pwm, device.pwm_duty_cycle_stop, device.pwm_freq);
        break;
    case 1:
        ret = pwm_config(device.pwm, device.pwm_duty_cycle_slow_move, device.pwm_freq);
        break;
    case 2:
        ret = pwm_config(device.pwm, device.pwm_duty_cycle_fast_move, device.pwm_freq);
        break;
    default:
        printk("This should not happened. U win\n");
        return -1;
        break;
    }
    if (ret)
    {
        printk("PWM - Failed to config\n");
        pwm_free(device.pwm);
        return ret;
    }
    printk("PWM - Succesful to config\n");

    ret = pwm_enable(device.pwm);
    if (ret)
    {
        printk("PWM - Failed to enable PWM\n");
        pwm_free(device.pwm);
    }
    printk("PWM - Succesful to enable PWM\n");

    devices[(int)mapStringToLabels(label)] = device;

    return 0;
}

/**
 * @brief This fin is called, when the module is removing from the kernel
 */
static int dt_remove(struct platform_device *pdev)
{
    printk("dt_servo - Now I am in the remove function\n");

    for (int i = 0; i < MAX_DEVICES; i++)
    {
        proc_remove(devices[i].proc_file);
        pwm_disable(devices[i].pwm);
        pwm_free(devices[i].pwm);
    }
    printk("procfs_test - Delete /proc/servo\n");
    return 0;
}

/**
 * @brief This fun is called, when the module is loaded into the kernel
 */
static int __init my_servo_init(void)
{
    printk("Hello, U just inicjalized ur servo!");
    if (platform_driver_register(&servo_driver))
    {
        printk("my_servo_init - Error! Could not load driver");
    }
    return 0;
}

/**
 * @brief This fin is called, when the module is removed from the kernel
 */
static void __exit my_servo_exit(void)
{
    printk("Goodbye, U just deinicjalized ur servo!");
    platform_driver_unregister(&servo_driver);
}

module_init(my_servo_init);
module_exit(my_servo_exit);
