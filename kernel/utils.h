#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <commons/log.h>
#include <commons/config.h>
#include <unistd.h>

// ---- VARIABLES ----//
t_log * logger;

// ---- FUNCIONES ----//
int iniciar_servidor(char * , char * );
int esperar_cliente(int);
void terminar_programa(int conexionA, int conexionB, int conexionC, t_log * logger, t_config * config);
int conexion_a_memoria(char * ip, char * puerto);

#endif
