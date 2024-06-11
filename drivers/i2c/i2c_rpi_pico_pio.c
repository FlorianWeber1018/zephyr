/*
 * Copyright 2024 Google LLC
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_pico_i2c_pio

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_pico_pio);

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/drivers/misc/pio_rpi_pico/pio_rpi_pico.h>

#include "i2c_rpi_pico_pio.h"

/** Working data for the device */
struct i2c_pio_config {
	struct gpio_dt_spec scl_gpio;
	struct gpio_dt_spec sda_gpio;
	uint32_t bitrate;
};

/* Driver instance data */
struct i2c_pio_context {
};

// --- //
// i2c //
// --- //

#define i2c_offset_entry_point 12u

RPI_PICO_PIO_DEFINE_PROGRAM(i2c, 12, 17,
			    0x008c, //  0: jmp    y--, 12
			    0xc030, //  1: irq    wait 0 rel
			    0xe027, //  2: set    x, 7
			    0x6781, //  3: out    pindirs, 1             [7]
			    0xba42, //  4: nop                    side 1 [2]
			    0x24a1, //  5: wait   1 pin, 1               [4]
			    0x4701, //  6: in     pins, 1                [7]
			    0x1743, //  7: jmp    x--, 3          side 0 [7]
			    0x6781, //  8: out    pindirs, 1             [7]
			    0xbf42, //  9: nop                    side 1 [7]
			    0x27a1, // 10: wait   1 pin, 1               [7]
			    0x12c0, // 11: jmp    pin, 0          side 0 [2]
				    //     .wrap_target
			    0x6026, // 12: out    x, 6
			    0x6041, // 13: out    y, 1
			    0x0022, // 14: jmp    !x, 2
			    0x6060, // 15: out    null, 32
			    0x60f0, // 16: out    exec, 16
			    0x0050, // 17: jmp    x--, 16
				    //     .wrap
);

// ----------- //
// set_scl_sda //
// ----------- //

RPI_PICO_PIO_DEFINE_PROGRAM(set_scl_sda, 0, 3,
			    //     .wrap_target
			    0xf780, //  0: set    pindirs, 0      side 0 [7]
			    0xf781, //  1: set    pindirs, 1      side 0 [7]
			    0xff80, //  2: set    pindirs, 0      side 1 [7]
			    0xff81, //  3: set    pindirs, 1      side 1 [7]
				    //     .wrap
);

static int i2c_pio_configure(const struct device *dev)
{
	/*
	       assert(pin_scl == pin_sda + 1);

	       pio_sm_config c = i2c_program_get_default_config(offset);
		// IO mapping
		sm_config_set_out_pins(&c, pin_sda, 1);
		sm_config_set_set_pins(&c, pin_sda, 1);
		sm_config_set_in_pins(&c, pin_sda);
		sm_config_set_sideset_pins(&c, pin_scl);
		sm_config_set_jmp_pin(&c, pin_sda);
		sm_config_set_out_shift(&c, false, true, 16);
		sm_config_set_in_shift(&c, false, true, 8);
		float div = (float)clock_get_hz(clk_sys) / (32 * 100000);
		sm_config_set_clkdiv(&c, div);
		// Try to avoid glitching the bus while connecting the IOs. Get things set
		// up so that pin is driven down when PIO asserts OE low, and pulled up
		// otherwise.
		gpio_pull_up(pin_scl);
		gpio_pull_up(pin_sda);
		uint32_t both_pins = (1u << pin_sda) | (1u << pin_scl);
		pio_sm_set_pins_with_mask(pio, sm, both_pins, both_pins);
		pio_sm_set_pindirs_with_mask(pio, sm, both_pins, both_pins);
		pio_gpio_init(pio, pin_sda);
		gpio_set_oeover(pin_sda, GPIO_OVERRIDE_INVERT);
		pio_gpio_init(pio, pin_scl);
		gpio_set_oeover(pin_scl, GPIO_OVERRIDE_INVERT);
		pio_sm_set_pins_with_mask(pio, sm, 0, both_pins);
		// Clear IRQ flag before starting, and make sure flag doesn't actually
		// assert a system-level interrupt (we're using it as a status flag)
		pio_set_irq0_source_enabled(pio, (enum pio_interrupt_source)((uint)pis_interrupt0 +
	   sm), false); pio_set_irq1_source_enabled(pio, (enum
	   pio_interrupt_source)((uint)pis_interrupt0 + sm), false); pio_interrupt_clear(pio, sm);
		// Configure and start SM
		pio_sm_init(pio, sm, offset + i2c_offset_entry_point, &c);
		pio_sm_set_enabled(pio, sm, true);*/
	return 0;
}

static int i2c_pio_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			    uint16_t addr)
{
	return 0;
}

static const struct i2c_driver_api i2c_pio_api = {
	.transfer = i2c_pio_transfer,
};

static int i2c_pio_init(const struct device *dev)
{
	const struct i2c_pico_pio_config *dev_cfg = dev->config;
	// const struct i2c_pio_context *dev_cfg = dev->data;

	return i2c_pio_configure(dev);
};

#define DEFINE_I2C_PIO(_num)                                                                       \
                                                                                                   \
	static struct i2c_pio_context i2c_pio_dev_data_##_num;                                     \
                                                                                                   \
	static const struct i2c_pio_config i2c_pio_dev_cfg_##_num = {                              \
		.scl_gpio = GPIO_DT_SPEC_INST_GET(_num, scl_gpios),                                \
		.sda_gpio = GPIO_DT_SPEC_INST_GET(_num, sda_gpios),                                \
		.bitrate = DT_INST_PROP(_num, clock_frequency),                                    \
	};                                                                                         \
                                                                                                   \
	I2C_DEVICE_DT_INST_DEFINE(_num, i2c_pio_init, NULL, &i2c_pio_dev_data_##_num,              \
				  &i2c_pio_dev_cfg_##_num, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,  \
				  &i2c_pio_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_I2C_PIO)
bool test = DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT);