#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/spi/spi.h>

struct clt_device {
	struct spi_device *spi;
	struct gpio_chip gpio_chip;
};

static int clt_read(struct spi_device *spi, u8 *val)
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

	*val = buf[0];

	return 0;
}

static int clt_get_direction(struct gpio_chip *chip, unsigned int offset)
{
	return GPIOF_DIR_IN;
}

static int clt_get(struct gpio_chip *chip, unsigned int offset)
{
	struct clt_device *data = container_of(chip, struct clt_device, gpio_chip);
	u8 val;
	int ret;

	ret = clt_read(data->spi, &val);
	if (ret < 0)
		return ret;

	val >>= offset;
	return val & 0x1;
}

static int clt_probe(struct spi_device *spi)
{
	struct clt_device *data;
	int ret;

	spi->bits_per_word = 8;
	ret = spi_setup(spi);
	if (ret)
		return ret;

	data = devm_kzalloc(&spi->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->spi = spi;

	data->gpio_chip.parent = &spi->dev;
	data->gpio_chip.of_node = spi->dev.of_node;
	data->gpio_chip.of_gpio_n_cells = 2;
	data->gpio_chip.of_xlate = of_gpio_simple_xlate;
	data->gpio_chip.base = -1;
	data->gpio_chip.ngpio = 8;
	data->gpio_chip.get_direction = clt_get_direction;
	data->gpio_chip.get = clt_get;

	ret = devm_gpiochip_add_data(&spi->dev, &data->gpio_chip, NULL);
	if (ret) {
		dev_err(&spi->dev, "Adding GPIO chip failed\n");
		return ret;
	}

	spi_set_drvdata(spi, data);

	dev_info(&spi->dev, "CLT01-38SQ7 probed\n");

	return 0;
}

static int clt_remove(struct spi_device *spi)
{
	//struct clt_device *data = spi_get_drvdata(spi);

	//devm_gpiochip_remove(&data->gpio_chip);

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
