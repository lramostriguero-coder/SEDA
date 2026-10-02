#include <LPC17xx.H>
#include <glcd.h>
#include <TouchPanel.h>
#include <stdio.h>
#include <string.h>

#define Fpclk 25e6	// Fcpu/4 (defecto después del reset)
#define Fpwm 2e3	// Frecuencia de la señal PWM (2kHz)

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
struct t_screenZone zone_1 = { 20,  20, 200,  50, 0}; /*  Visualizacion anguloLIDAR */
struct t_screenZone zone_2 = { 20, 120, 200,  50, 0}; /*  Visualizacion umbralDistancia    */
struct t_screenZone zone_3 = { 20, 220, 200, 50, 0}; 	/* Visualizacion brilloDisplay		*/
struct t_screenZone zone_4 = { 40, 85,  30,  30, 0}; 													 							 /* Botón  Aumentar    */
struct t_screenZone zone_5 = {140, 85,  30,  30, 0}; 																				 /* Botón  Disminuir   */
struct t_screenZone zone_6 = { 40, 175,  30,  30, 0}; 																				 /* Botón  Aumentar    */
struct t_screenZone zone_7 = {140, 175,  30,  30, 0}; 	                                       /* Botón  Disminuir   */
struct t_screenZone zone_8 = { 40, 285,  30,  30, 0}; 																			 /* Botón  Aumentar    */
struct t_screenZone zone_9 = {140, 285,  30,  30, 0}; 	                                     /* Botón  Disminuir   */
//struct t_screenZone zone_10 = {}; 	


/* Flag que indica si se detecta una pulsación válida */
uint8_t pressedTouchPanel = 0;

/* Variables que contiene el dato del programa */
int angLIDAR = 0;
float umbDISTANCIA = 0;
int brilloDISP = 30;

/* Variables donde almacenar cadenas de caracteres */
char grado[25];
char cm[25];
char porcentaje[25];

void configPWM(void) {
   LPC_PINCON->PINSEL3|=(2<<20); // P1.26 salida PWM (PWM1.6) 
   LPC_SC->PCONP|=(1<<6);
   LPC_PWM1->MR0= Fpclk/Fpwm-1; 
   LPC_PWM1->PCR|=(1<<14); //configurado el ENA6 (P1.26)
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
   //squareButton(&zone_1, "Pulsa para subir o bajar", White, Blue);
   //squareButton(&zone_2, "                        ", White, Blue);
	// ini añadido
	 squareButton(&zone_1, "Lidar:         grados   ", White, Blue);
	 squareButton(&zone_2, "Distancia:          cm  ", White, Blue);
	 squareButton(&zone_3, "Brillo:             %   ", White, Blue);
	 drawMinus(&zone_5, Yellow);
	 drawMinus(&zone_7, Yellow);
	 drawMinus(&zone_9, Yellow);
   drawAdd(&zone_4, Yellow);
   drawAdd(&zone_6, Yellow);
   drawAdd(&zone_8, Yellow);
	 
	// fin añadido
   /*drawMinus(&zone_3, White);
   drawAdd(&zone_4, White);
   drawMinus(&zone_5, Yellow);
   drawAdd(&zone_6, Yellow);*/
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
  // ini añadido 
  screenMain();
	while(1){
     checkTouchPanel();
		setBrillo(brilloDISP);
		 if (zonePressed(&zone_1)){
				angLIDAR = 0;}
     if (zonePressed(&zone_2))
        umbDISTANCIA = 0;
     if (zonePressed(&zone_3))
        brilloDISP = 30;
				setBrillo(brilloDISP);
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
		 if (zoneNewPressed(&zone_8)){
			 (brilloDISP < 100) ? brilloDISP ++ : brilloDISP;
			 setBrillo(brilloDISP);	 // incrementamos el brillo de 0.01 en 0.01 
		 }
		 if (zoneNewPressed(&zone_9)){
			 (brilloDISP == 30) ? brilloDISP : brilloDISP --;
			 setBrillo(brilloDISP);	 // incrementamos el brillo de 0.01 en 0.01 
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

