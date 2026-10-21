
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

char buffer[30];			// Buffer de recepción de 30 caracteres
char buffer_tx[30];		// Buffer de recepción de 30 caracteres
char *ptr_rx;					// puntero de recepción
char rx_completa;			// Flag de recepción de cadena que se activa a "1" al recibir la tecla return CR(ASCII=13)
char *ptr_tx;					// puntero de transmisión
char tx_completa;			// Flag de transmisión de cadena que se activa al transmitir el caracter null (fin de cadena)
char fin=0;

int main(void) {
   LPC_GPIO3->FIODIR |= (1<<26);	// P3.26 definido como salida  
   LPC_GPIO3->FIOCLR |= (1<<26);	// P3.26 apagado 
   ptr_rx=buffer;	                // inicializa el puntero de recepción al comienzo del buffer
   uart0_init(9600);					 		// configura la UART0 a 9600 baudios, 8 bits, 1 bit stop
                  
   tx_cadena_UART0("Esto es una prueba de la UART0 del LPC1768 a 9600 baudios\n\r"
									 "Introduce tu nombre y apellidos\n\r");

   while(tx_completa==0); 											 // Espera a que se tranmita la cadena
   while(rx_completa==0); 											 // Espera introducir una cadena de caracteres terminada con CR (0x0D)
   sprintf(buffer_tx,"\nHola %s \r\n\n",buffer); // Prepara la respuesta inclyendo la info recibida
   tx_cadena_UART0(buffer_tx);
   while(tx_completa==0); 											 // Espera a que se tranmita la cadena

   rx_completa=0;					// Borrar flag 
      
   tx_cadena_UART0("Introduce un comando: (apaga o enciende) \n\r");
   

	 do{
      if(rx_completa){					 	// Comprabamos la llegada de una cadena por RXD
         rx_completa=0; 					// Borrar flag para otra recepción
         if (strcmp (buffer, "enciende\r") == 0) 		LPC_GPIO3->FIOPIN&=~(1<<26);
         else if (strcmp (buffer, "apaga\r") == 0)  LPC_GPIO3->FIOPIN|=(1<<26);
         else if (strcmp (buffer, "fin\r") == 0)		fin=1;
         else tx_cadena_UART0("Comando erroneo\n\r");
      }
   }while(fin==0);
   
   tx_cadena_UART0("FIN del programa\n\r"); 

   while(1);
}


