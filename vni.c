#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/spi/spi.h>

struct vni_device {
	struct spi_device *spi;
	struct gpio_desc *out_en;
	struct gpio_chip gpio_chip;
	u8 val;
};

static int vni_send(struct spi_device *spi, u8 x)
{
	u8 buf[2];
	struct spi_transfer xfer = {
		.tx_buf = buf,
		.len = 2,
	};
	int ret;

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

	return 0;
}

static int vni_get_direction(struct gpio_chip *chip, unsigned int offset)
{
	return GPIOF_DIR_OUT;
}

static void vni_set(struct gpio_chip *chip, unsigned int offset, int value)
{
	struct vni_device *data = container_of(chip, struct vni_device, gpio_chip);

	if (value)
		data->val |= BIT(offset);
	else
		data->val &= ~BIT(offset);

	vni_send(data->spi, data->val);
}

static int vni_probe(struct spi_device *spi)
{
	struct vni_device *data;
	int ret;

	data = devm_kzalloc(&spi->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->spi = spi;

	data->out_en = devm_gpiod_get_optional(&spi->dev, "output-enable",
				GPIOD_OUT_HIGH);
	if (!data->out_en)
		dev_warn(&spi->dev, "no OUT_EN GPIO available, ignoring\n");

	if (data->out_en && !IS_ERR(data->out_en))
		gpiod_set_value(data->out_en, 1);

	spi->bits_per_word = 8;
	ret = spi_setup(spi);
	if (ret)
		return ret;

	data->gpio_chip.parent = &spi->dev;
	data->gpio_chip.of_node = spi->dev.of_node;
	data->gpio_chip.of_gpio_n_cells = 2;
	data->gpio_chip.of_xlate = of_gpio_simple_xlate;
	data->gpio_chip.base = -1;
	data->gpio_chip.ngpio = 8;
	data->gpio_chip.get_direction = vni_get_direction;
	data->gpio_chip.set = vni_set;

	data->val = 0x0;

	ret = devm_gpiochip_add_data(&spi->dev, &data->gpio_chip, NULL);
	if (ret) {
		dev_err(&spi->dev, "Adding GPIO chip failed\n");
		return ret;
	}

	ret = vni_send(spi, data->val);
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
