/**
 * @file    myeeprom.c
 * @author  Christian Yanez
 * @date    2019-06-18
 * @version 0.1
 * @brief   Driver i2c bbb eeprom 24lc256
 * @see 
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/kernel.h>         		// Contains types, macros, functions for the kernel
#include <linux/fs.h>             		// Header for the Linux file system support
#include <linux/uaccess.h>          		// Required for the copy to user function

#define  DEVICE_NAME "myeeprom"    		///< The device will appear at /dev/ebbchar using this value
#define  CLASS_NAME  "i2c"        		///< The device class -- this is a character device driver
#define BUFFER_SIZE 20

/*************Defición de variables locales*********************/ 
const char ADDRESS[]={0x00,0x00};

static int    majorNumber;                  	///< Stores the device number -- determined automatically
static char   message[256] = {0};           	///< Memory for the string that is passed from userspace
static short  size_of_message;              	///< Used to remember the size of the string stored
static int    numberOpens = 0;              	///< Counts the number of times the device is opened
static struct class*  myeepromClass  = NULL; 	///< The device-driver class struct pointer
static struct device* myeepromDevice = NULL; 	///< The device-driver device struct pointer
static struct i2c_client *modClient;
// The prototype functions for the character driver -- must come before the struct definition
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

//-------------------------------------------------- 
/** @brief Devices are represented as file structure in the kernel. The file_operations structure from
 *  /linux/fs.h lists the callback functions that you wish to associated with your file operations
 *  using a C99 syntax structure. char devices usually implement open, read, write and release calls
 */
static struct file_operations fops =
{
   .open = dev_open,
   .read = dev_read,
   .write = dev_write,
   .release = dev_release,
};

static const struct i2c_device_id myeeprom_i2c_id[] = {
    { "myeeprom", 0 },
    { }
};

MODULE_DEVICE_TABLE(i2c, myeeprom_i2c_id);

static const struct of_device_id myeeprom_of_match[] = {
    { .compatible = "mse,myeeprom" },
    { }
};

MODULE_DEVICE_TABLE(of, myeeprom_of_match);

/*************Defición de funciones locales*********************/ 
static int __init myeeprom_init(void){
   printk(KERN_INFO "Initializing the eeprom module\n");
 
   // Try to dynamically allocate a major number for the device -- more difficult but worth it
   majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
   if (majorNumber<0){
      printk(KERN_ALERT "Failed to register a major number\n");
      return majorNumber;
   }
   printk(KERN_INFO "Registered correctly with major number %d\n", majorNumber);
 
   // Register the device class
   myeepromClass = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(myeepromClass)){                			// Check for error and clean up if there is
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class\n");
      return PTR_ERR(myeepromClass);         			// Correct way to return an error on a pointer
   }
   printk(KERN_INFO "Device class registered correctly\n");
 
   // Register the device driver
   myeepromDevice = device_create(myeepromClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(myeepromDevice)){               			// Clean up if there is an error
      class_destroy(myeepromClass);          			// Repeated code but the alternative is goto statements
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the device\n");
      return PTR_ERR(myeepromDevice);
   }
   printk(KERN_INFO "Device class created correctly\n"); 	// Made it! device was initialized

   return 0;
}
/**************************************************************/ 
static void __exit myeeprom_exit(void){
   device_destroy(myeepromClass, MKDEV(majorNumber, 0));     	// remove the device
   class_unregister(myeepromClass);                         	// unregister the device class
   class_destroy(myeepromClass);                             	// remove the device class
   unregister_chrdev(majorNumber, DEVICE_NAME);             	// unregister the major number
   printk(KERN_INFO "Goodbye from the module!\n");
}
/**************************************************************/ 
static int dev_open(struct inode *inodep, struct file *filep){
   numberOpens++;
   printk(KERN_INFO "Device has been opened %d time(s)\n", numberOpens);
   return 0;
}
/**************************************************************/ 
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
   int error_count = 0;
   int Ret;
   
   Ret=i2c_master_send(modClient,ADDRESS,2);
   pr_info("bytes a leer=%d",len);
   Ret=i2c_master_recv(modClient,message,len);
   // copy_to_user has the format ( * to, *from, size) and returns 0 on success
   error_count = copy_to_user(buffer, message, len);		
 
   if (error_count==0){            				// if true then have success
      pr_info("Sent %d characters to the user\n", Ret);
      return (size_of_message=0);  				// clear the position to the start and return 0
   }
   else {
      pr_info( "Failed to send %d characters to the user\n", error_count);
      return -EFAULT;              				// Failed -- return a bad address message (i.e. -14)
   }
}
/**************************************************************/ 
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
   int Ret;
   char buf[BUFFER_SIZE];
   
   copy_from_user(message,buffer,len);
   size_of_message = strlen(message); 
   
   pr_info("Recibido %zu del usuario \n",len);   		// appending received string with its length
   
   
   buf[0]=ADDRESS[0];
   buf[1]=ADDRESS[1];
   
   strcpy(buf+2,message);
   pr_info("copiado %s a buffer \n",buf+2);
   
   Ret=i2c_master_send(modClient,buf,size_of_message+2);
   pr_alert("Valor de retorno de escritura %d \n",Ret);
   
   return len;
}
/**************************************************************/ 
static int dev_release(struct inode *inodep, struct file *filep){
   pr_info("Device successfully closed\n");
   return 0;
}

static int myeeprom_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    myeeprom_init();
    modClient=client;
    return 0;
}

static int myeeprom_remove(struct i2c_client *client)
{

    myeeprom_exit();
    return 0;
}
/**************************************************************/ 
static struct i2c_driver myeeprom_i2c_driver = {
    .driver = {
        .name = "myeeprom",
        .of_match_table = myeeprom_of_match,
    },
    .probe = myeeprom_probe,
    .remove = myeeprom_remove,
    .id_table = myeeprom_i2c_id
};

module_i2c_driver(myeeprom_i2c_driver);


MODULE_LICENSE("GPL");
