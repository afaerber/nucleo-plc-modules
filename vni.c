#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/spi/spi.h>

static int vni_probe(struct spi_device *spi)
{
	u8 x;
	u8 buf[2];
	struct spi_transfer xfer = {
		.tx_buf = buf,
		.len = 2,
	};
	int out_en;
	int ret;

	out_en = of_get_named_gpio(spi->dev.of_node, "output-enable-gpios", 0);
	if (out_en == -ENOENT)
		dev_warn(&spi->dev, "no OUT_EN GPIO available, ignoring\n");

	if (gpio_is_valid(out_en))
		gpio_set_value(out_en, 1);

	spi->bits_per_word = 8;
	ret = spi_setup(spi);
	if (ret)
		return ret;

	x = 0xf0;
	buf[0] = x;
	buf[1] = ((((x >> 0) & 0x1) +
		   ((x >> 1) & 0x1) +
		   ((x >> 2) & 0x1) +
		   ((x >> 3) & 0x1) +
		   ((x >> 4) & 0x1) +
		   ((x >> 5) & 0x1) +
		   ((x >> 6) & 0x1) +
		   ((x >> 7) & 0x1)) & 0x1) << 1;
	buf[1] |= ((buf[1] >> 1) & 0x1) ? 0 : 1;
	buf[1] |= ((((x >> 1) & 0x1) +
		    ((x >> 3) & 0x1) +
		    ((x >> 5) & 0x1) +
		    ((x >> 7) & 0x1)) & 0x1) << 2;
	buf[1] |= ((((x >> 0) & 0x1) +
		    ((x >> 2) & 0x1) +
		    ((x >> 4) & 0x1) +
		    ((x >> 6) & 0x1)) & 0x1) << 3;

	ret = spi_sync_transfer(spi, &xfer, 1);
	if (ret < 0)
		return ret;

	dev_info(&spi->dev, "VNI8200XP\n");

	return 0;
}

static int vni_remove(struct spi_device *spi)
{
	return 0;
}

static const struct of_device_id vni_dt_ids[] = {
	{ .compatible = "st,vni8200xp" },
//	{ .compatible = "st,clt01-38sq7" },
	{}
};
MODULE_DEVICE_TABLE(of, vni_dt_ids);

static struct spi_driver vni_spi_driver = {
	.driver = {
		.name = "vni8200xp",
		.of_match_table = vni_dt_ids,
	},
	.probe = vni_probe,
	.remove = vni_remove,
};

module_spi_driver(vni_spi_driver);

MODULE_LICENSE("GPL");
