// SPDX-License-Identifier: GPL-2.0
/*
 * ledat_demo.c — driver I2C minh hoạ: parse toàn bộ property trong
 * ledat-demo-overlay.dts (số, mảng, chuỗi, boolean, gpio, irq, clock,
 * regulator, phandle).
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/gpio/consumer.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

struct demo_data {
	struct i2c_client	*client;
	struct gpio_desc	*reset_gpio;
	struct clk		*ext_clk;
	struct regulator	*vdd;
	struct device_node	*aux_np;
	u32			sample_rate_hz;
	u32			thresholds[3];
	const char		*label;
	bool			wakeup_source;
};

static irqreturn_t demo_irq_handler(int irq, void *data)
{
	struct demo_data *ddata = data;

	dev_info(&ddata->client->dev, "ngắt xảy ra trên IRQ %d\n", irq);
	return IRQ_HANDLED;
}

/* Đọc toàn bộ property trong node DT — mỗi khối ứng với 1 hàng trong
 * bảng so sánh: DT khai gì -> gọi hàm nào.
 */
static int demo_parse_dt(struct demo_data *ddata)
{
	struct device_node *np = ddata->client->dev.of_node;
	struct device *dev = &ddata->client->dev;
	int ret;

	if (!np)
		return -ENODEV;

	/* --- số nguyên đơn --- */
	ddata->sample_rate_hz = 500; /* mặc định nếu DT không khai */
	of_property_read_u32(np, "sample-rate-hz", &ddata->sample_rate_hz);

	/* --- mảng số (đã zero sẵn nhờ devm_kzalloc ở probe) --- */
	ret = of_property_read_u32_array(np, "threshold-levels",
					  ddata->thresholds, 3);
	if (ret)
		dev_warn(dev, "không có threshold-levels, dùng mặc định {0,0,0}\n");

	/* --- chuỗi --- */
	ret = of_property_read_string(np, "label", &ddata->label);
	if (ret)
		ddata->label = "unnamed";

	/* --- boolean --- */
	ddata->wakeup_source = of_property_read_bool(np, "wakeup-source");

	/* --- gpio --- */
	ddata->reset_gpio = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ddata->reset_gpio))
		return PTR_ERR(ddata->reset_gpio);

	/* --- clock --- */
	ddata->ext_clk = devm_clk_get_optional(dev, "ext-clk");
	if (IS_ERR(ddata->ext_clk))
		return PTR_ERR(ddata->ext_clk);

	/* --- regulator --- */
	ddata->vdd = devm_regulator_get_optional(dev, "vdd");
	if (IS_ERR(ddata->vdd)) {
		if (PTR_ERR(ddata->vdd) == -ENODEV)
			ddata->vdd = NULL; /* DT không khai vdd-supply, chấp nhận được */
		else
			return PTR_ERR(ddata->vdd); /* lỗi thật, có thể -EPROBE_DEFER */
	}

	/* --- phandle tổng quát --- */
	ddata->aux_np = of_parse_phandle(np, "ledat,aux-phandle", 0);

	/* client->irq đã được I2C core tự điền sẵn từ property "interrupts",
	 * không cần gọi of_irq_get() thủ công ở đây.
	 */
	return 0;
}

static int demo_probe(struct i2c_client *client)
{
	struct demo_data *ddata;
	int ret;

	ddata = devm_kzalloc(&client->dev, sizeof(*ddata), GFP_KERNEL);
	if (!ddata)
		return -ENOMEM;

	ddata->client = client;
	i2c_set_clientdata(client, ddata);

	ret = demo_parse_dt(ddata);
	if (ret)
		return ret;

	if (ddata->reset_gpio) {
		gpiod_set_value_cansleep(ddata->reset_gpio, 1);
		usleep_range(1000, 2000);
		gpiod_set_value_cansleep(ddata->reset_gpio, 0);
	}

	if (ddata->ext_clk) {
		ret = clk_prepare_enable(ddata->ext_clk);
		if (ret)
			return ret;
	}

	if (ddata->vdd) {
		ret = regulator_enable(ddata->vdd);
		if (ret)
			goto err_clk;
	}

	if (client->irq > 0) {
		ret = devm_request_threaded_irq(&client->dev, client->irq, NULL,
						 demo_irq_handler, IRQF_ONESHOT,
						 "ledat-demo", ddata);
		if (ret)
			goto err_regulator;
	}

	dev_info(&client->dev,
		 "probe OK — label=%s sample_rate=%uHz thresholds={%u,%u,%u} wakeup=%d aux=%s irq=%d\n",
		 ddata->label, ddata->sample_rate_hz,
		 ddata->thresholds[0], ddata->thresholds[1], ddata->thresholds[2],
		 ddata->wakeup_source,
		 ddata->aux_np ? ddata->aux_np->full_name : "(none)",
		 client->irq);

	return 0;

err_regulator:
	if (ddata->vdd)
		regulator_disable(ddata->vdd);
err_clk:
	if (ddata->ext_clk)
		clk_disable_unprepare(ddata->ext_clk);
	return ret;
}

static void demo_remove(struct i2c_client *client)
{
	struct demo_data *ddata = i2c_get_clientdata(client);

	if (ddata->vdd)
		regulator_disable(ddata->vdd);
	if (ddata->ext_clk)
		clk_disable_unprepare(ddata->ext_clk);
	if (ddata->aux_np)
		of_node_put(ddata->aux_np);
}

static const struct of_device_id demo_of_match[] = {
	{ .compatible = "ledat,demo-dev" },
	{ }
};
MODULE_DEVICE_TABLE(of, demo_of_match);

static const struct i2c_device_id demo_id[] = {
	{ "demo-dev", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, demo_id);

static struct i2c_driver demo_driver = {
	.driver = {
		.name           = "ledat-demo",
		.of_match_table = demo_of_match,
	},
	.probe    = demo_probe,
	.remove   = demo_remove,
	.id_table = demo_id,
};
module_i2c_driver(demo_driver);

MODULE_AUTHOR("LeDat");
MODULE_DESCRIPTION("Vi du driver I2C parse toan bo property tu device tree");
MODULE_LICENSE("GPL");
