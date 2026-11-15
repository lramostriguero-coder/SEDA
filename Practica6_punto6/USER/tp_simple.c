#include <LPC17xx.H>
#include <glcd.h>
#include <TouchPanel.h>
#include <stdio.h>
#include <string.h>
#include "uart.h"
#include <math.h>

#define Fpclk 25e6	// Fcpu/4 (defecto después del reset)
#define Fpwm 2e3		// Frecuencia de la señal PWM (brillo LCD)

#define T_pwm 15e-3	// Periodo de la PWM generada por TIMER 0 (servos)
#define Tmin 0.5e-3	
#define Tmax 2.6e-3	
uint32_t cuentas_periodo = Fpclk * T_pwm;
uint32_t cuentas[2];

#define ZONAS 2			// Zona valida o no valida
#define PI 3.14

/* Estructura de zona inicial y zona final captadas por el LIDAR */
struct t_zona
{
	uint16_t inicial;
	uint16_t final;
};

/* Definicion de zona valida o invalida captadas por el LIDAR */
struct t_zona zonas_validas[ZONAS] = {{45, 270}, {360, 45}};


/* Estructura que define una zona de la pantalla */
struct t_screenZone
{
  uint16_t x;         
	uint16_t y;
	uint16_t size_x;
	uint16_t size_y;
	uint8_t  pressed;
};

/* Definicion de las diferentes zonas de la pantalla */
struct t_screenZone zone_1 = { 20,  14, 200,  50, 0}; /*  Visualizacion anguloLIDAR			 */
struct t_screenZone zone_2 = { 20, 116, 200,  50, 0}; /*  Visualizacion umbralDistancia  */
struct t_screenZone zone_3 = { 20, 218, 200, 50, 0}; 	/*  Visualizacion brilloDisplay		 */
struct t_screenZone zone_4 = { 55, 72,  30,  30, 0}; 	/* 				Botón  Aumentar    			 */
struct t_screenZone zone_5 = {155, 72,  30,  30, 0}; 	/* 				Botón  Disminuir   			 */
struct t_screenZone zone_6 = { 55, 174,  30,  30, 0}; /* 				Botón  Aumentar    			 */
struct t_screenZone zone_7 = {155, 174,  30,  30, 0}; /* 				Botón  Disminuir  			 */
struct t_screenZone zone_8 = { 55, 276,  30,  30, 0}; /* 				Botón  Aumentar    			 */
struct t_screenZone zone_9 = {155, 276,  30,  30, 0}; /* 				Botón  Disminuir   			 */


/* Estados para comandos por Terminal */
typedef enum 
{
	enviando_msj_inicial, 
	msj_inicial_enviado,
	enviando_mensaje_ordenes,
	enviado_mensaje_ordenes, 
	esperando_comando,
	notificando_medida,
	modificando_distancia,
	notificando_comando_erroneo,
	notificando_finalizacion, 
	fin
} t_estado;

/* Estados para recepcion tramas LIDAR */
typedef enum
{
	esperando_cab1,
	esperando_cab2,
	esperando_cab3,
	esperando_cab4,
	recibiendo_datos
} t_estado_LIDAR;


/* Flag que indica si se detecta una pulsación válida */
uint8_t pressedTouchPanel = 0;

/* Variables que contiene el dato del programa */
int angLIDAR = 0;						// (º)
float umbDISTANCIA = 30;		// (cm)
int brilloDISP = 20;				// (%)

/* Variables donde almacenar cadenas de caracteres */
char grado[25];
char cm[25];
char porcentaje[25];

/* Variables para comunicacion por Terminal (UART0) */
char buffer[100];			// Buffer de recepción de 100 caracteres
char buffer_tx[100];	// Buffer de recepción de 100 caracteres
char *ptr_rx;					// puntero de recepción
char rx_completa;			// Flag de recepción de cadena que se activa a "1" al recibir la tecla return CR(ASCII=13)
char *ptr_tx;					// puntero de transmisión
char tx_completa;			// Flag de transmisión de cadena que se activa al transmitir el caracter null (fin de cadena)

t_estado estado; 	// Variable estado para comandos por Terminal

/* Variables para Rx datos LIDAR (UART3) */
uint8_t dato_rx3;									// Buffer de recepcion de trama del LIDAR (1 Byte)
uint8_t trama_lidar[TAM_TRAMA];		// Buffer de recepcion de tramas del LIDAR (36 Bytes)
uint32_t tramas_ok = 0;						// Flag de recepcion de tramas correcta
uint32_t tramas_error = 0;				// Flag de error en trama recibida
uint8_t indice_trama = 0;					// Numero de byte de la trama
uint16_t lidar_scan_360[360];			// Array para almacenar medidas de distancia
	
t_estado_LIDAR estado_lidar = esperando_cab1; // Estado incial Rx LIDAR


/* Configuración TIMER0 para generar señal PWM por MAT0.0 y MAT0.1 */
void init_TIMER0(void)
{
	  LPC_SC->PCONP|=(1<<1);						// Power ON
    LPC_TIM0->MCR = (1<<0)|(1<<3);  	// Interrup on MATCH 1 y MATCH 0 (Running TC) 
    LPC_TIM0->MR0 = 20000; 						// Valor inicial cualquiera  
    LPC_TIM0->MR1 = 10000; 						// Valor inicial cualquiera  
		LPC_TIM0->EMR = 0xF0;   		 		  // Toggle on MATCH0.0 y MATCH0.1
    LPC_TIM0->TCR = 0x01;     				// Start 
}	
/* Generacion de señal PWM para servos */
void TIMER0_IRQHandler(void)
{
	if (LPC_TIM0->IR & (1<<0)) 		//MATCH 0?
		{ 
		LPC_TIM0->IR=(1<<0);				// Clear flag MR0	
		if (LPC_TIM0->EMR & (1<<0))
		{
			LPC_TIM0->MR0 += cuentas[0];
		} 
		else
		{
			LPC_TIM0->MR0 += cuentas_periodo - cuentas[0];
		}
	}
	if (LPC_TIM0->IR & (1<<1)) 		//MATCH 1?
	{
		LPC_TIM0->IR=(1<<1);				// Clear flag MR1
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

void delay_ms(int ms) {
   int i,j;
   for (j=0; j < ms; j++ )
      for (i=0; i < 17000; i++);
}

/* Paso 11: coloca el indicador (dos servos apilados) en una posición entre 0 y 360 grados */
void set_indicador_360(uint16_t grados)
{
	uint16_t g0, g1;                    // Grados de cada servo (g0 = servo0, g1 = servo1)
 
	if (grados <= 180){
		g0 = grados;					// El servo 0 hace los primeros 180º
		g1 = 0;
	}
	else {
		g0 = 180;						// El servo 0 se queda al final
		g1 = grados - 180;				// El servo 1 hace el resto
	}
 
	cuentas[0] = Fpclk*Tmin + Fpclk*(Tmax-Tmin)*g0/180;	// Servo 0
	cuentas[1] = Fpclk*Tmin + Fpclk*(Tmax-Tmin)*g1/180;	// Servo 1
}

/* Funcion comprobacion zona valida */
uint16_t zona_valida (int angulo){
	int z;
	
	for ( z= 0; z < ZONAS; z++)
	{
	  if((angulo >= zonas_validas[z].inicial)&&(angulo <= zonas_validas[z].final))
			return 1;	// Zona valida
	}
	
	return 0;			// Zona no valida
}

/* Extraer las 8 muestras de una trama con CRC correcto y guardarla en lidar_scan_360 */
void extraer_muestras(void)
{
	float ang_ini, ang_fin, inc_ang, ang_i;
	uint16_t distancia;
	int i, indice;
	
	// Angulo inicial (bytes 6-7) y final (bytes 32-33).
	// Primero byte bajo, luego alto.
	// Vienen multiplicadas por 64
	ang_ini = ((trama_lidar[7] << 8) | trama_lidar[6])/64.0f;
	ang_fin = ((trama_lidar[33] << 8) | trama_lidar[32])/64.0f;
 
	if (ang_fin < ang_ini)								
		ang_fin = ang_fin + 360;						// +360º en caso de angulo incremento negativo
	
	// Incremento de ángulo entre muestras
	inc_ang = (ang_fin - ang_ini)/8;
 
	// Ángulo y distancia de cada una de las 8 muestras
	for (i = 0; i < 8; i++)
	{
		ang_i = (ang_ini + i * inc_ang);			// angulo en la muestra
		indice = (int)(ang_i + 0.5f) % 360;		// round(ang_i) mod 360 : angulo de toma de medida
		
		// Distancia: byte 8+3*i (bajo) y 9+3*i (alto). Byte 10+3*i es la calidad
		distancia = (trama_lidar[9 + 3*i] << 8) | trama_lidar[8 + 3*i];
		
		if (distancia & 0x8000)		// 0x8000: el LiDAR no ha medido nada en ese ángulo
			distancia = 0;
		
		lidar_scan_360[indice] = distancia;		// distnacia en mm
	}
}
 
	
/* Maquina de estados: Rx LIDAR */
void maquina_estados_lidar(uint8_t dato)
{
	switch(estado_lidar){
	case esperando_cab1:
		if (dato == 0x55)
		{
			trama_lidar[0] = dato;
			estado_lidar = esperando_cab2;
		}
		break;

	case esperando_cab2:
		if (dato == 0xAA)
		{
			trama_lidar[1] = dato;
			estado_lidar = esperando_cab3;
		}
		else
			estado_lidar = esperando_cab1;	// Cabecera erronea, volver a empezar
		break;

	case esperando_cab3:
		if (dato == 0x03)
		{
			trama_lidar[2] = dato;
			estado_lidar = esperando_cab4;
		}
		else
			estado_lidar = esperando_cab1; 	// Cabecera erronea, volver a empezar
		break;

	case esperando_cab4:								
		if (dato == 0x08)
		{
			trama_lidar[3] = dato;
			indice_trama = 4;								// Los datos empiezan en la posición 4
			estado_lidar = recibiendo_datos;
		}
		else
			estado_lidar = esperando_cab1;	// Cabecera erronea, volver a empezar
		break;
	
	case recibiendo_datos:
		trama_lidar[indice_trama++] = dato;
		if (indice_trama == TAM_TRAMA)		// Trama completa (36 bytes)
		{	// CRC recibido: byte 34 (parte baja) y byte 35 (parte alta)
			if (Camsense_Checksum(trama_lidar) == (trama_lidar[34] | (trama_lidar[35] << 8))) // Comprobar CRC
			{
				tramas_ok++;
				extraer_muestras();						// Extraer las 8 muestras de la trama
			}
			else
				tramas_error++;
			
			estado_lidar = esperando_cab1;	// Siguiente trama
		}
		break;
		
	default:
		break;
	}
	
}	
	
/* Maquina de estados: comandos por Terminal */
void maquina_de_estados (void){
	
		 switch(estado){
		 case enviando_msj_inicial:
				if (tx_completa)
					estado = msj_inicial_enviado;
				break;
				
		 case msj_inicial_enviado:
				tx_cadena_UART0("Introduce un comando apaga, enciende, GetMedida, SetUmbralDistancia o fin.\n");
				estado = enviando_mensaje_ordenes;
				break;
		 
		 case enviando_mensaje_ordenes:
				if (tx_completa)
					estado = enviado_mensaje_ordenes;
				break;
				
		 case enviado_mensaje_ordenes:
				estado = esperando_comando;
				break;
		 
		 case esperando_comando:
				if(rx_completa)
				{
					rx_completa = 0;
					if (strcmp(buffer, "enciende\r") == 0)
						LPC_GPIO3->FIOPIN&=~(1<<26);					 	// Led P3.26 encendido
          else if (strcmp (buffer, "apaga\r") == 0)  
						LPC_GPIO3->FIOPIN|=(1<<26);							// Led P3.26 apagado
					
					else if ((strcmp(buffer, "GetMedida\r")) == 0)
					{
						// Guardar en buffer Tx el valor de las variables						
						sprintf(buffer_tx, "Angulo: %d grados, Umbral de Distancia: %.1f cm, Brillo: %d %%", angLIDAR, umbDISTANCIA, brilloDISP);
						tx_cadena_UART0(buffer_tx);																							// Transmitir buffer
						estado = notificando_medida;
					}
					else if (sscanf(buffer, "SetUmbralDistancia %f", &umbDISTANCIA) == 1)
					{		// Esperar a introducir valor Distancia y guardar en umbralDistancia
						sprintf(buffer_tx, "Umbral de distancia = %.1f cm\n\r", umbDISTANCIA);	// Guardar valor de umbralDistancia en buffer de Tx
						tx_cadena_UART0(buffer_tx);																							// Transmitir buffer
						estado = notificando_medida;
					}
          else if (strcmp (buffer, "fin\r") == 0)
					{		
						tx_cadena_UART0("Fin del programa.\r");
						estado = notificando_finalizacion;
					}
          else
					{
						tx_cadena_UART0("Comando erroneo\n\r");
						estado = notificando_comando_erroneo;
					} 
				}
				break;
			
		 case notificando_medida:
			 if (tx_completa)
				 estado = esperando_comando;
			 break;
			 
		 case notificando_comando_erroneo:
				if (tx_completa)
					estado= esperando_comando;
				break;
				
		 case notificando_finalizacion:
			 if (tx_completa)
				 estado= fin;
			 break;
			 
		 case fin:
			 
		 default:
			 break;
	 }
}

/* Funcion de configuracion PWM1 (brillo LCD) */
void configPWM(void) {
   LPC_PINCON->PINSEL3|=(2<<20); 	// P1.26 salida PWM (PWM1.6) 
   LPC_SC->PCONP|=(1<<6);
   LPC_PWM1->MR0= Fpclk/Fpwm-1; 
   LPC_PWM1->PCR|=(1<<14); 				// Configurado el ENA6 (P1.26)
   LPC_PWM1->MCR|=(1<<1);
   LPC_PWM1->TCR|=(1<<0)|(1<<3);
}

/*******************************************************************************
* Function Name  : squareButton
* Description    : Dibuja un cuadrado en las coordenadas especificadas colocando 
*                  un texto en el centro del recuadro
* Input          : zone: zone struct
*                  text: texto a representar en el cuadro
*                  textColor: color del texto
*                  lineColor: color de la línea
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/
void squareButton(struct t_screenZone* zone, char * text, uint16_t textColor, uint16_t lineColor)
{
   LCD_DrawLine( zone->x, zone->y, zone->x + zone->size_x, zone->y, lineColor);
   LCD_DrawLine( zone->x, zone->y, zone->x, zone->y + zone->size_y, lineColor);
   LCD_DrawLine( zone->x, zone->y + zone->size_y, zone->x + zone->size_x, zone->y + zone->size_y, lineColor);
   LCD_DrawLine( zone->x + zone->size_x, zone->y, zone->x + zone->size_x, zone->y + zone->size_y, lineColor);
	 GUI_Text(zone->x + zone->size_x/2 - (strlen(text)/2)*8, zone->y + zone->size_y/2 - 8,
            (uint8_t*) text, textColor, Black);	
}

/*******************************************************************************
* Function Name  : drawMinus
* Description    : Draw a minus sign in the center of the zone
* Input          : zone: zone struct
*                  lineColor
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/
void drawMinus(struct t_screenZone* zone, uint16_t lineColor)
{
   LCD_DrawLine( zone->x + 5 , zone->y + zone->size_y/2 - 1, 
                 zone->x + zone->size_x-5, zone->y + zone->size_y/2 - 1,
                 lineColor);
   LCD_DrawLine( zone->x + 5 , zone->y + zone->size_y/2, 
                 zone->x + zone->size_x-5, zone->y + zone->size_y/2,
                 lineColor);
   LCD_DrawLine( zone->x + 5 , zone->y + zone->size_y/2 + 1, 
                 zone->x + zone->size_x-5, zone->y + zone->size_y/2 + 1,
                 lineColor);
}

/*******************************************************************************
* Function Name  : drawMinus
* Description    : Draw a minus sign in the center of the zone
* Input          : zone: zone struct
*                  lineColor
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/
void drawAdd(struct t_screenZone* zone, uint16_t lineColor)
{
   drawMinus(zone, lineColor);
   
   LCD_DrawLine( zone->x + zone->size_x/2 - 1,  zone->y + 5 ,
                 zone->x + zone->size_x/2 - 1,  zone->y + zone->size_y - 5, 
                 lineColor);
   LCD_DrawLine( zone->x + zone->size_x/2 ,  zone->y + 5 ,
                 zone->x + zone->size_x/2 ,  zone->y + zone->size_y - 5, 
                 lineColor);
   LCD_DrawLine( zone->x + zone->size_x/2 + 1,  zone->y + 5 ,
                 zone->x + zone->size_x/2 + 1,  zone->y + zone->size_y - 5, 
                 lineColor);
}


/*******************************************************************************
* Function Name  : screenMain
* Description    : Visualiza la pantalla principal
* Input          : None
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/
void screenMain(void)
{	
	 squareButton(&zone_1, "Lidar:         grados   ", White, Blue);
	 squareButton(&zone_2, "Distancia:          cm  ", White, Blue);
	 squareButton(&zone_3, "Brillo:             %   ", White, Blue);
	 drawMinus(&zone_5, Yellow);
	 drawMinus(&zone_7, Yellow);
	 drawMinus(&zone_9, Yellow);
   drawAdd(&zone_4, Yellow);
   drawAdd(&zone_6, Yellow);
   drawAdd(&zone_8, Yellow);
}

/*******************************************************************************
* Function Name  : checkTouchPanel
* Description    : Lee el TouchPanel y almacena las coordenadas si detecta pulsación
* Input          : None
* Output         : Modifica pressedTouchPanel
*                    0 - si no se detecta pulsación
*                    1 - si se detecta pulsación
*                        En este caso se actualizan las coordinadas en la estructura display
* Return         : None
* Attention		  : None
*******************************************************************************/
void checkTouchPanel(void)
{
	Coordinate* coord;
	
	coord = Read_Ads7846();
	
	if (coord > 0) {
	  getDisplayPoint(&display, coord, &matrix );
     pressedTouchPanel = 1;
   }   
   else
   {   
     pressedTouchPanel = 0;
      
     // Esto es necesario hacerlo si hay dos zonas diferentes en 
     // dos pantallas secuenciales que se solapen      
     zone_1.pressed = 1;
     zone_2.pressed = 1;
     zone_3.pressed = 1;
     zone_4.pressed = 1;
     zone_5.pressed = 1;
		 zone_6.pressed = 1;
		 zone_7.pressed = 1;
		 zone_8.pressed = 1;
		 zone_9.pressed = 1;
     
   }  
}

/*******************************************************************************
* Function Name  : zonePressed
* Description    : Detecta si se ha producido una pulsación en una zona contreta
* Input          : zone: Estructura con la información de la zona
* Output         : Modifica zone->pressed
*                    0 - si no se detecta pulsación en la zona
*                    1 - si se detecta pulsación en la zona
* Return         : 0 - si no se detecta pulsación en la zona
*                  1 - si se detecta pulsación en la zona
* Attention		  : None
*******************************************************************************/
int8_t zonePressed(struct t_screenZone* zone)
{
	if (pressedTouchPanel == 1) {

		if ((display.x > zone->x) && (display.x < zone->x + zone->size_x) && 
			  (display.y > zone->y) && (display.y < zone->y + zone->size_y))
      {
         zone->pressed = 1;
		   return 1;
      }   
	}
   
	zone->pressed = 0;
	return 0;
}

/*******************************************************************************
* Function Name  : zoneNewPressed
* Description    : Detecta si se ha producido el flanco de una nueva pulsación en 
*                  una zona contreta
* Input          : zone: Estructura con la información de la zona
* Output         : Modifica zone->pressed
*                    0 - si no se detecta pulsación en la zona
*                    1 - si se detecta pulsación en la zona
* Return         : 0 - si no se detecta nueva pulsación en la zona
*                  1 - si se detecta una nueva pulsación en la zona
* Attention		  : None
*******************************************************************************/
int8_t zoneNewPressed(struct t_screenZone* zone)
{
	if (pressedTouchPanel == 1) {

		if ((display.x > zone->x) && (display.x < zone->x + zone->size_x) && 
			  (display.y > zone->y) && (display.y < zone->y + zone->size_y))
      {
         if (zone->pressed == 0)
         {   
            zone->pressed = 1;
            return 1;
         }
		   return 0;
      }
	}

   zone->pressed = 0;
	return 0;
}

/* Ajuste de brillo de display LCD (PWM1) */
void setBrillo(float brilloDISP) 
{
   LPC_PWM1->MR6= LPC_PWM1->MR0*brilloDISP/100; // P1.26
   LPC_PWM1->LER|=(1<<6)|(1<<0);
}



int main (void) 
{ 
	int i;											// variable de bucle para recorrer lidar_scan_360
	uint16_t color;
	uint8_t deteccion;					// '1' => objeto detectado,  '0'=> sin detección
	uint16_t distancia_minima;	// umbral de distancia de prueba
	int angulo_deteccion;				// usada solo si deteccion = 1
	char texto[40];							// Notifica por pantalla si hay deteccion.
	
	configPWM();
  LCD_Initializtion();
  LCD_Clear(Black);		
  TP_Init(); 
  //TouchPanel_Calibrate(); /* Configurar "matrix" y comentar para eliminar la calibración */
  
  // screenMain(); 					/* Comentar para ver salida LiDAR */
	
	/* UART0 y 3 (Comandos por Terminal y Rx datos LIDAR) */
  ptr_rx=buffer;	    		  // Inicializa el puntero de recepción al comienzo del buffer
  uart0_init(115200); 		 	// Configura la UART0 a 115200 baudios, 8 bits, 1 bit stop
	uart3_init(115200);				// Configura la UART3 a 115200 baudios, 8 bits, 1 bit stop
	
	LPC_PINCON->PINSEL7|=(2<<18); // P3.25 es MAT0.0	
	LPC_PINCON->PINSEL7|=(2<<20); // P3.26 es MAT0.1	
	
	cuentas[0]= Fpclk*Tmin;			// Posicion de reposo servo 0
	cuentas[1]= Fpclk*Tmin;			// Posicion de reposo servo 1
	
	set_indicador_360(0);
	
	init_TIMER0();
	NVIC_EnableIRQ(TIMER0_IRQn);

	
  tx_cadena_UART0("Esto es una prueba de la UART0 del LPC1768 a 115200 baudios\n\r");
  estado = enviando_msj_inicial;

	while(1){
		maquina_de_estados();
    //checkTouchPanel();
		setBrillo(brilloDISP);
		 
		LCD_Clear(Black);
		deteccion = 0;
		distancia_minima = 0xFFFF;
		for (i=0; i < 360; i++) 									// Recorrer datos de LiDAR
		{
			if(!zona_valida(i))
				color = Blue;				// Ignorar zona no valida
			else if (lidar_scan_360[i] != 0 && lidar_scan_360[i] < umbDISTANCIA*10)
			{
				color  = Red;				// Objeto cercano detectado
				deteccion = 1;
				if (lidar_scan_360[i] < distancia_minima)
				{
					distancia_minima = lidar_scan_360[i];		// Actualizar objeto más cercano detectado
					angulo_deteccion = i;										// Angulo de detección
				}
			}
			else 
				color = White;
			
			LCD_SetPoint(120+(int)((lidar_scan_360[i]/20.0f)*sin(i*PI/180)),			// Mostrar por display 
							     160+(int)((lidar_scan_360[i]/20.0f)*cos(i*PI/180)), 			// entorno captado por LiDAR
									 color); 
		}
		
		if(deteccion)
		{
			set_indicador_360(angulo_deteccion);			// Apuntar al objeto
			sprintf(texto, "Objeto: %3d grados %4d mm", angulo_deteccion, distancia_minima);
			GUI_Text(0, 300, (uint8_t *)texto, Red, Black);
		}
		else
		{
			set_indicador_360(0);											// En reposo
			GUI_Text(0, 300, (uint8_t *)"Sin deteccion.", White, Black);
		}
			 
	}   
}

