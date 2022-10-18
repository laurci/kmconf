#include "config.h"
#include "init.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Laurentiu Ciobanu");
MODULE_DESCRIPTION("Device for project configuration files");

#pragma region IPC Implementation
static int ipc_open(struct inode *i, struct file *f)
{
    LOG_INFO("IPC: open()\n");
    return 0;
}

static int ipc_close(struct inode *i, struct file *f)
{
    LOG_INFO("IPC: close()\n");
    return 0;
}

#pragma endregion


#pragma region IPC Device init
struct ipc_device_data {
    struct cdev cdev;
    struct device* device;
    struct class* class;
};

static struct ipc_device_data ipc_device;

static dev_t ipc_first_device;

static struct file_operations ipc_fops = {
    .owner = THIS_MODULE,
    .open = ipc_open,
    .release = ipc_close
};

static int init_ipc_device(void) {
    int err;

    if ((err = alloc_chrdev_region(&ipc_first_device, 0, 1, "jscfgipc")) < 0)
    {
        return err;
    }

    ipc_device.class = class_create(THIS_MODULE, "jscfgipc");
    ipc_device.device = device_create(ipc_device.class, NULL, ipc_first_device, NULL, "jscfgipc");

    cdev_init(&ipc_device.cdev, &ipc_fops);
    cdev_add(&ipc_device.cdev, ipc_first_device, 1);

    return 0;
}

static void cleanup_ipc_device(void) {
    cdev_del(&ipc_device.cdev);
    device_destroy(ipc_device.class, ipc_device.device);
    class_destroy(ipc_device.class);
    unregister_chrdev_region(ipc_first_device, 1);
}
#pragma endregion

#pragma region Config Devices init
struct config_device_data {
    struct cdev cdev;
    struct device* device;
};

static struct config_device_data config_devices[MAX_CONFIG_DEVICES];

static dev_t config_first_device;
static struct class* config_device_class;

static struct file_operations config_fops = {
    .owner = THIS_MODULE,
};

static int init_config_devices(void) {
    int err, i;

    if ((err = alloc_chrdev_region(&config_first_device, 0, 1, "jscfgi")) < 0)
    {
        return err;
    }

    config_device_class = class_create(THIS_MODULE, "jscfgi");

    for (i = 0; i < MAX_CONFIG_DEVICES; i++) {
        config_devices[i].device = device_create(config_device_class, NULL, MKDEV(MAJOR(config_first_device), i), NULL, "jscfgi%d", i);
        cdev_init(&config_devices[i].cdev, &config_fops);
        cdev_add(&config_devices[i].cdev, MKDEV(MAJOR(config_first_device), i), 1);
    }

    return 0;
}

static void cleanup_config_devices(void) {
    int i;

    for (i = 0; i < MAX_CONFIG_DEVICES; i++) {
        cdev_del(&config_devices[i].cdev);
        device_destroy(config_device_class, config_devices[i].device);
    }
    class_destroy(config_device_class);
    unregister_chrdev_region(config_first_device, 1);
}

#pragma endregion

static int __init config_device_init(void)
{
    int ret;
    LOG_INFO("init()\n");
    
    if ((ret = init_ipc_device()) < 0)
    {
        LOG_INFO("Failed to init IPC device\n");
        return ret;
    }

    if ((ret = init_config_devices()) < 0)
    {
        LOG_INFO("Failed to init config devices\n");
        return ret;
    }

    LOG_INFO("init() done\n");

    return 0;
}

static void __exit config_device_exit(void)
{
    cleanup_ipc_device();
    cleanup_config_devices();
    LOG_INFO("exit()");
}

module_init(config_device_init);
module_exit(config_device_exit);
