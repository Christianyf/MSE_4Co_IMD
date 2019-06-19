/**
 * @file    usercode.c
 * @author  Christian Yanez
 * @date    2019-06-18
 * @version 0.1
 * @brief  Interfaz para manejo de driver i2c bbb eeprom 24lc256
 * @see 
 */

#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>

#define BUFFER_LENGTH 4  
#define BUFFER_DOBLE  8 
#define TAM 40

/*************Defición de variables locales*********************/            
char receive[BUFFER_DOBLE];  
char stringToSend[BUFFER_LENGTH];  
static char clave[TAM], opcion[TAM];

int caso,dat[TAM],salir=0;
int ret, fd;

/************Declaración de funciones internas*****************/
int ingresar_clave(void);
int validar_clave(void);

/************Programa principal********************************/
int main(){

   printf("Inicializando.....\n");
   fd = open("/dev/myeeprom", O_RDWR);          
   if (fd < 0){
      perror("Failed to open the device...");
      return errno;
   }

   printf("Primera lectura sobre el dispositivo...\n");
   ret = read(fd, receive, BUFFER_LENGTH);       
   if (ret < 0){
      perror("Failed to read the message from the device.");
      return errno;
   }
   
   strcpy(clave,receive);
   printf("Dato almacecnado: [%s]\n \n", clave);
   
   printf(" +------------------------------------+ \n");
   printf(" |  Control de Acceso EEPROM 24lc256  | \n");
   printf(" +------------------------------------+ \n");
   printf(" | Implementación de driver i2c       | \n");
   printf(" | Linux embebido                     | \n");
   printf(" | Beaglebone black                   | \n");
   printf(" +------------------------------------+ \n\n");
   
   	do{
		printf(" Seleccione una opción\n");
		printf("1 Cambiar clave\n");
		printf("2 Validar clave\n");
		printf("3 Salir\n");
		
		scanf("%[^\n]%*c", opcion);
		
		if (strcmp(opcion,"1")==0) {
			caso=1;
		}else if(strcmp(opcion,"2")==0){
			caso=2;
		}else if(strcmp(opcion,"3")==0){
			caso=3;
		}else{
			caso=0;
		}
		
		switch(caso){
		case 1:
			if(validar_clave()){
				ingresar_clave();
				salir=1;
			}else{
			}
		break;
		case 2:
			validar_clave();
		break;
		case 3:
			salir=1;
		break;
		default:
			printf("Opción Incorrecta\n");
			salir=1;
		break;
		} 

	}while(salir==0);
   		
   printf("End of the program\n");   
   return 0;
}
/***********Definición de funciones internas****************/
int ingresar_clave(void){
	
   printf("Ingrese la nueva clave:\n");
   scanf("%[^\n]%*c", stringToSend);                
   printf("La nueva clave es [%s].\n", stringToSend);
   
   ret = write(fd, stringToSend, strlen(stringToSend)); 
   if (ret < 0){
      perror("Failed to write the message to the device.");
      return errno;
   }
	
	return 0;
}

int validar_clave(void){
	char texto1[BUFFER_DOBLE],usuario[BUFFER_DOBLE];
	
	printf("Ingrese su clave\n");
	scanf("%[^\n]%*c", texto1); 
				 
	if (strcmp(texto1, receive)==0) {
		printf("BIENVENIDO\n");
		return 1;
	}
		 
	else{
		printf("ACCESO DENEGADO\n");
		return 0;
	}	
}
