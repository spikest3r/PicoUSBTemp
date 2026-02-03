#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define VENDOR_ID  0xCafe
#define PRODUCT_ID 0x4002
#define PICO_EP_OUT 0x01
#define PICO_EP_IN 0x81

struct usb_pico {
    struct usb_device *udev;
    unsigned char *bulk_in_buffer;
};

static struct usb_driver pico_driver;

static ssize_t pico_read(struct file *file, char __user *user_buffer,
                         size_t count, loff_t *ppos)
{
    struct usb_pico *dev = file->private_data;
    int retval;
    int actual_length;
    unsigned char *cmd_buf;
    unsigned char *read_buf;

    if (*ppos > 0) return 0;

    cmd_buf = kmalloc(1, GFP_KERNEL);
    read_buf = kmalloc(64, GFP_KERNEL);
    if (!cmd_buf || !read_buf) {
        kfree(cmd_buf); kfree(read_buf);
        return -ENOMEM;
    }

    *cmd_buf = 0xFE;

    // Send the command
    retval = usb_bulk_msg(dev->udev, usb_sndbulkpipe(dev->udev, PICO_EP_OUT),
                          cmd_buf, 1, &actual_length, 1000);

    if (retval) {
        printk(KERN_ERR "Pico USB: Write command failed: %d\n", retval);
        goto exit;
    }

    // Read the response string
    retval = usb_bulk_msg(dev->udev, usb_rcvbulkpipe(dev->udev, PICO_EP_IN),
                          read_buf, 64, &actual_length, 5000);

    if (retval) {
        printk(KERN_ERR "Pico USB: Read failed: %d\n", retval);
        goto exit;
    }

    // Copy to userspace
    if (copy_to_user(user_buffer, read_buf, actual_length)) {
        retval = -EFAULT;
        goto exit;
    }

    *ppos += actual_length;
    retval = actual_length;

    exit:
    kfree(cmd_buf);
    kfree(read_buf);
    return retval;
}

static int pico_open(struct inode *inode, struct file *file) {
    struct usb_interface *interface = usb_find_interface(&pico_driver, iminor(inode));
    file->private_data = usb_get_intfdata(interface);
    return 0;
}

static const struct file_operations pico_fops = {
    .owner = THIS_MODULE,
    .read  = pico_read,
    .open  = pico_open,
};

static struct usb_class_driver pico_class = {
    .name = "picotemp%d",
    .fops = &pico_fops,
    .minor_base = 0,
};

static int pico_probe(struct usb_interface *interface, const struct usb_device_id *id) {
    struct usb_pico *dev = kzalloc(sizeof(*dev), GFP_KERNEL);
    dev->udev = usb_get_dev(interface_to_usbdev(interface));
    usb_set_intfdata(interface, dev);

    usb_register_dev(interface, &pico_class);
    printk(KERN_INFO "Pico USB: Device connected to /dev/picotempX\n");
    return 0;
}

static void pico_disconnect(struct usb_interface *interface) {
    struct usb_pico *dev = usb_get_intfdata(interface);
    usb_deregister_dev(interface, &pico_class);
    usb_put_dev(dev->udev);
    kfree(dev);
    printk(KERN_INFO "Pico USB: Device removed\n");
}

static struct usb_device_id pico_table[] = {
    { USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
    {}
};
MODULE_DEVICE_TABLE(usb, pico_table);

static struct usb_driver pico_driver = {
    .name = "pico_driver",
    .probe = pico_probe,
    .disconnect = pico_disconnect,
    .id_table = pico_table,
};

module_usb_driver(pico_driver);
MODULE_LICENSE("GPL");
