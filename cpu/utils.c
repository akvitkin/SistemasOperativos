#include "utils.h"

pcb * pcb_create() {
  pcb * pcb_nuevo;

  pcb_nuevo = malloc(sizeof(pcb));

  if (pcb_nuevo == NULL) {
    return NULL;
  }

  pcb_nuevo -> instrucciones = list_create();
  if (pcb_nuevo -> instrucciones == NULL) {
    free(pcb_nuevo);
    return NULL;

  }

  return pcb_nuevo;
}

void pcb_destroy(pcb * pcb_destruir) {
  list_destroy_and_destroy_elements(pcb_destruir -> instrucciones, instruccion_destroy);
  free(pcb_destruir);
}

void instruccion_destroy(void* instruccion_a_destruir){
	Instruccion* instr = (Instruccion*) instruccion_a_destruir;
	free(instr);
}

void enviar_pcb_interrupcion(pcb* pcb_a_enviar, int socket_cliente){
	int pcb_bytes = sizeof(int) + 3*sizeof(unsigned int) + 2*sizeof(double) + list_size(pcb_a_enviar->instrucciones) * sizeof(Instruccion);
	int bytes = pcb_bytes + sizeof(double) + 2*sizeof(int);

	void* a_enviar = serializar_mensaje_interrupcion(pcb_a_enviar, bytes);

	//Enviamos estructura de bloqueo de pcb
	send(socket_cliente, a_enviar, bytes, MSG_WAITALL);
	log_info(cpu_info_logger, "Mensaje de interrupción enviado");
	pcb_destroy(pcb_a_enviar);
	free(a_enviar);
}

void* serializar_mensaje_interrupcion(pcb* pcb_a_enviar, int bytes){

	void* memoria_asignada = malloc(bytes);

	int desplazamiento = 0;
	mensaje_cpu mensaje = PASAR_A_READY;

	memcpy(memoria_asignada + desplazamiento, &mensaje, sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(memoria_asignada + desplazamiento, &rafaga_actual, sizeof(double));
	desplazamiento += sizeof(double);
	serializar_pcb(pcb_a_enviar, memoria_asignada, desplazamiento);
	return memoria_asignada;
}

void enviar_pcb_bloqueo(pcb* pcb_a_enviar, unsigned int tiempo_bloqueo, int socket_cliente){
	int pcb_bytes = 2*sizeof(int) + 3*sizeof(unsigned int) + 2*sizeof(double) + list_size(pcb_a_enviar->instrucciones) * sizeof(Instruccion);
	int bytes = pcb_bytes + sizeof(unsigned int) + sizeof(double) + sizeof(int);

	void* a_enviar = serializar_mensaje_bloqueo(pcb_a_enviar, tiempo_bloqueo, bytes);

	//Enviamos estructura de bloqueo de pcb
	send(socket_cliente, a_enviar, bytes, MSG_WAITALL);
	log_info(cpu_info_logger, "Mensaje de bloqueo enviado");
	pcb_destroy(pcb_a_enviar);
	free(a_enviar);
}

void* serializar_mensaje_bloqueo(pcb* pcb_a_enviar, unsigned int tiempo_bloqueo, int bytes){

	void* memoria_asignada = malloc(bytes);

	int desplazamiento = 0;
	mensaje_cpu mensaje = PASAR_A_BLOQUEADO;

	memcpy(memoria_asignada + desplazamiento, &mensaje , sizeof(int));
	desplazamiento  += sizeof(int);
	memcpy(memoria_asignada + desplazamiento, &tiempo_bloqueo , sizeof(unsigned int));
	desplazamiento  += sizeof(unsigned int);
	memcpy(memoria_asignada + desplazamiento, &contador_rafaga , sizeof(double));
	desplazamiento  += sizeof(double);
	serializar_pcb(pcb_a_enviar, memoria_asignada, desplazamiento);
	return memoria_asignada;
}

void* serializar_pcb(pcb* pcb_a_enviar, void* memoria_asignada, int desplazamiento){


	memcpy(memoria_asignada + desplazamiento, &(pcb_a_enviar->id), sizeof(unsigned int));
	desplazamiento  += sizeof(unsigned int);
	memcpy(memoria_asignada + desplazamiento, &(pcb_a_enviar->tam_proceso), sizeof(unsigned int));
	desplazamiento  += sizeof(unsigned int);
	memcpy(memoria_asignada + desplazamiento, &(pcb_a_enviar->pc), sizeof(unsigned int));
	desplazamiento  += sizeof(unsigned int);
	memcpy(memoria_asignada + desplazamiento, &(pcb_a_enviar->rafaga), sizeof(double));
	desplazamiento  += sizeof(double);
	memcpy(memoria_asignada + desplazamiento, &(pcb_a_enviar->estimacion_anterior), sizeof(double));
	desplazamiento  += sizeof(double);
	memcpy(memoria_asignada + desplazamiento, &(pcb_a_enviar->tabla_paginas), sizeof(int));
	desplazamiento  += sizeof(int);

	serializar_instrucciones(memoria_asignada, desplazamiento, pcb_a_enviar->instrucciones);
	return memoria_asignada;
}

void serializar_instrucciones(void* memoria_asignada, int desplazamiento, t_list* instrucciones){
	int contador_de_instrucciones = 0;
	int cantidad_de_instrucciones = list_size(instrucciones);

	//Indicamos la cantidad de instrucciones que debe leer cpu
	memcpy(memoria_asignada + desplazamiento, &(cantidad_de_instrucciones), sizeof(int));
	desplazamiento  += sizeof(int);

	while(contador_de_instrucciones < cantidad_de_instrucciones){
		Instruccion* instruccion_aux = (Instruccion *)list_get(instrucciones, contador_de_instrucciones);
		memcpy(memoria_asignada + desplazamiento, instruccion_aux, sizeof(Instruccion));
		desplazamiento  += sizeof(Instruccion);
		contador_de_instrucciones++;
	}
}

void enviar_exit(int socket_cliente){

	mensaje_cpu mensaje = PASAR_A_EXIT;
	send(socket_cliente, &mensaje, sizeof(int), 0);
	log_info(cpu_info_logger, "Mensaje de exit enviado");
}

pcb * recibir_pcb(int socket_cliente){

	pcb* pcb_leido;

	pcb_leido = pcb_create();

	leer_y_asignar_pcb(socket_cliente, pcb_leido);

	return pcb_leido;
}

void leer_y_asignar_pcb(int socket_cliente, pcb* pcb_leido){

	int cantidad_de_instrucciones;
	int contador = 0;
	Instruccion * instruccion_aux;

	//Recibo el process id
	recv(socket_cliente, &(pcb_leido->id), sizeof(unsigned int), MSG_WAITALL);
	//Recibo el tamaño del proceso
	recv(socket_cliente, &(pcb_leido->tam_proceso), sizeof(unsigned int), MSG_WAITALL);
	//Recibo el program counter
	recv(socket_cliente, &(pcb_leido->pc), sizeof(unsigned int), MSG_WAITALL);
	//Recibo la estimacion de rafaga restante
	recv(socket_cliente, &(pcb_leido->rafaga), sizeof(double), MSG_WAITALL);
	//Recibo la estimacion anterior
	recv(socket_cliente, &(pcb_leido->estimacion_anterior), sizeof(double), MSG_WAITALL);
	//Recibo la estimacion de rafaga
	recv(socket_cliente, &(pcb_leido->tabla_paginas), sizeof(int), MSG_WAITALL);
	//Recibo la cantidad de instrucciones que posee el proceso
	recv(socket_cliente, &(cantidad_de_instrucciones), sizeof(int), MSG_WAITALL);

	//Recibo las instrucciones del proceso
	while(contador < cantidad_de_instrucciones){
		instruccion_aux = malloc(sizeof(Instruccion));

		recv(socket_cliente, instruccion_aux, sizeof(Instruccion), 0);
		list_add(pcb_leido->instrucciones, instruccion_aux);
		contador++;
	}
}

int conexion_servidor(char * ip, char * puerto){
  struct addrinfo hints;
  struct addrinfo * server_info;

  memset( & hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  getaddrinfo(ip, puerto, & hints, & server_info);

  int cliente = socket(server_info -> ai_family, server_info -> ai_socktype, server_info -> ai_protocol);

  connect(cliente, server_info -> ai_addr, server_info -> ai_addrlen);

  freeaddrinfo(server_info);
  return cliente;
}


void terminar_programa(int conexionA, int conexionB, int conexionC, t_log * logger, t_config * config) {
  log_destroy(logger);
  config_destroy(config);
  close(conexionA);
  close(conexionB);
  close(conexionC);
}
