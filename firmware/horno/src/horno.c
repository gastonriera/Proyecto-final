/*
===============================================================================
 Name        : horno.c
 Author      : Elián Hanisch, Gastón Riera y Rodrigo Oliver
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

#include <cr_section_macros.h>

/* includes del proyecto */
#include "init.h"
#include "motor.h"
#include "320240.h"
#include "delay.h"

/* definitions and declarations here */
#define ADC_CHANNEL ADC_TH  // canal de captura del comando 'm'
#define NUM_MUESTRAS_CAPTURA 100
#define NUM_MUESTRAS_ADC 1000*PERIODO_PROMEDIO/PERIODO_MUESTREO

static bool adc_enabled = false;
static bool adc_continue = false;

static int32_t pasos;
static int32_t paso_inc = 0;
static uint32_t time_ms = 0;

static uint16_t muestras[NUM_MUESTRAS_CAPTURA];

typedef struct {
	uint32_t th_suma;			// suma accumulada del valor del AD (para hacer el promedio)
	uint32_t th_cantidad;       // cantidad de valores sumados
	uint16_t th_valor;          // valor del AD promediado
	uint32_t lm_suma;
	uint32_t lm_cantidad;
	uint16_t lm_valor;
	uint32_t valor_n;           // número del último valor obtenido
} HORNO_PROMEDIO_T;

static HORNO_PROMEDIO_T horno_adc;

/* mensaje de inicio para mandar por el UART */
static char mensaje_inicio[] =
		"\r\n"
		"Proyecto Final Horno Dental\r\n"
		"===========================\r\n"
		"\r\n";
static char mensaje_menu[] = "- Presione 'c' para iniciar/detener la captura continua.\r\n"
                             "- Presione 'm' para capturar N muestras.\r\n";

/* rutina de interrupción del systick */
void SysTick_Handler(void)
{
//	time_ms++;
//	if ((time_ms % 100) == 0) {
//		/* parpadeo de 10hz */
//		Board_LED_Toggle(0);
//	}
//	if (paso_inc == 0) {
//		/* no queremos dejar el motor clavado con una bobina siempre encendida */
//		Horno_MotorApagar();
//	} else {
//		pasos += paso_inc;
//		Horno_MotorPaso(pasos);
//	}
	if (adc_enabled) {
		if (!adc_continue) {
			if (horno_adc.valor_n > NUM_MUESTRAS_CAPTURA) {
				adc_enabled = false;
				Board_LED_Set(0, false);
			}
			Chip_ADC_SetStartMode(LPC_ADC, ADC_START_NOW, ADC_TRIGGERMODE_RISING);
			if(Chip_ADC_ReadStatus(LPC_ADC, ADC_CHANNEL, ADC_DR_DONE_STAT) == SET) {
				Chip_ADC_ReadValue(LPC_ADC, ADC_CHANNEL, &muestras[horno_adc.valor_n++]);
			}
		} else {
			/* captura continua */
			if(Chip_ADC_ReadStatus(LPC_ADC, ADC_LM35, ADC_DR_DONE_STAT) == SET) {
				uint16_t muestra;
				Chip_ADC_ReadValue(LPC_ADC, ADC_TH, &muestra);
				horno_adc.th_suma += muestra;
				horno_adc.th_cantidad++;
				Chip_ADC_ReadValue(LPC_ADC, ADC_LM35, &muestra);
				horno_adc.lm_suma += muestra;
				horno_adc.lm_cantidad++;
				if (horno_adc.lm_cantidad >= NUM_MUESTRAS_ADC) {
					Board_LED_Toggle(0);
					horno_adc.th_valor = horno_adc.th_suma / horno_adc.th_cantidad;
					horno_adc.lm_valor = horno_adc.lm_suma / horno_adc.lm_cantidad;
					DEBUGOUT("%10d, %4d, %4d\r\n", horno_adc.valor_n,
							                       horno_adc.th_valor,
							                       horno_adc.lm_valor);
					horno_adc.th_suma = 0;
					horno_adc.th_cantidad = 0;
					horno_adc.lm_suma = 0;
					horno_adc.lm_cantidad = 0;									
					horno_adc.valor_n++;
				}
			}
		}
	}
}

int main(void) {
	uint8_t charUART;
	int i;

#if defined (__USE_LPCOPEN)
#if !defined(NO_BOARD_LIB)
    // Read clock settings and update SystemCoreClock variable
	SystemCoreClockUpdate();
    // Set up and initialize all required blocks and
    // functions related to the board hardware
    Board_Init();
#endif
#endif

    /* código del horno empieza aquí */

    Horno_Init();


    DEBUGOUT(mensaje_inicio);
   	DEBUGOUT(mensaje_menu);

    while(1) {
    	charUART = DEBUGIN();
    	if (charUART == 'm') {
    		adc_enabled = true;
    		adc_continue = false;
    		horno_adc.valor_n = 0;
    		Board_LED_Set(0, true);
    		while(adc_enabled) {}
    		for (i=0; i < NUM_MUESTRAS_CAPTURA; i++) {
    			DEBUGOUT("%10d, %4d\r\n", i, muestras[i]);
    		}
    		Board_LED_Set(0, false);
    	} else if (charUART == 'c') {
    		adc_enabled = !adc_enabled;
    		adc_continue = true;
    		if (!adc_enabled) {
    			DEBUGOUT("Conversión detenida.\r\n");
    		} else {
    			horno_adc.valor_n = 0;
    			horno_adc.th_suma = 0;
    			horno_adc.th_cantidad = 0;
    			horno_adc.lm_suma = 0;
    			horno_adc.lm_cantidad = 0;
    		}
    	} else if (charUART == 'h') {
    		DEBUGOUT(mensaje_menu);
    	}
    }

    Horno_Display_Test();

    return 0;
}
