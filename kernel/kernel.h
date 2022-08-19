
#ifndef KERNEL_H_
#define KERNEL_H_

#include <commons/config.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "instrucciones_handler.h"



//---------------------------------------------------------------
// ----------------- ENUMS Y VARIABLES GLOBALES  ----------------
//---------------------------------------------------------------

char * ip_memoria;
char * puerto_memoria;
char * ip_kernel;
char * puerto_escucha;
char * ip_cpu;
char * puerto_cpu_interrupt;
char * puerto_cpu_dispatch;
int grado_multiprogramacion;
int tiempo_maximo_bloqueado;
int conexion_consola;
int conexion_dispatch;
int conexion_interrupt;
t_config * kernel_config;




//---------------------------------------------------------------
// ----------------- DECLARACION DE FUNCIONES  ------------------
//---------------------------------------------------------------


void inicializar_semaforos();
void inicializar_planificador_corto_plazo(pthread_t * hilo_running);
void inicializar_hilo_pasar_ready(pthread_t * hilo_ready);
void inicializar_cpu_dispatch_handler(pthread_t* hilo_dispatch_handler);


#endif /* KERNEL_H_ */
