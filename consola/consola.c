#include "consola.h"

int main(int argc, char ** argv) {

  //Crear logger.
  error_logger = log_create("errors.log", "error_logger", 1, LOG_LEVEL_ERROR);
  info_logger = log_create("consola.log", "consola_info", 1, LOG_LEVEL_INFO);

  consola_config = config_create("consola.config");
  ip = strdup(config_get_string_value(consola_config, "IP_KERNEL"));
  puerto_kernel = strdup(config_get_string_value(consola_config, "PUERTO_KERNEL"));

  //inicializar conexion con kernel.
  int conexion = conexion_a_kernel(ip, puerto_kernel);

  int tam_proceso = atoi(argv[2]);

  log_info(info_logger, "Tamaño del proceso recibido: %d", tam_proceso);

  //Enviar tamaño de proceso.
  enviar_tam_proceso(tam_proceso, conexion);

  //Parser de instrucciones
  char* path_archivo_instrucciones = strdup(argv[1]);
  log_info(info_logger, "Recibí el path %s", path_archivo_instrucciones);

  clock_gettime(CLOCK_MONOTONIC, &tiempo_inicio);

  leer_y_enviar_archivo_de_instrucciones(path_archivo_instrucciones, conexion);

  bool finalizo_exitosamente;
  recv(conexion, &finalizo_exitosamente, sizeof(bool), 0);
  clock_gettime(CLOCK_MONOTONIC, &tiempo_fin);

  if(finalizo_exitosamente){
	  log_info(info_logger, "El proceso finalizó exitosamente!");
	  int segundos_totales = tiempo_fin.tv_sec - tiempo_inicio.tv_sec;
	  int minutos_ejecucion = (int) floor(segundos_totales / 60);
	  int segundos_ejecucion = segundos_totales - minutos_ejecucion * 60;
	  log_info(info_logger, "Duración total %02d:%02d", minutos_ejecucion, segundos_ejecucion);
  }else{
	  log_error(error_logger, "Ha ocurrido un error y el proceso ha finalizado de manera forzada. :'(");
  }

  //Finalizar conexion socket, logger y config.
   terminar_programa(conexion, error_logger, info_logger, consola_config);
}
