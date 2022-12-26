/* This file structure is based on chardev.c file from recitation6 */
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE


#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/slab.h>
#include <linux/errno.h>

MODULE_LICENSE("GPL");

/* Our custom definitions of IOCTL operations */
#include "message_slot.h"

/*
* channel represent each channels data:
*   -> the channel id
*   -> the last message in the channel
*   -> the message size in bytes
*   -> a pointer to the next channel in the message slot file channels linked 
*       list
*   -> a pointer to the messageSlotFile it belongs to
*/
typedef struct channel
{
    unsigned long channel_id;
    char* message;
    int message_size;
    struct channel* next;
} channel;

/* 
* messageSlotFile represent the message slot file and its' data:
*   -> minor number
*   -> a pointer to it's channels linked list 
*   -> a counter for the channels amount
*/
typedef struct messageSlotFile
{
    int minor;
    channel* channels_linked_list_head;
    int channels_count;
    channel* active_channel;
} messageSlotFile;



static long create_new_channel( messageSlotFile* message_slot_file,
                                unsigned long  ioctl_param);
static void free_channel_memory(messageSlotFile* message_slot_file);

/*
* The message slot files are accessed by minor number as index in the 
* message_slot_files_arr
*/
static messageSlotFile* message_slot_files_arr[256];

/*================== DEVICE FUNCTIONS ===========================*/
static int device_open( struct inode* inode,
                        struct file*  file )
{
    messageSlotFile* message_slot_file;
    int minor;

    printk("Invoking device_open(%p)\n", file);
    
    // if the file was opened before
    if (file->private_data != NULL) {
        message_slot_file = (messageSlotFile*) file->private_data;
        minor = message_slot_file->minor;
        printk("Open device minor number (%d)\n", minor);
        return SUCCESS;
    }

    message_slot_file = (messageSlotFile*) kmalloc(sizeof(messageSlotFile), GFP_KERNEL);
    if (message_slot_file == NULL) {  /* check memory allocation */
        return -EFAULT;
    }

    /* init new message_slot_file */
    minor = iminor(inode);
    message_slot_file->minor = minor;
    message_slot_file->channels_linked_list_head = NULL;
    message_slot_file->channels_count = 0;
    message_slot_file->active_channel = NULL;

    /* update message_slot_files_arr with new message_slot_files */
    message_slot_files_arr[minor] = message_slot_file;
    /* file points to current message_slot_file */
    file->private_data = message_slot_file;
    
    printk("Open device minor number (%d)\n", minor);
    return SUCCESS;
}


static int device_release( struct inode* inode,
                           struct file*  file)
{
    messageSlotFile* message_slot_file;
    int minor;
    
    printk("Invoking device_release(%p,%p)\n", inode, file);
    message_slot_file = (messageSlotFile*) file->private_data;
    minor = message_slot_file->minor;
    free_channel_memory(message_slot_file);
    kfree(message_slot_file);
    
    message_slot_files_arr[minor] = NULL;
    file->private_data = NULL;
    return SUCCESS;
}


static ssize_t device_read( struct file* file,
                            char __user* buffer,
                            size_t       length,
                            loff_t*      offset )
{
    char* the_message;
    int i;
    messageSlotFile* message_slot_file;
    channel* curr_channel;

    printk( "Invocing device_read(%p,%ld)\n", file, length);

    message_slot_file = (messageSlotFile*) file->private_data;
    curr_channel = message_slot_file->active_channel;

    /* check error scenarios */
    if (curr_channel == NULL) { return -EINVAL; }
    if (curr_channel->message_size == 0) { return -EWOULDBLOCK; }
    if (length < curr_channel->message_size) { return -ENOSPC; }

    /* read current message */
    the_message = curr_channel->message;
    for (i = 0; i < curr_channel->message_size; ++i) {
        if (put_user(curr_channel->message[i], &buffer[i]) != 0) {
            return -EFAULT;
        }
    }
    return i;
}

static ssize_t device_write( struct file*       file,
                             const char __user* buffer,
                             size_t             length,
                             loff_t*            offset)
{
    char* the_message;
    int i;
    messageSlotFile* message_slot_file;
    channel* curr_channel;

    printk( "Invocing device_write(%p,%ld)\n", file, length);

    message_slot_file = (messageSlotFile*) file->private_data;
    curr_channel = message_slot_file->active_channel;

    /* check error scenarios */
    if (curr_channel == NULL) { return -EINVAL; }
    if (length == 0 || length > 128) { return -EMSGSIZE; }

    the_message = curr_channel->message;
    kfree(the_message);  /* delete old message */
    the_message = (char*) kmalloc(length*sizeof(char), GFP_KERNEL);
    if (the_message == NULL) { return -EFAULT; } /* validate memory allocation */
    
    /* write current message */
    for (i = 0; i < length; ++i) {
        if (get_user(the_message[i], &buffer[i]) != 0) {
            return -EFAULT;
        }
    }
    curr_channel->message = the_message;
    curr_channel->message_size = i;

    return i;
}

static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param )
{
    channel* curr_channel;
    messageSlotFile* message_slot_file;
    int i;
    int channels_count;
    
    message_slot_file = (messageSlotFile*) file->private_data;
    channels_count = message_slot_file->channels_count;

    if (ioctl_command_id != MSG_SLOT_CHANNEL || ioctl_param == 0) {
        return -EINVAL;
    }
    
    /* if channel linked list is empty - add a new channel at head */
    if (message_slot_file->channels_linked_list_head == NULL) {
        return create_new_channel(message_slot_file, ioctl_param);
    }

    /* search the channel linked list for a matching channel id to the one in ioctl_param */
    curr_channel = message_slot_file->channels_linked_list_head;
    for (i = 0; i < channels_count; i++) {
        if (curr_channel->channel_id == ioctl_param) {
            message_slot_file->active_channel = curr_channel;
            return SUCCESS;
        }
        if (i != channels_count-1) {
            curr_channel = curr_channel->next;
        } else { /* if the channel id not found - create new channel */
            return create_new_channel(message_slot_file, ioctl_param);
        }
    }
    return SUCCESS;
}

static long create_new_channel( messageSlotFile* message_slot_file,
                                unsigned long  ioctl_param) 
{
    channel* new_channel;
    channel* tmp = NULL;

    new_channel = (channel*) kmalloc(sizeof(channel), GFP_KERNEL);
    if (new_channel == NULL) {  /* validate memory allocation */
        return -EFAULT;
    }
    /* init channel */
    new_channel->channel_id = ioctl_param;
    /* if the channels linked list is not empty - link to current head */
    if (message_slot_file->channels_linked_list_head != NULL) {
        tmp = message_slot_file->channels_linked_list_head;
    } 
    new_channel->next = tmp;
    new_channel->message = NULL;
    new_channel->message_size = 0;

    /* update corresponding message_slot_file with new channel */
    message_slot_file->channels_linked_list_head = new_channel;
    message_slot_file->channels_count++;
    message_slot_file->active_channel = new_channel;
    
    return SUCCESS;
}

/*==================== DEVICE SETUP =============================*/

struct file_operations Fops = {
  .owner	  = THIS_MODULE, 
  .read           = device_read,
  .write          = device_write,
  .open           = device_open,
  .unlocked_ioctl = device_ioctl,
  .release        = device_release,
};

/*---------------------------------------------------------------*/
/* Initialize the module - Register the character device */
static int __init simple_init(void)
{
    int i;
    messageSlotFile* message_slot_file;
    int rc = -1;

    // Register driver capabilities. Obtain major num
    rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );

    // Negative values signify an error
    if( rc < 0 ) {
      printk( KERN_ALERT "%s registraion failed for  %d\n",
                        DEVICE_FILE_NAME, MAJOR_NUM );
      return rc;
    }

    for (i = 0; i < 256; i++) {
        message_slot_file = message_slot_files_arr[i];
        message_slot_file->minor = -1;
        message_slot_file->channels_linked_list_head = NULL;
        message_slot_file->channels_count = 0;
        message_slot_file->active_channel = NULL;
    }
    
    printk( "Registeration is successful. ");
    printk( "mknod /dev/%s c %d 0\n", DEVICE_FILE_NAME, MAJOR_NUM );

    return SUCCESS;
}

/*---------------------------------------------------------------*/
static void __exit simple_cleanup(void)
{
    messageSlotFile* message_slot_file;
    int i;

    /* free device channel linked list memory */
    for (i = 0; i < 256; i++) {
        if (message_slot_files_arr[i] != NULL) {
            message_slot_file = message_slot_files_arr[i];
            free_channel_memory(message_slot_file);
            kfree(message_slot_file);
            message_slot_files_arr[i] = NULL;
        }
    }
    /* Unregister the device */
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

static void free_channel_memory(messageSlotFile* message_slot_file) 
{
    channel* curr_channel;
    channel* tmp_channel;
    int i;

    curr_channel = message_slot_file->channels_linked_list_head;
    tmp_channel = curr_channel;

    if (curr_channel != NULL) {
        for (i = 0; i < message_slot_file->channels_count; i++) {
            curr_channel = curr_channel->next;
            kfree(tmp_channel);
            tmp_channel = curr_channel;
        }
    }
}

/*---------------------------------------------------------------*/
module_init(simple_init);
module_exit(simple_cleanup);

/*========================= END OF FILE =========================*/
