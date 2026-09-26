#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <hal/nrf_gpio.h>
#include <SEGGER_RTT.h>
#include <string.h>
#include <stdio.h>

#define USER DT_PATH(zephyr_user)

static const struct gpio_dt_spec eg800q_pwr = GPIO_DT_SPEC_GET(USER, eg800q_pwr_en_gpios);
static const struct gpio_dt_spec radar_pwr  = GPIO_DT_SPEC_GET(USER, radar_pwr_en_gpios);
static const struct gpio_dt_spec ls_oe      = GPIO_DT_SPEC_GET(USER, level_shifter_oe_gpios);
static const struct gpio_dt_spec vbat_en    = GPIO_DT_SPEC_GET(USER, vbatt_sense_en_gpios);
static const struct gpio_dt_spec eg_status  = GPIO_DT_SPEC_GET(USER, eg800q_status_gpios);

static const struct device *const modem = DEVICE_DT_GET(DT_NODELABEL(uart0));

/* Bring-up safety net: runs before anything else in Zephyr. */
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

static void rtt_puts(const char *s)
{
	SEGGER_RTT_WriteString(0, s);
}

/* EG800Q -> PC: forward every received byte to RTT */
static void modem_isr(const struct device *dev, void *user_data)
{
	uint8_t buf[64];

	if (!uart_irq_update(dev)) {
		return;
	}
	while (uart_irq_rx_ready(dev)) {
		int n = uart_fifo_read(dev, buf, sizeof(buf));
		if (n <= 0) {
			break;
		}
		SEGGER_RTT_Write(0, buf, n);
	}
}

static void modem_send_line(const char *line)
{
	for (const char *p = line; *p; p++) {
		uart_poll_out(modem, *p);
	}
	uart_poll_out(modem, '\r');
}

static void local_command(const char *cmd)
{
	char msg[64];

	if (strcmp(cmd, "!vbat on") == 0) {
		gpio_pin_set_dt(&eg800q_pwr, 1);
		rtt_puts("[nrf] EG800Q VBAT ON - now pulse PWRKEY (U6 pin 4) ~1 s\r\n");
	} else if (strcmp(cmd, "!vbat off") == 0) {
		gpio_pin_set_dt(&ls_oe, 0);
		gpio_pin_set_dt(&eg800q_pwr, 0);
		rtt_puts("[nrf] OE OFF, EG800Q VBAT OFF\r\n");
	} else if (strcmp(cmd, "!oe on") == 0) {
		gpio_pin_set_dt(&ls_oe, 1);
		rtt_puts("[nrf] Level shifter ON (only if VDD_EXT = 1.8 V!)\r\n");
	} else if (strcmp(cmd, "!oe off") == 0) {
		gpio_pin_set_dt(&ls_oe, 0);
		rtt_puts("[nrf] Level shifter OFF\r\n");
	} else if (strcmp(cmd, "!status") == 0) {
		snprintf(msg, sizeof(msg), "[nrf] STATUS pin = %d (valid only with OE on)\r\n",
			 gpio_pin_get_dt(&eg_status));
		rtt_puts(msg);
	} else {
		rtt_puts("[nrf] Commands: !vbat on|off, !oe on|off, !status, !help\r\n"
			 "[nrf] Anything else is sent to the EG800Q as an AT command.\r\n");
	}
}

int main(void)
{
	char line[128];
	size_t len = 0;
	char c;

	/* Safe state first: everything OFF */
	gpio_pin_configure_dt(&eg800q_pwr, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&radar_pwr,  GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&ls_oe,      GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&vbat_en,    GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&eg_status,  GPIO_INPUT);

	if (!device_is_ready(modem)) {
		rtt_puts("[nrf] ERROR: modem UART not ready\r\n");
		return 0;
	}
	uart_irq_callback_set(modem, modem_isr);
	uart_irq_rx_enable(modem);

	rtt_puts("\r\n[nrf] TankSense EG800Q bridge ready. Type !help\r\n");

	/* PC -> nRF: line-buffered. '!' lines are local, others go to the EG800Q */
	while (1) {
		if (SEGGER_RTT_Read(0, &c, 1) == 1) {
			if (c == '\r' || c == '\n') {
				if (len > 0) {
					line[len] = '\0';
					if (line[0] == '!') {
						local_command(line);
					} else {
						modem_send_line(line);
					}
					len = 0;
				}
			} else if (len < sizeof(line) - 1) {
				line[len++] = c;
			}
		} else {
			k_msleep(2);
		}
	}
	return 0;
}