/*****************************************************************************************************************************/
/* El ADC se configura para convertir el canal 0, 2 y 4 de forma que el inicio de conversión (en modo ráfaga )               */
/* se inicia periódicamente por medio del  Timer 0 en Modo Match .                                                           */
/* El ADC trabaja por interrupción entre Modo BURST y modo Normal. Se habilita la interrupción  del TIMER 0 de forma         */
/* que con el Match se lanza el modo BURST para convertir de forma ininterrumpida los tres canales en modo ráfaga.           */
/* La interrupción del PENÚLTIMO canal del ADC (ver pag. 578 del manual) deshabilita el modo BURST y lo deja en modo Normal. */
/*                                                                                                                           */
/* Con objeto de ver como muestrea el ADC cada canal se han generado tres funciones de estímulo con objeto de introducir por */
/* cada canal una señal triangular frecuencia F. Para ello bastará ejecutar desde la linea de comandos:                      */
/* >triangular_canalX(valor en Hz) seguido de enter o directamente desde los botones predefinidos.                           */
/* >triangular_canal0(1000) >triangular_canal2(2000) >triangular_canal4(4000)                                                */
/* NOTA: Para introducir nuevos valores de frecuencia es necesario abortar dicha función con el botón programado o desde la  */
/* linea de comandos con la instrucción  >kill triangular_canalX().                                                          */ 
/*                                                                                                                           */
/*****************************************************************************************************************************/


#include <LPC17xx.H>
#include <math.h>
#include <stdio.h>
#include <GLCD.h>

#define F_cpu 100e6      // Defecto Keil (xtal=12Mhz)
#define F_pclk (F_cpu/4) // Defecto despues del reset
#define F_muestreo 50 	 // Fs=50Hz (Cada 20 ms se toma una muestra del canal 0 y 2)


uint32_t canal_0, canal_1;

uint8_t nueva_muestra = 0;
char texto[20];

char luzAmb[20];

void init_ADC(void)
{	
	LPC_SC->PCONP|= (1<<12);                           // POwer ON
	LPC_PINCON->PINSEL1|= ((1<<14)|(1<<16));            // (AD0.0 y AD0.1)  (P0.23 y P0.24)
	LPC_PINCON->PINMODE1|= ((2<<14)|(2<<16));           // Deshabilita pullup/pulldown
	LPC_SC->PCLKSEL0|= (0x00<<8);                      // CCLK/4 (Fpclk después del reset) (100 MHz/4 = 25MHz)
	LPC_ADC->ADCR|=(1<<0)|(1<<1)                				// canales 0,1
											| (0x01<<8)                   // CLKDIV=1   (Fclk_ADC=25MHz /(1+1)= 12.5MHz)
											| (0x01<<21);                 // PDN=1
	// Ojo no se habilita el modo BURST en la inicialización  
	LPC_ADC->ADINTEN=(1<<1);					// Hab. interrupción fin de conversión del ÚLTIMO canal(canal 1)
	NVIC_EnableIRQ(ADC_IRQn);					// Hab. interrupción del ADC
	NVIC_SetPriority(ADC_IRQn,1);	    // ISR del ADC prioridad 1
}	


/* Timer 1 interrumpe cada 1/Fs */
void TIMER1_IRQHandler(void)
{	
	LPC_TIM1->IR|=(1<<0);   // Borrar flag interrupción								
	LPC_ADC->ADCR|=(1<<16); // BURST=1 --> Cada 65TclkADC se toma una muestra de cada canal comenzando 
                          //             desde el más bajo (bit LSB de CR[0..7])
}


// Con la interrupción del PENÚLTIMO canal deshabilitamos la Ráfaga 
// Ojo!!!! la conversión del siguiente canal (último) sigue en curso --> Ver Pag. 578 del manual
void ADC_IRQHandler(void)
{	
	LPC_ADC->ADCR&=~(1<<16); // BURST=0     // Deshabilitamos el modo Ráfaga 
																					// (ojo continua la conversión del siguiente canal) 
	
	//Almacenamos las muestras a modo de ejemplo
	canal_0= ((LPC_ADC->ADDR0 >>4)&0xFFF);	// flag DONE se borra automat. al leer ADDR0
	canal_1= ((LPC_ADC->ADDR1 >>4)&0xFFF);	// flag DONE se borra automat. al leer ADDR1
	nueva_muestra = 1;
}


/*  Timer 1 en modo Output Compare (reset T1TC on Match 0)
	Counter clk: 25 MHz 	MAT1.0 : On match, Toggle pin/output (P1.29) para ver el instante de muestreo
  En cada Match interumpe, es decir con un periodo Ts= Fpclk/(MR1+1) y se habilita el modo Burst (Rafaga) 
	para los canales 0, 2, 4. 
	Habilitamos la salida (MAT0.1) para observar el instante en que se lanza la ráfaga 
	(ojo!! con cada flanco de esta señal)*/
						
void init_TIMER1(void)
{
	LPC_SC->PCONP|= (1<<2);                 	// TIMER1 power on
	LPC_PINCON->PINSEL3 |= (3 << 12);					// P1.22 => MAT1.0
	LPC_PINCON->PINMODE3 |= (1 << 13);				// MAT1.0 neither pull-up/down.
	LPC_TIM1->MCR = (3 << 0);              		// Interrupt on Match 0 and Reset TC
	LPC_TIM1->MR0 = (F_pclk/2/F_muestreo)-1;	// Periodo de muestreo de TODAS las entradas!!!! 
	LPC_TIM1->EMR |= (3 << 4);						 		// MAT1.0 toggle on match
	LPC_TIM1->TCR = 0x01;                  		// TC1 on
		
	NVIC_EnableIRQ(TIMER1_IRQn);            	// Hab. interrupción del TIMER1
	NVIC_SetPriority(TIMER1_IRQn,2);        	// ISR del TIMER1 con prioridad 2
}	


int main(void)
{
	uint16_t tam12, tam8, mV;
	uint16_t mVLuz, luz;
	
	NVIC_SetPriorityGrouping(2);
	LCD_Initializtion();
	LCD_Clear(Black);
	GUI_Text(20, 70, (uint8_t *)"Codigo 12 bits:", White, Black);
	GUI_Text(20, 100, (uint8_t *)"Codigo 8 bits:", White, Black);
	GUI_Text(20, 130, (uint8_t *)"Tension en mV:", White, Black);
	GUI_Text(20, 160, (uint8_t *)"Luminosidad:", White, Black);

	
	init_ADC();
	init_TIMER1();
	while(1)
	{
		if (nueva_muestra)
		{
			nueva_muestra = 0;			// Borrar flag de muestra
			
			tam12 = canal_0;				// EL valor leido en ADC_IRQ es de 12 bits directamente
			tam8 = (tam12 >> 4);		// Nos quedamos con lso 8 bits de mayor peso
			mV = (uint32_t)(tam12 * 3300/4095);		// Conversion del valor digital leido a análogico
			
			sprintf(texto, "%4d", tam12);
			GUI_Text(150, 70, (uint8_t *)texto, Yellow, Black);
			sprintf(texto, "%4d", tam8);
			GUI_Text(150, 100, (uint8_t *)texto, Yellow, Black);
			sprintf(texto, "%4d mV", mV);
			GUI_Text(150, 130, (uint8_t *)texto, Yellow, Black);
			
			mVLuz = (uint32_t)(canal_1 * 3300/4095);
			luz = (uint32_t)(mVLuz * 100/3300);
			sprintf(luzAmb, "%4d %%", luz);
			GUI_Text(150, 160, (uint8_t *)luzAmb, Yellow, Black);
		}
	}
}

