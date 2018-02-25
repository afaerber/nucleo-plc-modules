#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/spi/spi.h>

static int clt_read(struct spi_device *spi)
{
	u8 buf[2];
	struct spi_transfer xfer = {
		.rx_buf = buf,
		.len = 2,
	};
	int ret;

	ret = spi_sync_transfer(spi, &xfer, 1);
	if (ret < 0)
		return ret;

	dev_info(&spi->dev, "read 0x%02x 0x%02x\n", buf[0], buf[1]);

	return 0;
}

static int clt_probe(struct spi_device *spi)
{
	int ret;

	spi->bits_per_word = 8;
	ret = spi_setup(spi);
	if (ret)
		return ret;

	ret = clt_read(spi);
	if (ret)
		return ret;

	dev_info(&spi->dev, "CLT01-38SQ7\n");

	return 0;
}

static int clt_remove(struct spi_device *spi)
{
	return 0;
}

static const struct of_device_id clt_dt_ids[] = {
//	{ .compatible = "st,vni8200xp" },
	{ .compatible = "st,clt01-38sq7" },
	{}
};
MODULE_DEVICE_TABLE(of, clt_dt_ids);

static struct spi_driver clt_spi_driver = {
	.driver = {
		.name = "clt01-38sq7",
		.of_match_table = clt_dt_ids,
	},
	.probe = clt_probe,
	.remove = clt_remove,
};

module_spi_driver(clt_spi_driver);

MODULE_LICENSE("GPL");
