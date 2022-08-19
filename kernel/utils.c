#include "utils.h"

int conexion_a_memoria(char * ip, char * puerto) {
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

int iniciar_servidor(char * IP, char * PUERTO) {
  logger = log_create("connection.log", "connection", 1, LOG_LEVEL_INFO);
  int socket_servidor;
  struct addrinfo hints, * servinfo;
  memset( & hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  getaddrinfo(IP, PUERTO, & hints, & servinfo);
  // Creamos el socket de escucha del servidor
  socket_servidor = socket(
    servinfo -> ai_family,
    servinfo -> ai_socktype,
    servinfo -> ai_protocol
  );
  // Asociamos el socket a un puerto
  bind(socket_servidor, servinfo -> ai_addr, servinfo -> ai_addrlen);
  // Escuchamos las conexiones entrantes
  listen(socket_servidor, SOMAXCONN);
  freeaddrinfo(servinfo);
  return socket_servidor;
}

int esperar_cliente(int socket_servidor) {
	log_info(logger, "Listo para recibir cliente");
  // Aceptamos un nuevo cliente
  int socket_cliente = accept(socket_servidor, NULL, NULL);
  log_info(logger, "Se conecto un cliente a kernel!");

  return socket_cliente;
}

void terminar_programa(int conexionA, int conexionB, int conexionC, t_log * logger, t_config * config) {
  log_destroy(logger);
  config_destroy(config);
  close(conexionA);
  close(conexionB);
  close(conexionC);
}
