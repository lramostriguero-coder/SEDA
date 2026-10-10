//Salida PWM P1.29 (MAT0.1)

#include <LPC17xx.H>
#define F_cpu 100e6					// Defecto Keil (xtal=12Mhz)
#define F_pclk F_cpu/4 			// Defecto despues del reset

#define T_pwm 15e-3					

float ciclo_trabajo=0.5;
uint16_t grados = 0;

//Inicializa las interrupciones de Key1 u Key2 (/EINT1 y /EINT2)
void config_interrup_buttons()
{
	// Configuración interrupciones externas

	LPC_PINCON->PINSEL4|=(0x01<<22);	// P2.11 es entrada interrup. EXT 1
	LPC_PINCON->PINSEL4|=(0x01<<24);	// P2.12 es entrada interrup. EXT 2
	LPC_SC->EXTMODE|=(1<<2)|(1<<1);  	// Por Flanco,	
	LPC_SC->EXTPOLAR=0;				  			// de bajada

	// Asignación de prioridades
	NVIC_SetPriorityGrouping(2);		// 32 niveles de prioridad preemptive (Sin sub-prioridad)  
																	// NOTA: Los tres bits de menor peso no tienen efecto 
																	// en el LPC1768 (ojo! tiene 32 niveles) 

	NVIC_SetPriority(EINT1_IRQn, 3);	// Nivel 3
	NVIC_SetPriority(EINT2_IRQn, 2);	// NIvel 2 

	// Habilitación de las interrupciones
	NVIC_EnableIRQ(EINT1_IRQn); 		
	NVIC_EnableIRQ(EINT2_IRQn);	
}

void init_TIMER0(void)
{
	  LPC_SC->PCONP|=(1<<1);						// Power ON
    LPC_TIM0->MCR = (1<<3);  					// Interrup on MATCH 1 (Running TC) 
    LPC_TIM0->MR1 = 10000; 						// Valor inicial cualquiera  
    LPC_TIM0->EMR = 0xC0;   		 		  // Toggle on MATCH
    LPC_TIM0->TCR = 0x01;     				// Start 
}	


void TIMER0_IRQHandler(void)
{
	if(LPC_TIM0->EMR&(1<<1)){
					LPC_TIM0->MR1+= (uint32_t)(F_pclk*T_pwm*ciclo_trabajo); // EM1=1 ? (pin P1.29?)
				  }
	 else {
				  LPC_TIM0->MR1+= (uint32_t)(F_pclk*T_pwm*(1-ciclo_trabajo));
				}

	LPC_TIM0->IR|=(1<<1);	// Clear flag MR1
}

void EINT1_IRQHandler()
{
	LPC_SC->EXTINT=(1<<1); // Borrar flag  Externa 1
	ciclo_trabajo+=0.02;
	if(ciclo_trabajo>=0.98)ciclo_trabajo=0.98;
}

void EINT2_IRQHandler()
{
	LPC_SC->EXTINT=(1<<2); // Borrar flag Externa 2
	ciclo_trabajo-=0.02;
	if(ciclo_trabajo<=0.02)ciclo_trabajo=0.02;	
}

//void set_servo1_180 (uint8_t grados){
//	
//}
//void set_servo2_180 (uint8_t grados){}

int main(void)
{
	LPC_PINCON->PINSEL7|=(2<<18); // P3.25 es MAT0.0	
	LPC_PINCON->PINSEL7|=(2<<20); // P3.26 es MAT0.1	
	config_interrup_buttons();	
	NVIC_SetPriorityGrouping(2);
	init_TIMER0();
	NVIC_EnableIRQ(TIMER0_IRQn);
	while(1);
}
