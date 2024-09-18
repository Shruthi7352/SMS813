#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/device.h>

#define GPIO_USER_LED 193   // Specify the pin number here

int major_number;
struct class *sled_class;
struct device *sled_device;
char kbuff[20];

int led_init(int gpio_num)
{
    int retval = 0;
    retval = gpio_request(gpio_num, "gpio_user_led");

    if (retval < 0)
        return retval;

    retval = gpio_direction_output(gpio_num, 0); // Initially turn off LED
    return retval;
}

void led_on(int gpio_num)
{
    gpio_set_value(gpio_num, 1);
}

void led_off(int gpio_num)
{
    gpio_set_value(gpio_num, 0);
}

int sled_open(struct inode *in, struct file *fp)
{
    printk(KERN_INFO "This is sled open function\n");
    if (led_init(GPIO_USER_LED) < 0) {
        printk(KERN_ERR "Failed to initialize LED on pin %d\n", GPIO_USER_LED);
        return -EINVAL;
    }
    return 0;
}

ssize_t sled_write(struct file *fp, const char __user *buff, size_t sz, loff_t *offset)
{
    printk(KERN_INFO "This is sled write function\n");
    copy_from_user(kbuff, buff, sz);

    if (kbuff[0] == 'O' && kbuff[1] == 'N') {
        led_on(GPIO_USER_LED);
        msleep(100); // Sleep for 500 milliseconds to ensure LED stays on momentarily
        led_off(GPIO_USER_LED);
    } else if (kbuff[0] == 'O' && kbuff[1] == 'F' && kbuff[2] == 'F') {
        led_off(GPIO_USER_LED);
    } else {
        printk(KERN_INFO "Invalid function");
    }
    return 0;
}

int sled_release(struct inode *in, struct file *fp)
{
    printk(KERN_INFO "This is sled release function\n");
    return 0;
}

struct file_operations fops = {
    .open = sled_open,
    .write = sled_write,
    .release = sled_release,
};

static int __init sled_init(void)
{
    major_number = register_chrdev(0, "sled", &fops);
    if (major_number < 0) {
        printk(KERN_ERR "Failed to register character device\n");
        return major_number;
    }

    sled_class = class_create(THIS_MODULE, "sled");
    if (IS_ERR(sled_class)) {
        unregister_chrdev(major_number, "sled");
        return PTR_ERR(sled_class);
    }

    sled_device = device_create(sled_class, NULL, MKDEV(major_number, 0), NULL, "sled");
    if (IS_ERR(sled_device)) {
        class_destroy(sled_class);
        unregister_chrdev(major_number, "sled");
        return PTR_ERR(sled_device);
    }

    printk(KERN_INFO "sled driver Inserted..\n");
    return 0;
}

static void __exit sled_exit(void)
{
    device_destroy(sled_class, MKDEV(major_number, 0));
    class_unregister(sled_class);
    class_destroy(sled_class);
    unregister_chrdev(major_number, "sled");
    printk(KERN_INFO "sled driver Removed...\n");
}

module_init(sled_init);
module_exit(sled_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SASWATI");
MODULE_DESCRIPTION("sled driver");
