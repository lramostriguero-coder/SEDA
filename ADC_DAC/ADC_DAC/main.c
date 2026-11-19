#include <LPC17xx.H>
#include <Math.h>
#define F_cpu 100e6		// Defecto Keil (xtal=12Mhz)
#define F_pclk F_cpu/4 // Defecto despues del reset
#define F_muestreo 100 // Fs=100Hz (Cada 10ms se toma una muestra del canal 0)
#define pi 3.14159
#define F_out 1000
#define N_muestras 32
#define V_refp 3.3

uint16_t muestras[N_muestras];			// Array para guardar las muestras de un ciclo de un seno
float voltios;

void genera_muestras(uint8_t muestras_ciclo)
{
	uint8_t i;
	for(i=0;i<muestras_ciclo;i++)
		muestras[i]=1023*(0.5 + 0.5*sin(2*pi*i/muestras_ciclo)); // Ojo! el DAC es de 10bits
}

void ADC_IRQHandler(void)
{
	voltios= ((LPC_ADC->ADDR0>>4)&0xFFF)*3.3/4095;	// se borra automat. el flag DONE al leer ADCGDR
 //voltios= ((LPC_ADC->ADGDR>>4)&0xFFF)*3.3/4095;
}

// Timer 1 interrumpe periódicamente a F = F_out x N_muestras !!!!
// La muestra correpondiente del array,  se saca al DAC en cada interrupción
void TIMER1_IRQHandler(void)
{
	static uint8_t indice_muestra;
	LPC_TIM1->IR|= (1<<0); 						// ?
	LPC_DAC->DACR= muestras[indice_muestra++] << 6; // ? 
	indice_muestra&= 0x1F;						// ? 
}	

void init_DAC(void)
{
	LPC_PINCON->PINSEL1|= (2<<20); 	 	// DAC output = P0.26 (AOUT)
	LPC_PINCON->PINMODE1|= (2<<20); 	// Deshabilita pullup/pulldown
	LPC_SC->PCLKSEL0|= (0x00<<22); 	 	// CCLK/4 (Fpclk después del reset) (100 Mhz/4 = 25Mhz)	
	LPC_DAC->DACCTRL=0;								// ? 
}

void init_ADC(void)
{	
	LPC_SC->PCONP|= (1<<12);					// POwer ON
	LPC_PINCON->PINSEL1|= (1<<14);  	// ADC input= P0.23 (AD0.0)
	LPC_PINCON->PINMODE1|= (2<<14); 	// Deshabilita pullup/pulldown
	LPC_SC->PCLKSEL0|= (0x00<<8); 		// CCLK/4 (Fpclk después del reset) (100 Mhz/4 = 25Mhz)
	LPC_ADC->ADCR= (0x01<<0)|		  	  // Canal 0
								 (0x01<<8)|		  	  // CLKDIV=1   (Fclk_ADC=25Mhz /(1+1)= 12.5Mhz)
								 (0x01<<21)|			 	// PDN=1
								 (4<<24);				    // Inicio de conversión con el Match 1 del Timer 0
	
	LPC_ADC->ADINTEN=(1<<0);					// Hab. interrupción fin de conversión Canal 0 (Ojo no se ha utilizado |=, porque despues del reset queda habilitada ADGDR)
//LPC_ADC->ADINTEN=(1<<8);					// idem Reg. Global
	NVIC_EnableIRQ(ADC_IRQn);					// ? 
	NVIC_SetPriority(ADC_IRQn,1);			// ?        
}	

/* Timer 0 en modo Output Compare (reset T0TC on Match 1)
	 Counter clk: 25 MHz 	MAT0.1 : On match, Toggle pin/output (P1.29)
   Cada 2 Match se provoca el INICIO DE CONVERSIÓN DEL ADC 
	 Habilitamos la salida (MAT0.1) para observar la frecuencia de muestreo del ADC */
						
void init_TIMER0(void)
{
	  LPC_SC->PCONP|=(1<<1);						// ? 
	  LPC_PINCON->PINSEL3|= 0x0C000000; // ?
    LPC_TIM0->PR = 0x00;							// ?   
    LPC_TIM0->MCR = 0x10;   					// ?   
    LPC_TIM0->MR1 = (F_pclk/F_muestreo/2)-1; // Se han de producir DOS Match para iniciar la conversión!!!!   
    LPC_TIM0->EMR = 0x00C2;   				// ? 
    LPC_TIM0->TCR = 0x01;     				// ? 
}	


/*  Timer 1 en modo Output Compare (reset T0TC on Match 0)
	Counter clk: 25 MHz 	MAT1.0 : On match, salida de una muestra hacia el DAC */
void init_TIMER1(void)
{
	  LPC_SC->PCONP|=(1<<2);						// ? 	
    LPC_TIM1->PR = 0x00;     	 				// ? 
    LPC_TIM1->MCR = 0x03;							// Reset TC on Match, and Interrupt!  
    LPC_TIM1->MR0 = (F_pclk/F_out/N_muestras)-1;  // ? 
    LPC_TIM1->EMR = 0x02;   					// ? 
    LPC_TIM1->TCR = 0x01;							// ? 
    NVIC_EnableIRQ(TIMER1_IRQn);			// ? 
	  NVIC_SetPriority(TIMER1_IRQn,0);  // ?     
}

int main(void)
{
	NVIC_SetPriorityGrouping(2);
	genera_muestras(N_muestras);
	init_ADC();
	init_DAC();
	init_TIMER0();
	init_TIMER1();
	while(1);
}

