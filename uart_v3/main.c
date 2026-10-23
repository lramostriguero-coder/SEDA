
/*Created on: 1-Oct-2013
Author: J.M.V.C.

Mod: 17-Dic-2020  J.P.M.
  Modificado LED para la MiniDK2
	Representa el nombre del usuario
*/
#include <LPC17xx.H>
#include "uart.h"
#include <stdio.h>
#include <string.h>

char buffer[40];			// Buffer de recepción de 40 caracteres
char buffer_tx[40];		// Buffer de recepción de 40 caracteres
char *ptr_rx;					// puntero de recepción
char rx_completa;			// Flag de recepción de cadena que se activa a "1" al recibir la tecla return CR(ASCII=13)
char *ptr_tx;					// puntero de transmisión
char tx_completa;			// Flag de transmisión de cadena que se activa al transmitir el caracter null (fin de cadena)
int i = 0;

int angulo = 90;
float umbralDistancia = 15.5;
int brillo = 50;


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

t_estado estado;
	
	
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
						LPC_GPIO3->FIOPIN&=~(1<<26);
          else if (strcmp (buffer, "apaga\r") == 0)  
						LPC_GPIO3->FIOPIN|=(1<<26);
					
					else if (strcmp(buffer, "GetMedida\r")){
						sprintf(buffer_tx, "Angulo: %d grados, Umbral de Distancia: %.1f cm, Brillo: %d %%", angulo, umbralDistancia, brillo);
						tx_cadena_UART0(buffer_tx);
						estado = notificando_medida;
					}
					else if (strcmp(buffer, "SetUmbralDistancia\r")){
            if (sscanf(buffer, "SetUmbralDistancia %f", &umbralDistancia) == 1) { 
              sprintf(buffer_tx, "Umbral de distancia = %.1f cm\n\r", umbralDistancia);
              tx_cadena_UART0(buffer_tx);
              estado = notificando_medida;
						}
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


int main(void) {
   LPC_GPIO3->FIODIR |= (1<<26);	// P3.26 definido como salida  
   LPC_GPIO3->FIOCLR |= (1<<26);	// P3.26 apagado 
   ptr_rx=buffer;	                // inicializa el puntero de recepción al comienzo del buffer
   uart0_init(115200);					 	// configura la UART0 a 115200 baudios, 8 bits, 1 bit stop
              
   tx_cadena_UART0("Esto es una prueba de la UART0 del LPC1768 a 115200 baudios\n\r");
	 estado = enviando_msj_inicial;

   while(1){
			maquina_de_estados();
			i++;
	 }
}


