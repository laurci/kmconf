#include "config.h"
#include "init.h"
#include "protocol.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Laurentiu Ciobanu");
MODULE_DESCRIPTION("Device for project configuration files");

static bool disable = false;

#pragma region Request queue
struct k_list {
    struct list_head queue_list;
    char *data;
};

static bool blocking_queue[BLOCKING_QUEUE_LENGTH] = {0};
static struct list_head *request_queue = NULL;
static char* response_queue[BLOCKING_QUEUE_LENGTH] = {0};


static bool wait_for_request(u32 req_id)
{
    if (req_id >= BLOCKING_QUEUE_LENGTH)
    {
        return false;
    }

    return blocking_queue[req_id] && response_queue[req_id] == NULL;
}

static void unblock_request(u32 req_id)
{
    if (req_id >= BLOCKING_QUEUE_LENGTH)
    {
        return;
    }

    blocking_queue[req_id] = false;
}

u32 next_available_request_id(void)
{
    u32 i;
    for (i = 0; i < BLOCKING_QUEUE_LENGTH; i++)
    {
        if (!blocking_queue[i])
        {
            blocking_queue[i] = true;
            return i;
        }
    }
    return -1;
}

void init_request_queue(void)
{
    request_queue = kmalloc(sizeof(struct list_head *), GFP_KERNEL);
    INIT_LIST_HEAD(request_queue);
}
#pragma endregion

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

DEFINE_MUTEX(x_ipc_read);
DEFINE_MUTEX(x_ipc_write);

static ssize_t ipc_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
    LOG_INFO("IPC: read()\n");

    mutex_lock(&x_ipc_read);

    if (*off == 0)
    {
        while(list_empty(request_queue) && !disable) yield();

        if(disable) {
            mutex_unlock(&x_ipc_read);
            return 0;
        }

        struct k_list *node = list_first_entry(request_queue, struct k_list, queue_list);
        u8 *data = node->data;

        copy_to_user(buf, data, sizeof(struct read_request_packet) + sizeof(struct packet_header)); // FIXME: this is hardcoded
        
        list_del(&node->queue_list);

        kfree(node->data);
        kfree(node);

        (*off) ++;

        mutex_unlock(&x_ipc_read);

        return sizeof(struct read_request_packet) + sizeof(struct packet_header);
    }

    mutex_unlock(&x_ipc_read);

    return 0;
}

static ssize_t ipc_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
{
    LOG_INFO("IPC: write()\n");

    mutex_lock(&x_ipc_write);


    struct packet_header *header = kmalloc(sizeof(struct packet_header), GFP_KERNEL);
    copy_from_user(header, buf, sizeof(struct packet_header));

    if(header->type == 'X') {
        LOG_INFO("disable\n");
        disable = true;
    } else if(header->type == PACKET_READ_RESPONSE) {
        struct read_response_packet *packet = kmalloc(sizeof(struct read_response_packet), GFP_KERNEL);
        copy_from_user(packet, buf + sizeof(struct packet_header), sizeof(struct read_response_packet));

        LOG_INFO("len %d\n", packet->length);

        char *text = kmalloc(packet->length, GFP_KERNEL);
        copy_from_user(text, buf + sizeof(struct packet_header) + sizeof(struct read_response_packet), packet->length);

        response_queue[packet->req_id] = text;
        unblock_request(packet->req_id);

        kfree(packet);
    }

    kfree(header);

    mutex_unlock(&x_ipc_write);
    return len;
}
#pragma endregion

#pragma region Config implementation
static int config_open(struct inode *i, struct file *f)
{
    LOG_INFO("Config: open()\n");
    return 0;
}

static int config_close(struct inode *i, struct file *f)
{
    LOG_INFO("Config: close()\n");
    return 0;
}

static u32 request_read_from_address(u32 address)
{
    u32 req_id = next_available_request_id();

    struct packet_header header = {
        .type = PACKET_READ_REQUEST
    };

    struct read_request_packet packet = {
        .req_id = req_id,
        .address = address
    };

    u8 *data = kmalloc(sizeof(header) + sizeof(packet), GFP_KERNEL);
    memcpy(data, &header, sizeof(header));
    memcpy(data + sizeof(header), &packet, sizeof(packet));

    struct k_list *node = kmalloc(sizeof(struct k_list *),GFP_KERNEL);
    node->data = data;
    list_add_tail(&node->queue_list, request_queue);

    return req_id;
}

static ssize_t config_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
    LOG_INFO("Config: read() %d\n", iminor(f->f_inode));

    if (*off == 0)
    {
        u32 req_id = request_read_from_address(iminor(f->f_inode));
        while(wait_for_request(req_id) && !disable) yield();

        if(disable) {
            return 0;
        }

        int data_len = strlen(response_queue[req_id]);

        copy_to_user(buf, response_queue[req_id], data_len);

        kfree(response_queue[req_id]);
        response_queue[req_id] = NULL;
        
        (*off) ++;

        return data_len;
    }
    else
    {
        return 0;
    }
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
    .release = ipc_close,
    .read = ipc_read,
    .write = ipc_write
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
    .open = config_open,
    .release = config_close,
    .read = config_read
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
    
    init_request_queue();

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
