#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include <stdio.h>

// Definições de GPIO
#define BTN_DESLIGAR 5
#define BTN_5MIN 6
#define BUZZER_PIN 10

// Estados do sistema
volatile bool alarme_ativado = false;
volatile bool modo_24h = true;
volatile bool buzzer_ligado = false;
volatile uint32_t alarme_time = 0;
volatile uint32_t snooze_time = 0;
volatile int hora_alarme = 7;
volatile int minuto_alarme = 0;

// Offset de tempo em segundos (22h = 22*3600 = 79200 segundos)
volatile uint32_t offset_tempo = 25195;

// Tempo atual simulado (em segundos desde o boot)
volatile uint32_t tempo_atual() {
	return (to_ms_since_boot(get_absolute_time()) / 1000) + offset_tempo; // Adiciona o offset
}

// Configuração do PWM para o buzzer
void configurar_buzzer() {
	gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
	uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
	pwm_set_wrap(slice_num, 100);
	pwm_set_enabled(slice_num, false);
}

// Ativa/desativa o buzzer
void controlar_buzzer(bool ligar) {
	uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
	if (ligar) {
		pwm_set_enabled(slice_num, true);
		pwm_set_gpio_level(BUZZER_PIN, 50); // 50% duty cycle
	} else {
		pwm_set_enabled(slice_num, false);
	}
}

// Exibe o tempo atual no console
void exibir_tempo() {
	uint32_t segundos = tempo_atual();
	int hora = (segundos / 3600) % 24;
	int minuto = (segundos / 60) % 60;
	int segundo = segundos % 60;

	if (modo_24h) {
		printf("Tempo atual: %02d:%02d:%02d\n", hora, minuto, segundo);
	} else {
		char periodo = (hora < 12) ? 'A' : 'P';
		int hora_12h = hora % 12;
		if (hora_12h == 0) hora_12h = 12;
		printf("Tempo atual: %02d:%02d:%02d %cM\n", hora_12h, minuto, segundo, periodo);
	}
}

// Loop principal
int main() {
	stdio_init_all();
	sleep_ms(2000); // Aguarda inicialização do console

	// Configura GPIOs
	gpio_init(BTN_DESLIGAR);
	gpio_init(BTN_5MIN);
	gpio_set_dir(BTN_DESLIGAR, GPIO_IN);
	gpio_set_dir(BTN_5MIN, GPIO_IN);
	gpio_pull_up(BTN_DESLIGAR);
	gpio_pull_up(BTN_5MIN);

	configurar_buzzer();

	alarme_time = (hora_alarme * 3600) + (minuto_alarme * 60); // Adicione esta linha

	while (true) {
		uint32_t agora = tempo_atual();

		// Verifica se é hora do alarme
		if (agora >= (alarme_time + offset_tempo) && !alarme_ativado) {
			alarme_ativado = true;
			controlar_buzzer(true);
		}

		// Verifica snooze
		if (alarme_ativado && agora >= snooze_time && !buzzer_ligado) {
			controlar_buzzer(true);
			buzzer_ligado = true;
		}

		// Modo configuração
		if (!alarme_ativado) {
			if (!gpio_get(BTN_DESLIGAR)) {
				hora_alarme = (hora_alarme + 1) % (modo_24h ? 24 : 12);
				sleep_ms(200);
			}
			if (!gpio_get(BTN_5MIN)) {
				minuto_alarme = (minuto_alarme + 1) % 60;
				sleep_ms(200);
			}
		}

		// Exibe o tempo a cada segundo
		static uint32_t ultima_exibicao = 0;
		if (agora - ultima_exibicao >= 1) {
			exibir_tempo();
			ultima_exibicao = agora;
		}

		sleep_ms(100);
	}
}