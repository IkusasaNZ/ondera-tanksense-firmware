#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <hal/nrf_gpio.h>

#define USER DT_PATH(zephyr_user)

static const struct gpio_dt_spec eg800q_pwr = GPIO_DT_SPEC_GET(USER, eg800q_pwr_en_gpios);
static const struct gpio_dt_spec radar_pwr  = GPIO_DT_SPEC_GET(USER, radar_pwr_en_gpios);
static const struct gpio_dt_spec ls_oe      = GPIO_DT_SPEC_GET(USER, level_shifter_oe_gpios);
static const struct gpio_dt_spec vbat_en    = GPIO_DT_SPEC_GET(USER, vbatt_sense_en_gpios);

/* Bring-up safety net: runs before anything else in Zephyr.
 * 1) Force power enables OFF (no pull-downs fitted on this board rev).
 * 2) Busy-wait 10 s so a debugger can always attach after reset. */
static int bringup_safe_start(void)
{
	nrf_gpio_pin_clear(NRF_GPIO_PIN_MAP(1, 11));   /* EG800Q_PWR_EN */
	nrf_gpio_cfg_output(NRF_GPIO_PIN_MAP(1, 11));
	nrf_gpio_pin_clear(NRF_GPIO_PIN_MAP(1, 10));   /* RADAR_PWR_EN  */
	nrf_gpio_cfg_output(NRF_GPIO_PIN_MAP(1, 10));

	k_busy_wait(10 * 1000 * 1000);                 /* 10 s debug window */
	return 0;
}
SYS_INIT(bringup_safe_start, PRE_KERNEL_1, 0);

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