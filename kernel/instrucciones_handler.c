#include "instrucciones_handler.h"

void* atender_instrucciones_cliente(void* pointer_argumentos) {
	//Recibimos el int del cliente como un void* y lo reconvertimos mediante una asignación y utilizando (int*)
	// para indicar que guarde el void* como int* en cliente.
	argumentos* pointer_args = (argumentos*) pointer_argumentos;
	int cliente_fd = pointer_args->cliente_fd;
	t_list* instrucciones = pointer_args->instrucciones;

	free(pointer_args);

	//Recibe primero el tamaño del proceso.
	unsigned int tam_proceso;
	recv(cliente_fd, &tam_proceso, sizeof(unsigned int), MSG_WAITALL);
  while (1) {
	Instruccion* instruccionAux = malloc(sizeof(Instruccion));
	instruccionAux->params[0] = 0;
	instruccionAux->params[1] = 0;
    int cod_op = recibir_int(cliente_fd);
    instruccionAux->tipo = cod_op;
    switch (cod_op) {
    case I_O:
    case READ:
      instruccionAux->params[0] = recibir_int(cliente_fd);
      break;
    case WRITE:
    case COPY:
      instruccionAux->params[0] = recibir_int(cliente_fd);
      instruccionAux->params[1] = recibir_int(cliente_fd);
      break;
    case NO_OP:
    case EXIT:
      break;
    case -1:
    	crear_proceso(instrucciones, tam_proceso, cliente_fd);
      return NULL;
    default:
      break;
    }
    list_add(instrucciones, instruccionAux);
  }


}

void crear_proceso(t_list * instrucciones,  unsigned int tam_proceso, int socket_cliente){

	unsigned int pid_creado = crear_pcb_new(instrucciones, tam_proceso);

	relacion_consola_proceso* rel_consola_proceso = malloc(sizeof(relacion_consola_proceso));

	rel_consola_proceso->pid = pid_creado;

	rel_consola_proceso->conexion_consola = socket_cliente;

	list_add(lista_relacion_consola_proceso, rel_consola_proceso);

	sem_post(&sem_sincro_ready);
}


int recibir_int(int socket_cliente) {
  int leido;
  recv(socket_cliente, & leido, sizeof(int), MSG_WAITALL);
  return leido;
}


