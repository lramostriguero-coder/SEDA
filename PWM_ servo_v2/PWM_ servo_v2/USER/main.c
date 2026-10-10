#include <LPC17xx.H>
#define Fpclk 25e6	// Fcpu/4 (defecto después del reset)
#define Tpwm 15e-3	// Perido de la señal PWM (15ms)

// añadido TH = Tmin + (Tmax-Tmin/180)*grados
#define Tmin 0.5e-3	
#define Tmax 2.6e-3	

float grados;


/*******************************************************************************
* Function Name  : configPWM
* Description    : Configura una señal PWM de 15ms de periodo por P1.20
* Input          : None
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/
void configPWM(void) {
   LPC_PINCON->PINSEL3|=(2<<8); // P1.20 salida PWM (PWM1.2)
   LPC_SC->PCONP|=(1<<6);
   LPC_PWM1->MR0=Fpclk*Tpwm-1;
   LPC_PWM1->PCR|=(1<<10); //configurado el ENA2 (1.2)
   LPC_PWM1->MCR|=(1<<1);
   LPC_PWM1->TCR|=(1<<0)|(1<<3);
}

/*******************************************************************************
* Function Name  : setServo
* Description    : Actusliza el valor de la señal PWM entre 1 y 2ms
* Input          : grados - Debe tomar valores de 0 a 180
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/
void setServo(float grados) {
   LPC_PWM1->MR2=(Fpclk*Tmin + Fpclk*(Tmax-Tmin)*grados/180); // TH
   LPC_PWM1->LER|=(1<<2)|(1<<0);
}

/*******************************************************************************
* Function Name  : delay_ms
* Description    : Función de retardo de milisegundos (aprox)
* Input          : ms - Número de milisegundos a retardar
* Output         : None
* Return         : None
* Attention		  : Esta función se considera una espera activa y no debe estar 
*                  en el proyecto final
*******************************************************************************/
void delay_ms(int ms) {
   int i,j;
   for (j=0; j < ms; j++ )
      for (i=0; i < 17000; i++);
}

/*******************************************************************************
* Function Name  : main
* Description    : Programa principal
* Input          : None
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/
int main(void) {
   uint8_t i;
   configPWM();
   
	 while(1){
		for(i=0;i<18;i++)	 {
				delay_ms(1); // variar el límite en simulación para ver más rápido la variación de la señal PWM									
				setServo(grados);	 // incrementamos la posición del servo de 10 en 10º
				grados+=10;
		}
		for(i=0;i<18;i++)	 {
				delay_ms(1); 								
				setServo(grados);	 
				grados-=10;
		}
	 }
}
