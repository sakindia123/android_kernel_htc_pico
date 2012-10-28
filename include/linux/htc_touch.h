#ifndef __HTC_TOUCH_H
#define __HTC_TOUCH_H

enum {
	TP_INIT	= 1,
	TP_SUSPEND,
	TP_RESUME
};

struct virtkey_attr {
	unsigned char panel_id;
	int keycode;
	int x, y, w, h;
};

struct touch_devconfig {
	const char *type;
	unsigned int id;
	unsigned int version;
	void *config;
	size_t size;
};

#define TOUCH_DEVCONFIG(type, id, version, config) \
	{ type, id, version, config, ARRAY_SIZE(config) }

struct touch_platform_data {
	int abs_x_min;
	int abs_x_max;
	int abs_y_min;
	int abs_y_max;
	int abs_p_min;
	int abs_p_max;
	int abs_w_min;
	int abs_w_max;
	int abs_z_min;
	int abs_z_max;

	const char *vendor;
	int (*power)(int on);
	void (*pin_config)(int state);
	struct touch_devconfig *(*get_devconfig)(unsigned int fw_version,
						unsigned char panel_id);
	int gpio_irq;
	int gpio_reset;
	int i2c_retries;
	uint16_t version;
	unsigned char log_level;
	unsigned irq_wakeup : 1;
	unsigned polling_mode : 1;
	unsigned trigger_low : 1;
};

/* Himax Touch Panel */
struct himax_cmd_size {
	uint8_t *cmd;
	size_t size;
};

#define CS(a) { .cmd = (a), .size = ARRAY_SIZE(a) }

#endif
