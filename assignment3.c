#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/errno.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/time.h>
#include <linux/linkage.h>
#include <linux/syscalls.h>
#include <linux/printk.h>
#include <linux/semaphore.h>
#include <linux/slab.h>

//#include <asm/uacess.h>
MODULE_LICENSE("GPL");


static int deviceNumber;
static int fileopened=0;
static struct miscdevice my_misc_device;
static struct file_operations fops;
static ssize_t read_mydevice(struct file *filenode1, char *buffer, size_t blength, loff_t *offset);
static int open_mydevice(struct inode *filenode, struct file *fileptr);
static int close_mydevice(struct inode *filenode, struct file *fileptr);
static ssize_t write_mydevice(struct file *filenode1, const char *buffer, size_t blength, loff_t *offset);
char *message_buffer;
int buffer_size;
module_param(buffer_size, int, 0000);
static DEFINE_SEMAPHORE(full);
static DEFINE_SEMAPHORE(empty);
static DEFINE_SEMAPHORE(mutex);
static int Number_ofWrites=0;

static struct miscdevice my_misc_device = {
.minor = MISC_DYNAMIC_MINOR,
.name = "char_device",
.fops = &fops
};

static struct file_operations fops = {
  .open = open_mydevice,
  .read = read_mydevice,
  .write = write_mydevice,
  .release = close_mydevice
};


static int __init my_module_init(void)
{

deviceNumber=misc_register(&my_misc_device);
printk("the device number is %d",deviceNumber);
if (deviceNumber < 0) 
	{
		printk("Driver registration failed");
		return deviceNumber;
	}
message_buffer = kmalloc(buffer_size, GFP_KERNEL);
    if (!message_buffer){
	printk(KERN_ALERT "Error: Memory Not allocated\n");
}
    sema_init(&full, 0);
    sema_init(&empty, buffer_size);
    sema_init(&mutex, 1);
return 0;	
}



static void __exit my_exit(void)
{
misc_deregister(&my_misc_device); //unregister the device
kfree(message_buffer);
}

static int open_mydevice(struct inode *filenode, struct file *fileptr)
{

fileopened++;

//if (open_device) return -EBUSY;

printk("Device Drive accessed: %d" , fileopened);
return 0;
}

static ssize_t read_mydevice(struct file *filenode1, char *buffer, size_t blength, loff_t *offset)
{

int buf_pointer=0;
int ret,next;

        while(buf_pointer<blength)
        {

                if( down_interruptible(&full) < 0)
                        {
                                printk(KERN_ALERT "Exit from user");
                                return -1;
                        }
                if( down_interruptible(&mutex) < 0)
                        {
                                printk(KERN_ALERT "User exit manually");
                                return -1;
                        }
  		 ret =  copy_to_user(buffer+buf_pointer,message_buffer,sizeof(message_buffer));
                if(ret<0)
                        {
                                printk(KERN_ALERT "Copy_to_user Error");
                                return ret;
                        }
		for(next=0;next < Number_ofWrites-1; next++)       //shifting the buffer 
			{
				message_buffer[next]=message_buffer[next+1];
			}
		Number_ofWrites--; 
                up(&empty);
                up(&mutex);
                buf_pointer++;
        }
return blength;


//err_read = copy_to_user(buffer,message_buffer,sizeof(message_buffer)+1);
}

static ssize_t write_mydevice(struct file *filenode1,const char *buffer, size_t blength, loff_t *offset)
{

int buf_pointer=0;
int ret;

	while(buf_pointer<blength)
	{

		if( down_interruptible(&empty) < 0)
			{
				printk(KERN_ALERT "Exit from user");
				return -1;
			}		
		if( down_interruptible(&mutex) < 0)
			{
				printk(KERN_ALERT "User exit manually");
				return -1;
			}
  		ret =  copy_from_user(message_buffer,&buffer,sizeof(message_buffer));            
		if(ret<0)
			{
				printk(KERN_ALERT "Copy_from_user Error");
				return ret;
			}	  
		buf_pointer++;
                Number_ofWrites++;
	 	up(&full);
	 	up(&mutex);
	}	

   return blength;

}



static int close_mydevice(struct inode *filenode, struct file *fileptr)
{

fileopened--;
printk("Device is now closed");
return 0;
}
module_init(my_module_init);
module_exit(my_exit);
