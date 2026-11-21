#include <linux/init.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kthread.h>
#include <asm/io.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/firmware.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/err.h>



struct cus_chip
{
	struct mutex		lock;
	struct device 		*dev;
	int (*write)(struct cus_chip *chip, unsigned short reg, unsigned short val);
	int (*multi_write)(struct cus_chip *chip, const void *pdata, unsigned short  len);
	unsigned short 		xfer_buf[2];
	unsigned short 		usBuswidth;
	unsigned short  	usMode;
	unsigned int      	u32MaxSpeedHz;
	unsigned int      	u32SpeedHz;
};

static struct cus_chip *cus_chip_priv;

static const struct spi_device_id fpga_ids[] = {
  
    { "chip_vendor,  fpga" },
    { },
    
};
MODULE_DEVICE_TABLE(spi, fpga_ids);


static int fpga_dev_spi_write(struct cus_chip *chip, 
	   		      unsigned short reg, 
			      unsigned short data)
{
	struct spi_device *spi = to_spi_device(chip->dev);
	int error=0;
	struct spi_transfer xfer;
	struct spi_message message;

	memset(&xfer, 0 , sizeof(struct spi_transfer));

	chip->xfer_buf[0] = cpu_to_le16(reg);
	chip->xfer_buf[1] = cpu_to_le16(data);

	dev_dbg(chip->dev, "(spi speed, spi bus width , spi mode) = (%d, %d, %d) \n",
		                       spi->max_speed_hz,
		                       spi->bits_per_word,
		                       spi->mode);

	xfer.bits_per_word = spi->bits_per_word;
	xfer.speed_hz 	= chip->u32SpeedHz;
	xfer.tx_buf 		= chip->xfer_buf;
	xfer.len 			= 2*sizeof(chip->xfer_buf[0]);
	spi_message_init(&message);
	spi_message_add_tail(&xfer, &message);

	error = spi_sync(spi, &message);

	if(unlikely(error)){
		dev_err(chip->dev, "spi write error %d\n",error);
		return error;
	}
	return error;
}

static int fpga_dev_spi_multi_write(struct cus_chip *chip, 
	 	  		    const void *pdata, 
				    size_t  len)
{
	struct spi_device *spi = to_spi_device(chip->dev);
	int error=0;
	struct spi_transfer xfer;
	struct spi_message message;

	memset(&xfer, 0 , sizeof(struct spi_transfer));

	if(IS_ERR(pdata) )
	{
		error = PTR_ERR(pdata);
		dev_err(chip->dev, "spi multi write (error code %d) since null pointer\n",error);
		return error;
	}

	dev_info(chip->dev, "(spi speed, bit per word, mode) = (%d Hz, %d bit, %d)\n",
		                       spi->max_speed_hz,
		                       spi->bits_per_word,
		                       spi->mode);

	xfer.bits_per_word = spi->bits_per_word;
	xfer.speed_hz 	= spi->max_speed_hz;
	xfer.tx_buf 		= (void *)pdata;
	xfer.len 			= len;

	dev_info(chip->dev, "Total write transfer bytes:%zu \n",xfer.len);
	spi_message_init(&message);
	spi_message_add_tail(&xfer, &message);

	error = spi_sync(spi, &message);

	if(unlikely(error)){
		dev_err(chip->dev, "spi write error %d\n",error);
		return error;
	}
	return error;
}

int  fpga_probe(struct device *dev, 
				   struct cus_chip *chip)
{
	int error;
	if(IS_ERR(dev) )
	{
		error = PTR_ERR(dev);
		pr_err("dev not exist error: %d\n",error);
		return error;
	}
	chip->dev = dev;
	chip->write = fpga_dev_spi_write;
	chip->multi_write = fpga_dev_spi_multi_write;
	return 0;
}

static int fpga_spi_probe(struct spi_device *spi)
{   
	int ret;
	const struct spi_device_id __maybe_unused *id = spi_get_device_id(spi);   
	struct device_node __maybe_unused *np = spi->dev.of_node;

	if (!of_device_is_available(np))   {
	    pr_err("device is not available \n");	
	    return -ENODEV;  
	}

	cus_chip_priv =devm_kzalloc(&spi->dev, sizeof(struct cus_chip), GFP_KERNEL);
	if(!cus_chip_priv) {
		dev_err(&spi->dev, "priv data allocate failed\n");
		return -ENOMEM;
	}

	/*Given default here, overwrite by DT setting if DT property exist*/
	cus_chip_priv->usBuswidth		= 8;
	cus_chip_priv->usMode			= spi->mode;
	cus_chip_priv->u32MaxSpeedHz	= spi->max_speed_hz;
	cus_chip_priv->u32SpeedHz		= spi->max_speed_hz;

	#if 0
	if( device_property_read_u16(&spi->dev, "buswidth", &cus_chip_priv->usBuswidth) < 0)
	{
		dev_info(&spi->dev, "spi default bit per word: %d\n", cus_chip_priv->usBuswidth);
	}
	#endif
	device_property_read_u16(&spi->dev, "buswidth", &cus_chip_priv->usBuswidth);
	spi->bits_per_word 		= cus_chip_priv->usBuswidth;
	ret = spi_setup(spi);

	if (ret < 0) {
		dev_err(&spi->dev, "failed to setup spi:%d \n",ret);
		return ret;
	}

	ret = fpga_probe(&spi->dev, cus_chip_priv);

	spi_set_drvdata(spi, cus_chip_priv);
	return ret;
}

int  fpga_write(unsigned short reg,  unsigned short data)
{
	int error=0;
	if(IS_ERR(cus_chip_priv)) {
		error = PTR_ERR(cus_chip_priv);
		return error;
	}
	if(cus_chip_priv->write)
		error =cus_chip_priv->write(cus_chip_priv, reg, data );
	return error;
}

int  fpga_multi_write(const void *pdata, size_t  len)
{
	int error=0;
	if(IS_ERR(cus_chip_priv)) {
		error = PTR_ERR(cus_chip_priv);
		return error;
	}
	if(cus_chip_priv->multi_write)
		error =cus_chip_priv->multi_write(cus_chip_priv, pdata, len );
	return error;
}

#ifdef CONFIG_OF
 static const struct of_device_id fpga_dev_table[] = {
         { .compatible = "chip_vendor, fpga" },
         {}
 };
MODULE_DEVICE_TABLE(of, fpga_dev_table);
#endif

static struct spi_driver fpga_driver = {
    .driver = {
        .name   = "fpga_driver",
        .owner  = THIS_MODULE,
        .of_match_table = fpga_dev_table,
    },
    .id_table   = fpga_ids,
    .probe  = fpga_spi_probe,
};

/* regster struct spi_driver to kernel */
module_spi_driver(fpga_driver);
MODULE_LICENSE("GPL");

EXPORT_SYMBOL_GPL(fpga_write);
EXPORT_SYMBOL_GPL(fpga_multi_write);


