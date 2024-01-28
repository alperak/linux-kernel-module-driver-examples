#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/i2c.h>

/**
 * Before we start, you can access datasheet -> https://cdn-shop.adafruit.com/datasheets/BST-BMP280-DS001-11.pdf
 * Also checkout https://docs.kernel.org/i2c/writing-clients.html for 'how to implement I2C device drivers' to have general knowledge
 */
#define DRIVER_NAME "bmp280"
#define DRIVER_CLASS "bmp280_class"

/* Initialize I2C adapter. The i2c_adapter is used to communicate with I2C devices over a physical I2C bus. */
static struct i2c_adapter *bmp280_i2c_adapter = NULL;
/* Initialize I2C client. The i2c_client identifies a single device connected to an I2C bus. Contains device-specific information, such as its address */
static struct i2c_client *bmp280_i2c_client = NULL;

/* Defines for device identification */
#define I2C_BUS_AVAILABLE	1			/* Using Raspberry Pi 3B, it has only one I2C Bus */
#define SLAVE_DEVICE_NAME	"BMP280"
#define BMP280_SLAVE_ADDRESS	0x76	/* BMP280 I2C address -> From datasheet 5.2 I2C Interface, page 28 */

/**
 * i2c_device_id structure is referenced by the i2c_driver structure.
 * Also, the I2C framework uses it to find the driver that is to be attached to a specific I2C device
 */
static const struct i2c_device_id bmp280_id[] = {
		{ SLAVE_DEVICE_NAME, 0 }
};

/**
 * Represent an I2C device driver
 * Contains and characterizes general access routines needed to handle the devices claiming the driver
 */
static struct i2c_driver bmp280_i2c_driver = {
	.driver = {
		.name = SLAVE_DEVICE_NAME,
		.owner = THIS_MODULE
	}
};

/* Used to build tables of information listing I2C devices that are present */
static struct i2c_board_info bmp280_i2c_board_info = {
	I2C_BOARD_INFO(SLAVE_DEVICE_NAME, BMP280_SLAVE_ADDRESS)
};

dev_t dev;
static struct class *dev_class;
static struct cdev my_cdev;

/**
 * Calculate temperature by referring to section '3.11.3 Compensation formula(page 21)' of the data sheet.
 * Declare variables for temperature calculation
 */
u16 dig_T1;
s16 dig_T2, dig_T3;

/* Read current temperature from BMP280 */
s32 read_temperature(void) {
	int var1, var2;
	s32 raw_temp;
	s32 d1, d2, d3;

	/**
	 * Read temperature
	 * Using i2c_smbus_read_byte_data instead of i2c_smbus_read_word_data because we read 8 bit(1 byte) data
	 * Check '4.2 Memory map' section from datasheet
	 * i2c_smbus_read_word_data used to read 16 bits(2 bytes) of data from an I2C device
	 */
	d1 = i2c_smbus_read_byte_data(bmp280_i2c_client, 0xFA);	/* Contains the MSB(Most Significant Bit) part ut[19:12] */
	d2 = i2c_smbus_read_byte_data(bmp280_i2c_client, 0xFB);	/* Contains the LSB(Least Significant Bit) part ut[11:4]  */
	d3 = i2c_smbus_read_byte_data(bmp280_i2c_client, 0xFC);	/* Contains the XLSB(Extra Least Significant Bit) part ut[3:0] */
	/**
	 * Both pressure and temperature values are expected to be received in 20 bit format,
	 * positive, stored in a 32 bit signed integer(s32).
	 */
	raw_temp = ((d1 << 12) | (d2 << 4) | (d3 << 4));

	/**
	 * Calculate temperature
	 * Check '3.11.3 Compensation formula'
	 */
	var1 = ((((raw_temp >> 3) - ((s32)dig_T1 << 1))) * ((s32)dig_T2)) >> 11;
	var2 = (((((raw_temp >> 4) - ((s32)dig_T1)) * ((raw_temp >> 4) - ((s32)dig_T1))) >> 12) * ((s32)dig_T3)) >> 14;
	return ((var1 + var2) *5 +128) >> 8; /* Returns temperature in DegC */
}

static ssize_t driver_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
	int to_copy, not_copied, delta;
	char out_string[20];
	int temperature;

	/* Get amount of bytes to copy */
	to_copy = min(sizeof(out_string), len);

	/* Get temperature */
	temperature = read_temperature();
	snprintf(out_string, sizeof(out_string), "%d.%d\n", temperature/100, temperature%100);

	/* Copy Data to user */
	not_copied = copy_to_user(buf, out_string, to_copy);

	/* Calculate delta */
	delta = to_copy - not_copied;

	return delta;
}

static int driver_open(struct inode *inode, struct file *file) {
	printk("Open called for BMP280 device driver\n");
	return 0;
}

static int driver_release(struct inode *inode, struct file *file) {
	printk("Release called for BMP280 device driver\n");
	return 0;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = driver_open,
	.release = driver_release,
	.read = driver_read,
};

static int __init ModuleInit(void)
{
	int err = 0;
	u8 id;

	/* Dynamically allocating major number */
	if ((alloc_chrdev_region(&dev, 0, 1, DRIVER_NAME) < 0)) {
		pr_info("Couldn't allocated major number\n");
		return -1;
	}
	pr_info("Major: %d Minor: %d\n", MAJOR(dev), MINOR(dev));

	/* Initialize cdev structure */
	cdev_init(&my_cdev, &fops);

	/* Add char device to kernel */
	if ((cdev_add(&my_cdev, dev, 1)) < 0) {
		pr_info("Couldn't added character device\n");
		goto kernel_error;
	}

	/* Create struct class structure */
	if (IS_ERR(dev_class = class_create(THIS_MODULE, DRIVER_CLASS))) {
		pr_err("Couldn't created struct class\n");
		goto class_error;
	}

	/* Create device and registers it with sysfs */
	if(IS_ERR(device_create(dev_class, NULL, dev, NULL, DRIVER_NAME))) {
		pr_err("Couldn't created device\n");
		goto file_error;
	}

	/* The I2C adapter resource is received. This adapter provides access to the physical I2C connection. */
	bmp280_i2c_adapter = i2c_get_adapter(I2C_BUS_AVAILABLE);

	if (bmp280_i2c_adapter) {
		bmp280_i2c_client = i2c_new_client_device(bmp280_i2c_adapter, &bmp280_i2c_board_info); /* The interface is used to instantiate an I2C client device. */
		if (bmp280_i2c_client) {
			err = i2c_add_driver(&bmp280_i2c_driver); /* Register the driver */
			if(err) {
				pr_info("Couldn't added BMP280 driver\n");
				return err;
			}
		}
		i2c_put_adapter(bmp280_i2c_adapter); /* Release the I2C adapter resource obtained by i2c_get_adapter */
	}
	pr_info("BMP280 device driver added successfully\n");

	/* Read ID and set by using '4.2 Memory map' section from datasheet */
	id = i2c_smbus_read_byte_data(bmp280_i2c_client, 0xD0);
	pr_info("ID: 0x%x\n", id);

	/**
	 * Read Calibration Values
	 * Using i2c_smbus_read_word_data instead of i2c_smbus_read_byte_data because dig_T1, dig_T2, dig_T3 are 16 bit data types
	 * Check it from '3.11.2 and 3.11.3' section from datasheet
	 */
	dig_T1 = i2c_smbus_read_word_data(bmp280_i2c_client, 0x88);
	dig_T2 = i2c_smbus_read_word_data(bmp280_i2c_client, 0x8A);
	dig_T3 = i2c_smbus_read_word_data(bmp280_i2c_client, 0x8C);

	if(dig_T2 > 32767)
		dig_T2 -= 65536;

	if(dig_T3 > 32767)
		dig_T3 -= 65536;

	/**
	 * Set device config accordingly '4.2 Memory map', '4.3.5 Register 0xF5', '3.6.3 Normal mode' sections from datasheet
	 * We gonna set 'controls inactive duration t_standby in normal mode' to 1000ms(1sec)
	 */
	i2c_smbus_write_byte_data(bmp280_i2c_client, 0xF5, 5<<5);

	/**
	 * Set ctrl_meas which is register sets the data acquisition options of the device.
	 * Accordingly '4.2 Memory map', '4.3.4 Register 0xF4', '3.6 Power modes' sections from datasheet
	 * We gonna use 'Temperature oversampling x16', 'Pressure oversampling x16', 'Normal mode'
	 */
	i2c_smbus_write_byte_data(bmp280_i2c_client, 0xF4, ((5<<5) | (5<<2) | (3<<0)));

	return 0;

kernel_error:
	device_destroy(dev_class, dev);
file_error:
	class_destroy(dev_class);
class_error:
	unregister_chrdev(dev, DRIVER_NAME);
	return (-1);
}

static void __exit ModuleExit(void)
{
	pr_info("BMP280 device driver removed successfuly\n");
	i2c_unregister_device(bmp280_i2c_client);
	i2c_del_driver(&bmp280_i2c_driver);
	cdev_del(&my_cdev);
	device_destroy(dev_class, dev);
	class_destroy(dev_class);
	unregister_chrdev_region(dev, 1);
}

module_init(ModuleInit);
module_exit(ModuleExit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alper Ak");
MODULE_DESCRIPTION("BMP280 I2C Temperature Driver");
