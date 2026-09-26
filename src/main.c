#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define USER DT_PATH(zephyr_user)

static const struct gpio_dt_spec eg800q_pwr = GPIO_DT_SPEC_GET(USER, eg800q_pwr_en_gpios);
static const struct gpio_dt_spec radar_pwr  = GPIO_DT_SPEC_GET(USER, radar_pwr_en_gpios);
static const struct gpio_dt_spec ls_oe      = GPIO_DT_SPEC_GET(USER, level_shifter_oe_gpios);
static const struct gpio_dt_spec vbat_en    = GPIO_DT_SPEC_GET(USER, vbatt_sense_en_gpios);

int main(void)
{
	/* Safe state first: everything OFF */
	gpio_pin_configure_dt(&eg800q_pwr, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&radar_pwr,  GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&ls_oe,      GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&vbat_en,    GPIO_OUTPUT_INACTIVE);

	/* Bring-up test: radar LDO on/off every 2 s -> watch TP4 */
	while (1) {
		gpio_pin_toggle_dt(&radar_pwr);
		k_msleep(2000);
	}
	return 0;
}