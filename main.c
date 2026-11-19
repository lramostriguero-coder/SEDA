#include <LPC17xx.H>
#include <Math.h>
// #include "audio_muestras.h"
#define F_cpu 100e6			// Defecto Keil (xtal=12Mhz)
#define F_pclk F_cpu/4 	// Defecto despues del reset
#define F_sample 8000
#define N_muestras 16000 // igual a la longitud del array generado


typedef  enum audio 
{
	idle,
	grabando,
	reproduciendo
} t_audio;

t_audio audio = idle;

volatile uint8_t muestras_voz[N_muestras];	// Variable para almacenar audio
volatile uint8_t canal_5;
uint8_t fin_grabacion = 0;		// Semaforo

// Timer 1 interrumpe periódicamente a F_sample = 8000 Hz !!!!
// La muestra correpondiente del array,  se saca al DAC en cada interrupción
void TIMER1_IRQHandler(void)
{
	static uint16_t indice_muestra;
	LPC_TIM1->IR|= (1<<0); 								// Borrar flag
	
	switch(audio){
		case idle:
			break;
		case grabando:
			LPC_ADC->ADCR |= (1 << 24);	// START = 001
			break;
		case reproduciendo:
			LPC_DAC->DACR= muestras_voz[indice_muestra++]<<8; // 8 bits!!!!
			if(indice_muestra==N_muestras-1)
			{
				audio = idle;					// Reposo
				indice_muestra = 0;
				LPC_TIM1->TCR = 0x02;	//Stop Timer and reset, DAC= 0V.
				LPC_DAC->DACR = 0;	  // 0 V
			}
			break;
		default:
			break;
	}
}	

void init_ADC (void)
{
	LPC_SC->PCONP |= (1 << 12);							// Power ON ADC
	LPC_PINCON->PINSEL3 |= (3 << 30);			// P1.31 => AD0.5
	LPC_PINCON->PINMODE3 = (2 << 30);			// P1.31 neither pull up/down
	LPC_SC->PCLKSEL0 |= (0x00 << 8);
	
	LPC_ADC->ADCR = (1 << 5)|			// Selecciona AD0.5
									 (1 << 8)|			// CLKDIV = 1 -> Fclk_adc = 12.5MHz
									 (1 << 21);			// PDN= 1
	LPC_ADC->ADINTEN = (1 << 5);		// Interrumpe canal 5
	
	NVIC_EnableIRQ(ADC_IRQn);
	NVIC_SetPriority(ADC_IRQn, 1);
	
}

void init_DAC(void)
{
	LPC_PINCON->PINSEL1|= (2<<20); 	 	// DAC output = P0.26 (AOUT)
	LPC_PINCON->PINMODE1|= (2<<20); 	// Deshabilita pullup/pulldown
	LPC_DAC->DACCTRL=0;								//  
}


/*  Timer 1 en modo Output Compare (reset T0TC on Match 0)
	  Counter clk: 25 MHz 	MAT1.0 : On match, salida de una muestra hacia el DAC */
void init_TIMER1(void)
{
	  LPC_SC->PCONP|=(1<<2);								// 	
    LPC_TIM1->PR = 0x00;     	 						// 
    LPC_TIM1->MCR = 0x03;									// Reset TC on Match, and Interrupt!  
    LPC_TIM1->MR0 = (F_pclk/F_sample)-1;  // El timer 1 interrumpe cada 125 us 
    LPC_TIM1->TCR = 0x02;									// Timer STOP y RESET
	
    NVIC_EnableIRQ(TIMER1_IRQn);					//
	  NVIC_SetPriority(TIMER1_IRQn,0);  		// Prioridad Máxima     
}


void init_Externas()
{
// Configuración interrupciones externas
LPC_PINCON->PINSEL4 |= (0x01<<22); 		// P2.11 es entrada interrup. EXT 1 (pulsador KEY1 en MIni-DK2)
LPC_PINCON->PINSEL4 |= (0x01<<24); 		// P2.11 es entrada interrup. EXT 2 (pulsador KEY2 en MIni-DK2)
LPC_SC->EXTMODE |= (1<<1)|(1 << 2);  						// Por Flanco,	
LPC_SC->EXTPOLAR &= ~((1 << 1)|(1 << 2));				// de bajada
	
NVIC_SetPriority(EINT1_IRQn, 1);		// Menor prioritaria!!! ;  sin CMSIS: NVIC->IP[22]=(4<<3); 
NVIC_SetPriority(EINT2_IRQn, 1);		// Menor prioritaria!!! ;  sin CMSIS: NVIC->IP[24]=(4<<3); 
NVIC_EnableIRQ(EINT1_IRQn); 				// sin CMSIS: NVIC->ISER[0]=(1<<28);
NVIC_EnableIRQ(EINT2_IRQn); 				// sin CMSIS: NVIC->ISER[0]=(1<<22);
}

void ADC_IRQHandler (void)
{	static uint16_t indice_grab;
	LPC_ADC-> ADCR &= ~(7 << 24);		// START = 000, Stop conversion
	muestras_voz[indice_grab++] = ((LPC_ADC->ADDR5 >> 8)&0xFF);		// 8 bits de mayor peso
	
	if (indice_grab == N_muestras)
	{
		LPC_TIM1->TCR = 0x02; // Stop y reset timer
		audio = idle;
		indice_grab = 0;
		fin_grabacion = 1;		// Semaforo
	}
}

/* Al pulsar KEY1 en Mini-DK2 se reproduce el audio */
void EINT1_IRQHandler()
{
	LPC_SC->EXTINT=(1<<1); // Borrar flag Externa 1
	audio = reproduciendo;
	LPC_TIM1->TCR=0x01;		 // Start Timer .
}

/* Al pulsar KEY2 en Mini-DK2 se muestrea el audio */
void EINT2_IRQHandler()
{
	LPC_SC->EXTINT=(1<<2); // Borrar flag Externa 1
	audio = grabando;			 
	LPC_TIM1->TCR=0x01;		 // Start Timer .
}
int main(void)
{
	NVIC_SetPriorityGrouping(2);
	init_Externas();
	init_DAC();
	init_ADC();
	init_TIMER1();
	LPC_TIM1->TCR=0x01;		//Start Timer .
	while(1);
}

