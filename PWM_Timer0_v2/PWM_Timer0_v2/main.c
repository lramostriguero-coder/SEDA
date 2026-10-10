//Salida PWM P1.29 (MAT0.1)

#include <LPC17xx.H>
#define F_cpu 100e6					// Defecto Keil (xtal=12Mhz)
#define F_pclk F_cpu/4 			// Defecto despues del reset

#define T_pwm 15e-3

#define Tmin 0.5e-3	
#define Tmax 2.6e-3	

uint32_t cuentas_periodo = (uint32_t)(F_pclk * T_pwm);
uint32_t grados = 0;
uint32_t cuentas[2];


void init_TIMER0(void)
{
	  LPC_SC->PCONP|=(1<<1);						// Power ON
    LPC_TIM0->MCR = (1<<0)|(1<<3);  	// Interrup on MATCH 1 y MATCH 0 (Running TC) 
    LPC_TIM0->MR0 = 20000; 						// Valor inicial cualquiera  
    LPC_TIM0->MR1 = 10000; 						// Valor inicial cualquiera  
		LPC_TIM0->EMR = 0xF0;   		 		  // Toggle on MATCH0.0 y MATCH0.1
    LPC_TIM0->TCR = 0x01;     				// Start 
}	


void TIMER0_IRQHandler(void)
{
	if (LPC_TIM0->IR & (1<<0)) //MATCH 0?
		{ 
		LPC_TIM0->IR=(1<<0);	// Clear flag MR0	
		if (LPC_TIM0->EMR & (1<<0))
		{
			LPC_TIM0->MR0 += cuentas[0];
		} 
		else
		{
			LPC_TIM0->MR0 += cuentas_periodo - cuentas[0];
		}
	}
	if (LPC_TIM0->IR & (1<<1)) //MATCH 1?
	{
		LPC_TIM0->IR=(1<<1);	// Clear flag MR1
		if (LPC_TIM0->EMR & (1<<1))
		{
			LPC_TIM0->MR1 += cuentas[1];
		} 
		else
		{
			LPC_TIM0->MR1 += cuentas_periodo - cuentas[1];
		}
	}
}


void set_indicador_360 (uint16_t grados){
	
	if (grados == 0)||(grados > 360) {return;}
	
	uint16_t ang0, g0, g1;
	for(ang0 = 0, ang0 <= 360, ang0 += grados)
	{
		if (ang0 <= 180)
		{
			g0 = ang0;
			g1 = 0;
		}
		if (ang0 > 180)
		{
			g0 = 0;
			g1 = ang0;
		}
		cuentas[0] = F_pclk*Tmin + F_pclk*(Tmax-Tmin)*g0/180;
	  cuentas[1] = F_pclk*Tmin + F_pclk*(Tmax-Tmin)*g1/180;
		delay(200);
	}
	delay(1000);
	
	cuentas[0] = F_pclk*Tmin;           // Vuelta a reposo (0 grados)
	cuentas[1] = F_pclk*Tmin;
	delay_ms(1000);                     // Tiempo para llegar a reposo
}

void delay_ms(int ms) {
   int i,j;
   for (j=0; j < ms; j++ )
      for (i=0; i < 17000; i++);
}


int main(void)
{
	LPC_PINCON->PINSEL7|=(2<<18); // P3.25 es MAT0.0	
	LPC_PINCON->PINSEL7|=(2<<20); // P3.26 es MAT0.1	
	cuentas[0]= F_pclk*Tmin;
	cuentas[1]= F_pclk*Tmin;
	init_TIMER0();
	NVIC_EnableIRQ(TIMER0_IRQn);
	
	while(1)
	{
		set_indicador_360(20);
	}
}
