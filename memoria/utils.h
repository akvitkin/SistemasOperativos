#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <commons/log.h>
#include <commons/config.h>



//---------------------------------------------------------------
// ----------------- ENUMS Y VARIABLES GLOBALES  ----------------
//---------------------------------------------------------------


t_log * logger;


//---------------------------------------------------------------
// ----------------- DECLARACION DE FUNCIONES  ------------------
//---------------------------------------------------------------

void terminar_programa(int conexion, t_log * logger, t_config * config);
void recibir_mensaje(int);
int iniciar_servidor(char * , char * );
int esperar_cliente(int);
int esperar_cliente(int);

#endif
