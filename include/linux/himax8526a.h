#ifndef HIMAX8526a_H
#define HIMAX8526a_H
#include <linux/types.h>
#include <linux/i2c.h>

#define HIMAX8526A_NAME "Himax8526a"

struct himax_config_init_api {
	int (*i2c_himax_master_write)(struct i2c_client *client, uint8_t *data, uint8_t length);
	int (*i2c_himax_write_command)(struct i2c_client *client, uint8_t command);
	int (*i2c_himax_read_command)(struct i2c_client *client, uint8_t length, uint8_t *data, uint8_t *readlength);
};

struct himax_i2c_platform_data {
	uint8_t version;
	int abs_x_min;
	int abs_x_max;
	int abs_y_min;
	int abs_y_max;
	int abs_pressure_min;
	int abs_pressure_max;
	int abs_width_min;
	int abs_width_max;
	int (*power)(int on);
	int gpio_irq;
	uint8_t tw_id;
	uint8_t slave_addr;
	uint32_t event_htc_enable;
	int (*loadSensorConfig)(struct i2c_client *client, struct himax_i2c_platform_data **pdata, struct himax_config_init_api *i2c_api);
	uint8_t cable_config[2];
	void (*reset)(void);
	uint8_t c1[3];
	uint8_t c2[3];
	uint8_t c3[5];
	uint8_t c4[2];
	uint8_t c5[2];
	uint8_t c6[2];
	uint8_t c7[3];
	uint8_t c8[2];
	uint8_t c9[4];
	uint8_t c10[6];
	uint8_t c11[15];
	uint8_t c12[5];
	uint8_t c13[8];
	uint8_t c14[4];
	uint8_t c15[11];
	uint8_t c16[4];
	uint8_t c17[2];
	uint8_t c18[11];
	uint8_t c19[11];
	uint8_t c20[11];
	uint8_t c21[11];
	uint8_t c22[11];
	uint8_t c23[11];
	uint8_t c24[11];
	uint8_t c25[11];
	uint8_t c26[11];
	uint8_t c27[11];
	uint8_t c28[11];
	uint8_t c29[11];
	uint8_t c30[23];
	uint8_t c31[49];
	uint8_t c32[10];
	uint8_t c33[2];
	uint8_t c34[5];
	uint8_t c35[5];
	uint8_t c36[8];
	/*uint8_t c37[3];*/
	uint8_t c38[2];
	/*uint8_t c39[5];*/
	uint8_t c40[5];
	uint8_t c41[2];
	uint8_t c42[5];
	uint8_t c43[6];
	uint8_t c44[6];
	uint8_t c45[2];
	uint8_t c46[2];
	uint8_t c47[3];
	uint8_t c48[2];
	uint8_t enterLeave[3]; /* 3 parameters */
	uint8_t averageDistance[5]; /* 5 parameters */
};
#endif

