#include "utils.h"


int conexion_a_kernel(char * ip, char * puerto) {
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

void enviar_tam_proceso(unsigned int tam, int conexion) {
  unsigned int tam_proceso = tam;
  send(conexion, &tam_proceso, sizeof(unsigned int), 0);
}

void enviar_instruccion(Instruccion instruccion, int conexion) {
  send(conexion, & instruccion.tipo, sizeof(int), 0);
  switch (instruccion.tipo) {
  case I_O:
  case READ:
    send(conexion, & instruccion.params[0], sizeof(unsigned int), 0);
    break;
  case WRITE:
  case COPY:
    send(conexion, & instruccion.params[0], sizeof(unsigned int), 0);
      send(conexion, & instruccion.params[1], sizeof(unsigned int), 0);
    break;
  case NO_OP:
  case EXIT:
	  break;
  }

}

void terminar_programa(int conexion, t_log * logger1, t_log* logger2, t_config * config) {
  log_destroy(logger1);
  log_destroy(logger2);
  config_destroy(config);
  close(conexion);
}
