/*
 * control.c
 *
 *  Created on: 23 de oct. de 2016
 *      Author: Elián Hanisch, Gastón Riera y Rodrigo Oliver
 *
 *  Implementación del control.
 */

#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif

#include "control.h"
#include "pwm.h"
#include "horno.h"
#include "adc.h"

/* valores del PI de secado */
#define KPs 0.5 		    // constante proporcional
#define KIs 0.003		    // constante integrador

/* valores del PI de coccion */
#define KPc 3.5
#define KIc 0.0012

#define TS PERIODO_PROMEDIO // periodo de muestreo (segundos)
#define P_MAX 0.8
#define P_SEC 0.3333        // pendiente de la referencia durante el secado.

/* TEMP_SECADO marca el límite donde usamos un PI o el otro. Si la
 * referencia es menor a TEMP_SECADO, se utiliza el PI de secado y viceversa. */
#define TEMP_SECADO 350

/* constantes del PI discreto, usando la transformación bilineal */
static const float kx_sec = KIs*TS*0.5 + KPs, kx1_sec = KIs*TS*0.5 - KPs;
static const float kx_coc = KIc*TS*0.5 + KPc, kx1_coc = KIc*TS*0.5 - KPc;

/*
 * @brief Lazo de control PI
 * @param temperatura: temperatura del sistema.
 */
void Horno_control_pi(float temperatura) {
	if (!horno_control.activo) {
		return;
	}

	float p_max;
	if (horno_control.referencia < TEMP_SECADO) {
		p_max = P_SEC;
	} else {
		/* TODO esta pendiente debería ser configurable */
		p_max = P_MAX;
	}

	/* condicionamiento de la referencia, limitando la pendiente máxima */
	float error_ref = horno_control.referencia - horno_control.referencia_cond;
	if (error_ref > 0) {
		if (error_ref > p_max) {
			horno_control.referencia_cond += p_max;
		} else {
			horno_control.referencia_cond = horno_control.referencia;
		}
	} else {
		if (error_ref < -p_max) {
			horno_control.referencia_cond -= p_max;
		} else {
			horno_control.referencia_cond = horno_control.referencia;
		}
	}

	horno_control.entrada = horno_control.referencia_cond - temperatura;
	if (horno_control.referencia < TEMP_SECADO) {
		/* secado */
		if ((horno_control.entrada < 10) && (horno_control.entrada > -10)) {
			/* control PI */
			horno_control.salida = horno_control.entrada   * kx_sec
								 + horno_control.entrada_1 * kx1_sec
								 + horno_control.salida_1;
		} else {
			/* solo control proporcional */
			horno_control.salida = horno_control.entrada * KPs;
		}
	} else {
		/* cocción */
		horno_control.salida = horno_control.entrada   * kx_coc
							 + horno_control.entrada_1 * kx1_coc
							 + horno_control.salida_1;
	}

	/* Actualizamos el PWM
	 * Dado nuestro modelo de la planta, la salida del PI es el ciclo de
	 * trabajo del PWM. */
	Horno_pwm_ciclo(horno_control.salida / 100);

	/* actualizamos las muestras anteriores */
	horno_control.entrada_1 = horno_control.entrada;
	horno_control.salida_1 = horno_control.salida;
}

/*
 * @brief actualiza la referencia o consigna de temperatura
 * @param ref: temperatura a alcanzar.
 */
void Horno_control_referencia(float ref)
{
	horno_control.referencia = ref;
	horno_control.referencia_cond = horno_adc.temperatura;
}

/*
 * Inicialización de las variables del PI
 */
void Horno_control_activar(bool activar)
{
	if (activar) {
		/* reiniciamos las variables a cero */
		horno_control.entrada = 0;
		horno_control.entrada_1 = 0;
		horno_control.salida = 0;
		horno_control.salida_1 = 0;
	}
	horno_control.activo = activar;
}
