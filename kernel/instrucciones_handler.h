#ifndef INSTRUCCIONES_HANDLER_H_
#define INSTRUCCIONES_HANDLER_H_

#include <commons/collections/list.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <semaphore.h>
#include "planificador.h"



//---------------------------------------------------------------
// ----------------- ENUMS Y VARIABLES GLOBALES  ----------------
//---------------------------------------------------------------

typedef enum {
  NO_OP,
  I_O,
  READ,
  WRITE,
  COPY,
  EXIT
}TipoInstruccion;

typedef struct Instruccion {
  TipoInstruccion tipo;
  unsigned int params[2];
}Instruccion;

typedef struct {
  int cliente_fd;
  t_list * instrucciones;
}argumentos;


//---------------------------------------------------------------
// ----------------- DECLARACION DE FUNCIONES  ------------------
//---------------------------------------------------------------

int recibir_int(int socket_cliente);
void* atender_instrucciones_cliente(void* pointer_void_cliente_fd);
void crear_proceso(t_list * instrucciones,  unsigned int tam_proceso, int socket_cliente);


#endif /* INSTRUCCIONES_HANDLER_H_ */
