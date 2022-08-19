#include "kernel.h"

#include "utils.h"

int main(int argc, char ** argv) {

  pthread_t hilo_running;
  pthread_t hilo_dispatch_handler;
  pthread_t hilo_pasar_ready;
  inicializar_listas_procesos();



  t_log * logger_kernel = log_create("kernelerrors.log", "kernel.c", 1, LOG_LEVEL_ERROR);
  planificador_logger = log_create("planificador_logger.log", "Planificador", 1, LOG_LEVEL_INFO);

  char* path_archivo_config = strdup(argv[1]);

  kernel_config = config_create(path_archivo_config);
  ip_kernel = strdup(config_get_string_value(kernel_config, "IP_KERNEL"));
  puerto_escucha = config_get_string_value(kernel_config, "PUERTO_ESCUCHA");

  ip_memoria = strdup(config_get_string_value(kernel_config,"IP_MEMORIA"));
  puerto_memoria = config_get_string_value(kernel_config,"PUERTO_MEMORIA");

  puerto_cpu_dispatch = strdup(config_get_string_value(kernel_config,"PUERTO_CPU_DISPATCH"));
  puerto_cpu_interrupt = strdup(config_get_string_value(kernel_config,"PUERTO_CPU_INTERRUPT"));

  estimacion_inicial = (unsigned int) config_get_int_value(kernel_config,"ESTIMACION_INICIAL");
  algoritmo_planificacion = strdup(config_get_string_value(kernel_config,"ALGORITMO_PLANIFICACION"));
  alfa = config_get_double_value(kernel_config,"ALFA");
  limite_grado_multiprogramacion = (unsigned int) config_get_int_value(kernel_config,"GRADO_MULTIPROGRAMACION");
  tiempo_maximo_bloqueado = config_get_int_value(kernel_config,"TIEMPO_MAXIMO_BLOQUEADO");

  conexion_memoria = conexion_a_memoria(ip_memoria, puerto_memoria);
  conexion_consola = iniciar_servidor(ip_kernel, puerto_escucha);
  conexion_dispatch = iniciar_servidor(ip_kernel, puerto_cpu_dispatch);
  conexion_interrupt = iniciar_servidor(ip_kernel, puerto_cpu_interrupt);

  dispatch = esperar_cliente(conexion_dispatch);
  interrupt = esperar_cliente(conexion_interrupt);

  inicializar_planificador_corto_plazo(&hilo_running);
  inicializar_hilo_pasar_ready(&hilo_pasar_ready);
  inicializar_cpu_dispatch_handler(&hilo_dispatch_handler);

  //Inicializamos el semáforo para el process id del planificador de largo plazo
  inicializar_semaforos();

  while (1) {
	// pthread requiere que el 4to argumento (argumentos de la funcion que pasamos) sea un void*
	// entonces creamos un int*, le asignamos memoria dinámica y le guardamos el int que retorna la conexión.
	t_list * instrucciones = list_create();
	argumentos *argumentos = malloc(sizeof(argumentos));
	argumentos->instrucciones = instrucciones;
	argumentos->cliente_fd = esperar_cliente(conexion_consola);


    if (argumentos->cliente_fd < 0) {
      //handlear error en logger y free para evitar memory leak.
      free(argumentos);
      log_info(logger_kernel, "Falló conexión con el cliente.");
    }

    else {
    	// HANDLER DE INSTRUCCIONES DE CLIENTE MEDIANTE HILOS (KLT).
    	pthread_t handler;
    	//La funcion (3er argumento) que recibe pthread debe ser del tipo void* y los argumentos (4to argumento) deben ser void*
    	if(pthread_create(&handler, NULL, atender_instrucciones_cliente,argumentos) != 0) {
    		// Si el pthread_create falla, handlea el error en el logger del kernel y free para evitar memory leak.
    	    free(argumentos);
    		log_info(logger_kernel, "No se pudo atender al cliente por error de Kernel.");
    	}else{
    		pthread_detach(handler);
    	}
    }
  }
  terminar_programa(conexion_consola, conexion_dispatch, conexion_interrupt, logger_kernel, kernel_config);
}

void inicializar_semaforos(){
	sem_init(&semaforo_pid, 0, 1);
	sem_init(&semaforo_pid_comparacion, 0, 1);
	sem_init(&semaforo_pid_comparacion_exit, 0, 1);
	sem_init(&semaforo_lista_new, 0, 1);
	sem_init(&semaforo_lista_ready, 0, 1);
    sem_init(&semaforo_lista_ready_suspendido, 0,1);
    sem_init(&semaforo_lista_running, 0,1);
    sem_init(&semaforo_bloqueado_suspendido, 0, 1);
    sem_init(&semaforo_grado_multiprogramacion,0,1);
    sem_init(&sem_entrada_salida, 0, 1);
    sem_init(&sem_sincro_running, 0, 0);
    sem_init(&sem_sincro_suspension, 0, 0);
    sem_init(&sem_cpu_libre, 0, 1);
    sem_init(&sem_hay_pcb_ready, 0, 0);
    sem_init(&sem_sincro_ready,0 ,0);
    sem_init(&sem_multiprogramacion, 0, limite_grado_multiprogramacion);
}

void inicializar_hilo_pasar_ready(pthread_t * hilo_ready){
	pthread_create(hilo_ready, NULL, hilo_pasar_ready, NULL);
	pthread_detach(*hilo_ready);
}

void inicializar_planificador_corto_plazo(pthread_t * hilo_running){
	pthread_create(hilo_running, NULL, hilo_de_corto_plazo_pasar_running, NULL);
	pthread_detach(*hilo_running);
}

void inicializar_cpu_dispatch_handler(pthread_t* hilo_dispatch_handler){
	pthread_create(hilo_dispatch_handler, NULL, cpu_dispatch_handler, NULL);
	pthread_detach(*hilo_dispatch_handler);
}


