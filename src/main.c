/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/shell/shell.h>
#include <inttypes.h>
#include <string.h>



#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

#define ARCH_STACK_PTR_ALIGN 8
#define CONFIG_SYS_CLOCK_TICKS_PER_SEC 1000


/* size of stack area used by each thread */
#define STACKSIZE 1024


/* scheduling priority used by each thread */
#define PRIORITY 7

#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)

#if !DT_NODE_HAS_STATUS(LED0_NODE, okay)
#error "Unsupported board: led0 devicetree alias is not defined"
#endif


#if !DT_NODE_HAS_STATUS(LED1_NODE, okay)
#error "Unsupported board: led1 devicetree alias is not defined"
#endif


struct led {
	struct gpio_dt_spec spec;
	uint8_t num;
};

static const struct led led0 = {
	.spec = GPIO_DT_SPEC_GET_OR(LED0_NODE, gpios, {0}),
	.num = 0,
};

void blink(const struct led *led, uint32_t sleep_ms, uint32_t id)
{
	const struct gpio_dt_spec *spec = &led->spec;
	int ret;
	int cnt = 0;

	if (!device_is_ready(spec->port)) {
		printk("Error: %s device is not ready\n", spec->port->name);
		return;
	}

	ret = gpio_pin_configure_dt(spec, GPIO_OUTPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure pin %d (LED '%d')\n",
			ret, spec->pin, led->num);
		return;
	}

	while (1) {
		gpio_pin_set(spec->port, spec->pin, cnt % 2);
		cnt++;

		k_msleep(sleep_ms);
	}
}

void blink0(struct k_work *)
{
	blink(&led0, 200, 0);
}

static struct k_work blink_work;

// void blink_work_handler(struct k_work *work) {
//     static int cnt = 0;

//     // Alterna o estado do LED
//     gpio_pin_set(led0.spec.port, led0.spec.pin, cnt % 2);
//     cnt++;

//     // Reagenda a si mesma para continuar piscando
// 		k_work_submit(&blink_work);

// }


//K_THREAD_DEFINE(blink0_id, STACKSIZE, blink0, NULL, NULL, NULL,PRIORITY, 0, 0);

// ------------------ start button ------------------

/* debounce time */
#define DEBOUNCE_TIME_MS 50

#define SLEEP_TIME_MS	1

/*
 * Get button configuration from the devicetree sw0 alias. This is mandatory.
 */
#define SW0_NODE	DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios,
							      {0});
static struct gpio_callback button_cb_data;


static int button_state = -1;  // estado atual do botão
static int64_t last_change_time = 0;  // tempo da última mudança de estado


/*
 * The led0 devicetree alias is optional. If present, we'll use it
 * to turn on the LED whenever the button is pressed.
 */
// static struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios,
// 						     {0});

void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
		int64_t now = k_uptime_get();
    int new_state = gpio_pin_get(dev, button.pin);

    // Verifica se o estado do botão mudou e se passou tempo suficiente desde a última mudança
    if (new_state != button_state && now - last_change_time > DEBOUNCE_TIME_MS) {
        printk("Button state changed to %d at %" PRIu32 "\n", new_state, k_cycle_get_32());
        button_state = new_state;
        last_change_time = now;

				printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());

				button_state = 0;

    }
}

// ------------------ end button ------------------

static void button_init(void){
	int ret;

	// Configura o pino do botão como entrada
	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure pin %d '%s'\n",
			ret, button.pin, button.port->name);
		return;
	}

	// Configura o pino do botão com resistor de pull-up
	ret = gpio_pin_configure_dt(&button, GPIO_INPUT | GPIO_PULL_UP);
	if (ret != 0) {
		printk("Error %d: failed to configure pin %d '%s'\n",
			ret, button.pin, button.port->name);
		return;
	}

	// Configura o pino do botão com interrupção
	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_BOTH);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on pin %d '%s'\n",
			ret, button.pin, button.port->name);
		return;
	}

	// Configura a função de callback para o botão
	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));

	// Associa a função de callback ao pino do botão
	ret = gpio_add_callback(button.port, &button_cb_data);
	if (ret != 0) {
		printk("Error %d: failed to add callback for pin %d '%s'\n",
			ret, button.pin, button.port->name);
		return;
	}

	// Habilita as interrupções para o pino do botão
	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_BOTH);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on pin %d '%s'\n",
			ret, button.pin, button.port->name);
		return;
	}
}




// K_THREAD_DEFINE(blink0_id, STACKSIZE, blink0, NULL, NULL, NULL,PRIORITY, 0, 0);
K_THREAD_DEFINE(button_id, STACKSIZE, button_init, NULL, NULL, NULL,PRIORITY, 0, 0);


K_WORK_DEFINE(blink0_id, blink0);


// ------------------- SHELL -------------------

// Função que realiza a FFT
void perform_fft(double val1, double val2) {
    // Aqui você colocaria o código para realizar a FFT nos valores
    printk("Realizando FFT em %f e %f\n", val1, val2);

    // Exemplo: imprimir valores fictícios de FFT
    printk("Resultado da FFT: [%.2f, %.2f]\n", val1 * 2, val2 * 2);
}

// Callback para o comando de shell
static int cmd_fft(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 3) {
        printk("Uso: fft <val1> <val2>");
        return -EINVAL;
    }

		shell_print(shell, "Executando FFT em %s e %s", argv[1], argv[2]);

    double val1 = strtod(argv[1], NULL);
    double val2 = strtod(argv[2], NULL);

    perform_fft(val1, val2);

		k_msleep(200);

    return 0;
}

// SHELL_CMD_ARG_REGISTER(fft, NULL, "Executa FFT em dois valores", cmd_fft, 1, 0);

// Registro do comando de shell
SHELL_STATIC_SUBCMD_SET_CREATE(sub_fft,
    SHELL_CMD(fft, NULL, "Executa FFT em dois valores", cmd_fft),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(fft, &sub_fft, "Comandos de FFT", NULL);




// ------------------- SHELL -------------------

int main(void)
{
	// ------------------ start button ------------------



	// Submete a função de trabalho para sua primeira execução
	k_work_submit(&blink0_id);


	// ------------------- end button -------------------

	return 0;
}
