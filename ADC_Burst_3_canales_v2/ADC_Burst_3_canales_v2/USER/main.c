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
#include <Math.h>
#define F_cpu 100e6      // Defecto Keil (xtal=12Mhz)
#define F_pclk (F_cpu/4) // Defecto despues del reset
#define F_muestreo 50000 // Fs=50kHz (Cada 20 us se toma una muestra del canal 0, 2 y 4 )


uint32_t canal_0, canal_2, canal_4;



void init_ADC(void)
{	
   LPC_SC->PCONP|= (1<<12);                           // POwer ON
   LPC_PINCON->PINSEL1|= ((1<<14)|(1<<18));            // (AD0.0 y AD0.2)  (P0.23 y P0.25)
   LPC_PINCON->PINMODE1|= ((2<<14)|(2<<18));           // Deshabilita pullup/pulldown
   LPC_PINCON->PINSEL3|= (3<<28);                     // (AD0.4) (P1.30)
   LPC_PINCON->PINMODE3|= (2<<28);                    // Deshabilita pullup/pulldown
   LPC_SC->PCLKSEL0|= (0x00<<8);                      // CCLK/4 (Fpclk después del reset) (100 Mhz/4 = 25Mhz)
   LPC_ADC->ADCR|=(1<<0)|(1<<2)|(1<<4)                // canales 0,2,4
                        | (0x01<<8)                   // CLKDIV=1   (Fclk_ADC=25Mhz /(1+1)= 12.5Mhz)
                        | (0x01<<21);                 // PDN=1
   // Ojo no se habilita el modo BURST en la inicialización  
   LPC_ADC->ADINTEN=(1<<2);					// Hab. interrupción fin de conversión del PENÚLTIMO canal(canal 2)
   NVIC_EnableIRQ(ADC_IRQn);					// 
   NVIC_SetPriority(ADC_IRQn,2);			   //         
}	


/* Timer 0 interrumpe cada 1/Fs */
void TIMER0_IRQHandler(void)
{	
   LPC_ADC->ADCR|=(1<<16); // BURST=1 --> Cada 65TclkADC se toma una muestra de cada canal comenzando 
                           //             desde el más bajo (bit LSB de CR[0..7])
   LPC_TIM0->IR|=(1<<1);   // Borrar flag interrupción								
}


// Con la interrupción del PENÚLTIMO canal deshabilitamos la Ráfaga 
// Ojo!!!! la conversión del siguiente canal (último) sigue en curso --> Ver Pag. 578 del manual
void ADC_IRQHandler(void)
{	
	LPC_ADC->ADCR&=~(1<<16); // BURST=0     // Deshabilitamos el modo Ráfaga (ojo continua la conversión del siguiente canal) 
  
   //Almacenamos las muestras a modo de ejemplo
   canal_0= ((LPC_ADC->ADDR0 >>4)&0xFFF);	// flag DONE se borra automat. al leer ADDR0
   canal_2= ((LPC_ADC->ADDR2 >>4)&0xFFF);	// flag DONE se borra automat. al leer ADDR2
   canal_4= ((LPC_ADC->ADDR4 >>4)&0xFFF);	// flag DONE se borra automat. al leer ADDR4
}


/*  Timer 0 en modo Output Compare (reset T0TC on Match 1)
	Counter clk: 25 MHz 	MAT0.1 : On match, Toggle pin/output (P1.29) para ver el instante de muestreo
  En cada Match interumpe, es decir con un periodo Ts= Fpclk/(MR1+1) y se habilita el modo Burst (Rafaga) para los canales 0, 2, 4
	Habilitamos la salida (MAT0.1) para observar el instante en que se lanza la ráfaga (ojo!! con cada flanco de esta señal)*/
						
void init_TIMER0(void)
{
   LPC_SC->PCONP|=(1<<1);                 // 
   LPC_PINCON->PINSEL3|= 0x0C000000;      // 
   LPC_TIM0->MCR = 0x18;                  //   Interrupt on Match 1 and Reset TC
   LPC_TIM0->MR1 = (F_pclk/F_muestreo)-1; //  Periodo de muestreo de TODAS las entradas!!!!   
   LPC_TIM0->EMR = 0x00C2;                // 
   LPC_TIM0->TCR = 0x01;                  //  
   NVIC_EnableIRQ(TIMER0_IRQn);           //  
   NVIC_SetPriority(ADC_IRQn,1);          // 
}	


int main(void)
{
   NVIC_SetPriorityGrouping(2);
   init_ADC();
   init_TIMER0();
   while(1);
}

