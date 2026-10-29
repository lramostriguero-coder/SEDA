#include <LPC17xx.H>
#include <glcd.h>
#include <TouchPanel.h>
#include <stdio.h>
#include <string.h>
#include "uart.h"

#define Fpclk 25e6	// Fcpu/4 (defecto después del reset)
#define Fpwm 2e3		// Frecuencia de la señal PWM (2kHz)

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


/* Flag que indica si se detecta una pulsación válida */
uint8_t pressedTouchPanel = 0;

/* Variables que contiene el dato del programa */
int angLIDAR = 0;
float umbDISTANCIA = 0;
int brilloDISP = 20;

/* Variables donde almacenar cadenas de caracteres */
char grado[25];
char cm[25];
char porcentaje[25];

/* Variables para UART */
char buffer[100];			// Buffer de recepción de 100 caracteres
char buffer_tx[100];	// Buffer de recepción de 100 caracteres
char *ptr_rx;					// puntero de recepción
char rx_completa;			// Flag de recepción de cadena que se activa a "1" al recibir la tecla return CR(ASCII=13)
char *ptr_tx;					// puntero de transmisión
char tx_completa;			// Flag de transmisión de cadena que se activa al transmitir el caracter null (fin de cadena)

/* Estados */
typedef enum {
	enviando_msj_inicial, 
	msj_inicial_enviado,
	enviando_mensaje_ordenes,
	enviado_mensaje_ordenes, 
	esperando_comando,
	notificando_medida,
	modificando_distancia,
	notificando_comando_erroneo,
	notificando_finalizacion, 
	fin} t_estado;

t_estado estado;	// Variable estado

/* Variables para Rx datos LIDAR */
uint8_t dato_rx3;
uint8_t trama_lidar[TAM_TRAMA];
uint32_t tramas_ok = 0;
uint32_t tramas_error = 0;
uint8_t indice_trama = 0;

/* Estados para recepcion tramas LIDAR */
typedef enum{
	esperando_cab1,
	esperando_cab2,
	esperando_cab3,
	esperando_cab4,
	recibiendo_datos
} t_estado_LIDAR;

t_estado_LIDAR estado_lidar = esperando_cab1; // Estado incial lidar

// Maquina de estados Rx LIDAR
void maquina_estados_lidar(uint8_t dato)
{
	switch(estado_lidar){
	case esperando_cab1:
		if (dato == 0x55){
			trama_lidar[0] = dato;
			estado_lidar = esperando_cab2;
		}
		break;

	case esperando_cab2:
		if (dato == 0xAA){
			trama_lidar[1] = dato;
			estado_lidar = esperando_cab3;
		}
		else
			estado_lidar = esperando_cab1;	// Cabecera erronea, volver a empezar
		break;

	case esperando_cab3:
		if (dato == 0x03){
			trama_lidar[2] = dato;
			estado_lidar = esperando_cab4;
		}
		else
			estado_lidar = esperando_cab1; 	// Cabecera erronea, volver a empezar
		break;

	case esperando_cab4:								
		if (dato == 0x08){
			trama_lidar[3] = dato;
			indice_trama = 4;								// Los datos empiezan en la posición 4
			estado_lidar = recibiendo_datos;
		}
		else
			estado_lidar = esperando_cab1;	// Cabecera erronea, volver a empezar
		break;

	case recibiendo_datos:
		trama_lidar[indice_trama++] = dato;
		if (indice_trama == TAM_TRAMA){		// Trama completa (36 bytes)
			// CRC recibido: byte 34 (parte baja) y byte 35 (parte alta)
			if (Camsense_Checksum(trama_lidar) == (trama_lidar[34] | (trama_lidar[35] << 8))){
				tramas_ok++;
				// Paso 6: aquí se extraerán las 8 muestras de la trama
			}
			else
				tramas_error++;
			estado_lidar = esperando_cab1;		// Siguiente trama
		}
		break;
	}
}	
	
// Funcion maquina de estados Terminal
void maquina_de_estados (void){
	
		 switch(estado){
		 case enviando_msj_inicial:
				if (tx_completa)
					estado = msj_inicial_enviado;
				break;
				
		 case msj_inicial_enviado:
				tx_cadena_UART0("Introduce un comando apaga, enciende, GetMedida o SetUmbralDistancia, fin.\n");
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
				if(rx_completa){
					rx_completa = 0;
					if (strcmp(buffer, "enciende\r") == 0)
						LPC_GPIO3->FIOPIN&=~(1<<26);					 	// Led P3.26 encendido
          else if (strcmp (buffer, "apaga\r") == 0)  
						LPC_GPIO3->FIOPIN|=(1<<26);							// Led P3.26 apagado
					
					else if ((strcmp(buffer, "GetMedida\r")) == 0){
						// Guardar en buffer Tx el valor de las variables						
						sprintf(buffer_tx, "Angulo: %d grados, Umbral de Distancia: %.1f cm, Brillo: %d %%", angLIDAR, umbDISTANCIA, brilloDISP);
						tx_cadena_UART0(buffer_tx);																							// Transmitir buffer
						estado = notificando_medida;
					}
					else if (sscanf(buffer, "SetUmbralDistancia %f", &umbDISTANCIA) == 1){		// Esperar a introducir valor Distancia y guardar en umbralDistancia
						sprintf(buffer_tx, "Umbral de distancia = %.1f cm\n\r", umbDISTANCIA);	// Guardar valor de umbralDistancia en buffer de Tx
						tx_cadena_UART0(buffer_tx);																							// Transmitir buffer
						estado = notificando_medida;
					}
          else if (strcmp (buffer, "fin\r") == 0){		
						tx_cadena_UART0("Fin del programa.\r");
						estado = notificando_finalizacion;
					}
          else {
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

void setBrillo(float brilloDISP) {
   LPC_PWM1->MR6= LPC_PWM1->MR0*brilloDISP/100; // P1.26
   LPC_PWM1->LER|=(1<<6)|(1<<0);
}

int main (void) { 
	configPWM();
  LCD_Initializtion();
  LCD_Clear(Black);	
  TP_Init(); 
  //TouchPanel_Calibrate(); /* Configurar "matrix" y comentar para eliminar la calibración */
  
  screenMain();
	
	// UART0,3 (Comandos por Terminal y Rx datos LIDAR)
	LPC_GPIO3->FIODIR |= (1<<26);	// P3.26 definido como salida  
  LPC_GPIO3->FIOCLR |= (1<<26);	// P3.26 apagado 

  ptr_rx=buffer;	    		  // Inicializa el puntero de recepción al comienzo del buffer
  uart0_init(115200); 		 	// Configura la UART0 a 115200 baudios, 8 bits, 1 bit stop
	uart3_init(115200);				// Configura la UART3 a 115200 baudios, 8 bits, 1 bit stop
	
  tx_cadena_UART0("Esto es una prueba de la UART0 del LPC1768 a 115200 baudios\n\r");
  estado = enviando_msj_inicial;
	// Fin inicialicacion UART0,3
	
	while(1){
		 maquina_de_estados();
     checkTouchPanel();
		 setBrillo(brilloDISP);
		 if (zonePressed(&zone_1)){
				angLIDAR = 0;}
     if (zonePressed(&zone_2)){
        umbDISTANCIA = 0;}
     if (zonePressed(&zone_3)){
        brilloDISP = 20;}
     if (zoneNewPressed(&zone_4)){
			  (angLIDAR == 360) ? angLIDAR = 360 : angLIDAR ++; 
		 }
     if (zoneNewPressed(&zone_5)){
			  (angLIDAR == 0) ? angLIDAR = 0 : angLIDAR --; 
     }
		 if (zoneNewPressed(&zone_6)){
			  umbDISTANCIA = umbDISTANCIA + 0.1;
		 }
		 if (zoneNewPressed(&zone_7)){
			 if (umbDISTANCIA <= 0.1) {
					umbDISTANCIA = 0;}
			 else {
					umbDISTANCIA = umbDISTANCIA - 0.1;}
		 }
		 if (zonePressed(&zone_8)){
			 (brilloDISP < 100) ? brilloDISP = brilloDISP + 1 : brilloDISP;
		 }
		 if (zonePressed(&zone_9)){
			 if(brilloDISP == 20) { 						// Brillo incial 20% para observar mejor el cambio del brillo
					brilloDISP = 20;} 
			 else { 
					brilloDISP = brilloDISP - 1;}
		 }
		 
		 sprintf(grado, "%4d", angLIDAR);
		 sprintf(cm,"%5.1f", umbDISTANCIA);
		 sprintf(porcentaje, "%3d", brilloDISP);
		 
     GUI_Text(zone_1.x + zone_1.size_x/2 - (strlen(grado)/2)*8, zone_1.y + zone_1.size_y/2 - 8,
             (uint8_t*) grado, White, Black);	
		 GUI_Text(zone_2.x + zone_2.size_x/2 - (strlen(cm)/2)*8, zone_2.y + zone_2.size_y/2 - 8,
             (uint8_t*) cm, White, Black);	
		 GUI_Text(zone_3.x + zone_3.size_x/2 - (strlen(porcentaje)/2)*8, zone_3.y + zone_3.size_y/2 - 8,
             (uint8_t*) porcentaje, White, Black);	
  } 	 
     
}

