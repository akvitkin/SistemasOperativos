#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/config.h>


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



//---------------------------------------------------------------
// ----------------- DECLARACION DE FUNCIONES  ------------------
//---------------------------------------------------------------

void enviar_tam_proceso(unsigned int tam, int conexion);
void enviar_instruccion(Instruccion instruccion, int conexion);
int conexion_a_kernel(char * ip, char * puerto);
void liberar_conexion(int socket_cliente);
void terminar_programa(int conexion, t_log * logger1, t_log * logger2, t_config * config);


#endif /* UTILS_H_ */
