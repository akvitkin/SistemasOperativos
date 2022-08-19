#include "planificador.h"

unsigned int pid_contador = 0; //Process id
unsigned int grado_multiprogramacion = 0;


// ----------------- INICIALIZACION GENERAL DE LISTAS -----------------

void inicializar_listas_procesos() {
  new = list_create();
  bloqueado = list_create();
  ready = list_create();
  running = list_create();
  bloqueado_suspendido = list_create();
  ready_suspendido = list_create();
  exit_estado = list_create();
  lista_relacion_consola_proceso = list_create();
}

//-------------------------------------------------------------
// ----------------- PLANIFICADOR LARGO PLAZO -----------------
//-------------------------------------------------------------


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

pcb * inicializar_pcb(t_list * instrucciones, unsigned int tam_proceso) {
  pcb * pcb_creado = pcb_create();
  //Enviamos la señal de wait al semáforo para bloquear el recurso
  sem_wait(&semaforo_pid);
  //Accedemos al recurso compartido y ejecutamos las instrucciones de la zona crítica
  pid_contador++;
  pcb_creado -> id = pid_contador;
  //Enviamos la señal de post para liberar el recurso
  sem_post(&semaforo_pid);
  pcb_creado -> tam_proceso = tam_proceso;
  pcb_creado -> instrucciones = instrucciones;
  pcb_creado -> pc = 0;
  pcb_creado -> tabla_paginas = 0;
  pcb_creado -> rafaga = estimacion_inicial;
  pcb_creado -> estimacion_anterior = estimacion_inicial;

  return pcb_creado;
}

// PROCESO DE FINALIZACION DE PROCESO

void pcb_destroy(pcb * pcb_destruir) {
//	list_destroy(pcb_destruir->instrucciones);
  list_destroy_and_destroy_elements(pcb_destruir -> instrucciones, instruccion_destroy);
  free(pcb_destruir);
}

void instruccion_destroy(void* instruccion_a_destruir){
	Instruccion* instr = (Instruccion*) instruccion_a_destruir;
	free(instr);
}

void relacion_consola_proceso_destroy(relacion_consola_proceso* relacion_cp){
	close(relacion_cp->conexion_consola);
	free(relacion_cp);
}

unsigned int crear_pcb_new(t_list* instrucciones, unsigned int tam_proceso) {
	//Inicializar pcb
	pcb * pcb_nuevo = inicializar_pcb(instrucciones, tam_proceso);

	//Asignar pcb a new
	sem_wait(&semaforo_lista_new);
	list_add(new, pcb_nuevo);
	sem_post(&semaforo_lista_new);

	log_info(planificador_logger, "Proceso %d agregado a new", pcb_nuevo->id);

	return pcb_nuevo->id;
}

void planificador_largo_plazo_ready(){
    sem_wait(&semaforo_lista_new);
    pcb* pcb_new = (pcb*) list_get(new, 0);
    sem_post(&semaforo_lista_new);
    accion_memoria accion_a_ejecutar = INICIALIZAR_ESTRUCTURAS;
    send(conexion_memoria, &accion_a_ejecutar, sizeof(int), 0);
    send(conexion_memoria, &pcb_new->id, sizeof(unsigned int), 0);
    send(conexion_memoria, &pcb_new->tam_proceso, sizeof(unsigned int), 0);
    recv(conexion_memoria, &pcb_new->tabla_paginas, sizeof(int), 0);
    log_info(planificador_logger, "Recibí la tabla de páginas: %d \n", pcb_new->tabla_paginas);

    //Eliminar pcb de new y mover a ready
    sem_wait(&semaforo_lista_new);
    pcb * pcb_ready = list_remove(new, 0);
    sem_post(&semaforo_lista_new);

    sem_wait(&semaforo_lista_ready);
    if (strcmp(algoritmo_planificacion, "SRT") == 0) {
    	list_add_sorted(ready, pcb_ready, ordenar_por_estimacion_rafaga);
    	if(list_size(running)!= 0){
    		log_info(planificador_logger, "Enviamos mensaje para evaluar desalojo por llegada de nuevo proceso a ready (Desde new)");
    		//Se envía mensaje de interrumpir por socket interrupt
    		int mensaje_interrupt = 1;
    		send(interrupt, &mensaje_interrupt, sizeof(int), 0);
    	}
    } else list_add(ready, pcb_ready);

    sem_post(&semaforo_lista_ready);

    sem_wait(&semaforo_grado_multiprogramacion);
    grado_multiprogramacion++;
    sem_post(&semaforo_grado_multiprogramacion);

  	sem_post(&sem_hay_pcb_ready);

  	log_info(planificador_logger, "Agregado a ready el proceso %d desde new", pcb_ready->id);
}

void exit_largo_plazo(){
		sem_wait(&sem_sincro_running);
		log_info(planificador_logger, "\n exit largo plazo \n");
		sem_wait(&semaforo_lista_running);
		pcb* pcb_exit = (pcb*) list_remove(running, 0);
		log_info(planificador_logger, "pcb exit: \n pid: %d \n tam_proceso: %d \n pc: %d \n cantidad de instrucciones: %d \n", pcb_exit->id, pcb_exit->tam_proceso, pcb_exit->pc, list_size(pcb_exit->instrucciones));
		sem_post(&semaforo_lista_running);
		sem_post(&sem_cpu_libre);
		log_info(planificador_logger, "Agrego pcb a exit");
		list_add(exit_estado, pcb_exit);

		//Enviamos mensaje a memoria para hacer free
		accion_memoria mensaje_destruir_estructuras = DESTRUIR_ESTRUCTURAS;
		send(conexion_memoria, &mensaje_destruir_estructuras, sizeof(int), 0);
		send(conexion_memoria, &pcb_exit->id, sizeof(unsigned int), 0);
		send(conexion_memoria, &pcb_exit->tabla_paginas, sizeof(int), 0);
		//Memoria devuelve que fue ok
		bool mensaje_ok;
		recv(conexion_memoria, &mensaje_ok, sizeof(bool), 0);
		log_info(planificador_logger, "Se destruyeron correctamente estructuras de memoria \n");
		log_info(planificador_logger, "Remuevo pcb de exit, envío el mensaje a consola y libero la memoria\n");
		list_remove(exit_estado,0);

		sem_wait(&semaforo_pid_comparacion_exit);
		pid_comparacion_exit = pcb_exit->id;
		relacion_consola_proceso* relacion_cp = list_remove_by_condition(lista_relacion_consola_proceso, es_pid_en_exit);
		sem_post(&semaforo_pid_comparacion_exit);
		bool finalizacion_exitosa = true;
		send(relacion_cp->conexion_consola, &finalizacion_exitosa, sizeof(bool), 0);
		relacion_consola_proceso_destroy(relacion_cp);
		pcb_destroy(pcb_exit);

		sem_wait( & semaforo_grado_multiprogramacion);
		grado_multiprogramacion--;
		sem_post( & semaforo_grado_multiprogramacion);

		sem_post(&sem_multiprogramacion);

		log_info(planificador_logger, "El grado de multiprogramación es: %d \n", grado_multiprogramacion);
}



//---------------------------------------------------------------
// ----------------- PLANIFICADOR MEDIANO PLAZO -----------------
//---------------------------------------------------------------


void planificador_mediano_plazo_ready() {
  sem_wait(&semaforo_lista_ready_suspendido);
  pcb * pcb_ready = list_remove(ready_suspendido, 0);
  sem_post(&semaforo_lista_ready_suspendido);

  sem_wait( & semaforo_lista_ready);
  if (strcmp(algoritmo_planificacion, "SRT") == 0) {
	list_add_sorted(ready, pcb_ready, ordenar_por_estimacion_rafaga);
  } else {
	list_add(ready, pcb_ready);
  }
  sem_post( & semaforo_lista_ready);
  //Envío mensaje a cpu para que el planificador evalúe el desalojo en caso de SRT
  if (strcmp(algoritmo_planificacion, "SRT") == 0 && list_size(running)!= 0){
	  log_info(planificador_logger, "Luego de agregar a ready saliendo de ready suspendido evaluo desalojo");
	  //Se envía mensaje de interrumpir por socket interrupt
	  int mensaje_interrupt = 1;
	  send(interrupt, &mensaje_interrupt, sizeof(int), 0);
  }
  log_info(planificador_logger, "Agregado el proceso %d a ready desde ready suspendido", pcb_ready->id);
  sem_wait( & semaforo_grado_multiprogramacion);
  grado_multiprogramacion++;
  sem_post( & semaforo_grado_multiprogramacion);

  sem_post(&sem_hay_pcb_ready);

  log_info(planificador_logger, "El grado de multiprogramación es: %d \n", grado_multiprogramacion);

}

void mediano_plazo_bloqueado_suspendido(unsigned int pid){
	// Removemos de bloqueado y pasamos a bloqueado suspendido
	sem_wait(&semaforo_pid_comparacion);
	pid_comparacion = pid;
	pcb* pcb_a_suspender = list_remove_by_condition(bloqueado, es_pid_a_desbloquear);
	sem_post(&semaforo_pid_comparacion);

	if(pcb_a_suspender != NULL){
	  //Se envia a memoria para que pase a disco
	  int mensaje_suspender = SUSPENDER;
	  send(conexion_memoria, &mensaje_suspender, sizeof(int), 0);
	  send(conexion_memoria, &pcb_a_suspender->id, sizeof(unsigned int), 0);
	  send(conexion_memoria, &pcb_a_suspender->tabla_paginas, sizeof(int), 0);

	  log_info(planificador_logger, "Esperando confirmación de suspensión de memoria para el proceso %d\n", pcb_a_suspender->id);

	  //Esperamos confirmación de memoria
	  bool swap_ok;
	  recv(conexion_memoria, &swap_ok, sizeof(bool), 0);
	  log_info(planificador_logger, "Confirmación de suspensión de proceso %d recibida \n", pcb_a_suspender->id);

	  sem_wait(&semaforo_bloqueado_suspendido);
	  list_add(bloqueado_suspendido, pcb_a_suspender);
	  sem_post(&semaforo_bloqueado_suspendido);

	  sem_wait( & semaforo_grado_multiprogramacion);
	  grado_multiprogramacion--;
	  sem_post( & semaforo_grado_multiprogramacion);
	  log_info(planificador_logger, "Al suspender el proceso %d el grado de multiprogramación baja a: %d", pcb_a_suspender->id, grado_multiprogramacion);

	  //Envío post al hilo de bloqueo para que saque de bloqueado suspendido y pase a ready suspendido
	  sem_post(&sem_sincro_suspension);
	  sem_post(&sem_multiprogramacion);
	}
}


void* hilo_pasar_ready(void* args){
	while(1){
		sem_wait(&sem_sincro_ready);
		sem_wait(&sem_multiprogramacion);
		if(list_size(ready_suspendido) > 0){
			planificador_mediano_plazo_ready();
		}else{
			planificador_largo_plazo_ready();
		}
	}
}

//--------------------------------------------------------------
// ----------------- PLANIFICADOR CORTO PLAZO  -----------------
//--------------------------------------------------------------


void * hilo_de_corto_plazo_pasar_running(void * argumentos) {
  while (1) {
	  sem_wait(&sem_hay_pcb_ready);
	  sem_wait(&sem_cpu_libre);
      //Sacamos de lista de ready
      pcb * pcb_running = list_remove(ready, 0);
      mensaje_cpu mensaje_ejecutar = EJECUTAR;
      send(dispatch,&mensaje_ejecutar, sizeof(int), 0);
      enviar_pcb(pcb_running, dispatch);
      //Enviamos a running
      sem_wait(&semaforo_lista_running);
      list_add(running, pcb_running);
      sem_post(&semaforo_lista_running);
      sem_post(&sem_sincro_running);
      log_info(planificador_logger, "Agregamos el proceso %d a running", pcb_running->id);
  }
}

void planificador_de_corto_plazo_fifo_running(mensaje_dispatch* mensaje_cpu) {
    switch (mensaje_cpu->mensaje) {
    	case PASAR_A_BLOQUEADO: ;//Para arreglar error con la declaración de datos_bloqueo
    		//Casteo los datos según lo necesario en el caso particular
    		log_info(planificador_logger, "Pasar a bloqueado \n");
			bloqueo_pcb * datos_bloqueo = (bloqueo_pcb*) mensaje_cpu->datos;

			sem_wait(&sem_sincro_running);
			pcb* pcb_destruir = (pcb*)list_remove(running, 0);
    		pcb_destroy(pcb_destruir);
    		sem_post(&sem_cpu_libre);
    		list_add(bloqueado, datos_bloqueo->pcb_a_bloquear);
    		//Comienzo hilo de evaluacion de suspendido
			unsigned int* pid_bloqueo = malloc(sizeof(unsigned int));
			*pid_bloqueo = datos_bloqueo->pcb_a_bloquear->id;
			pthread_t hilo_suspension;
			pthread_create(&hilo_suspension, NULL, hilo_contador_suspension_por_bloqueo, pid_bloqueo);
			pthread_detach(hilo_suspension);
			log_info(planificador_logger, "pcb a bloquear: \n pid: %d \n tam_proceso: %d \n pc: %d \n rafaga: %f \n cantidad de instrucciones: %d \n", datos_bloqueo->pcb_a_bloquear->id, datos_bloqueo->pcb_a_bloquear->tam_proceso, datos_bloqueo->pcb_a_bloquear->pc, datos_bloqueo->pcb_a_bloquear->rafaga, list_size(datos_bloqueo->pcb_a_bloquear->instrucciones));
    		argumentos_hilo_bloqueo * args_bloqueo = malloc(sizeof(argumentos_hilo_bloqueo));
    		args_bloqueo -> tiempo_bloqueo = datos_bloqueo->tiempo_bloqueo;
    		args_bloqueo -> pcb_actualizado = datos_bloqueo->pcb_a_bloquear;
    		pthread_t hilo_bloqueo;
    		pthread_create( & hilo_bloqueo, NULL, hilo_bloqueo_proceso, args_bloqueo);
    		pthread_detach(hilo_bloqueo);
    		break;
    	default:
    		break;
  }
}

void planificador_de_corto_plazo_sjf_running(mensaje_dispatch * mensaje_cpu) {
    switch (mensaje_cpu->mensaje) {
		case PASAR_A_BLOQUEADO: ;//Para arreglar error con la declaración de datos_bloqueo
			//Casteo los datos según lo necesario en el caso particular
			bloqueo_pcb * datos_bloqueo = (bloqueo_pcb*) mensaje_cpu->datos;
			log_info(planificador_logger, "Rafaga real recibida: %f \n Rafaga estimada anterior: %f \n", datos_bloqueo->rafaga_real_anterior, datos_bloqueo->pcb_a_bloquear->rafaga);
			double estimacion_calculada = calcular_estimacion_rafaga(datos_bloqueo->rafaga_real_anterior, datos_bloqueo->pcb_a_bloquear->estimacion_anterior);
			datos_bloqueo->pcb_a_bloquear->rafaga = estimacion_calculada;
			datos_bloqueo->pcb_a_bloquear->estimacion_anterior = estimacion_calculada;
			log_info(planificador_logger, "Rafaga estimada asignada: %f \n", datos_bloqueo->pcb_a_bloquear->rafaga);
			log_info(planificador_logger, "\npcb a bloquear: \n pid: %d \n tam_proceso: %d \n pc: %d \n rafaga: %f \n cantidad de instrucciones: %d \n", datos_bloqueo->pcb_a_bloquear->id, datos_bloqueo->pcb_a_bloquear->tam_proceso, datos_bloqueo->pcb_a_bloquear->pc, datos_bloqueo->pcb_a_bloquear->rafaga, list_size(datos_bloqueo->pcb_a_bloquear->instrucciones));
			list_add(bloqueado, datos_bloqueo->pcb_a_bloquear);
			sem_wait(&sem_sincro_running);
			//Comienzo hilo de evaluacion de suspendido
			unsigned int* pid_bloqueo = malloc(sizeof(unsigned int));
			*pid_bloqueo = datos_bloqueo->pcb_a_bloquear->id;
			pthread_t hilo_suspension;
			pthread_create(&hilo_suspension, NULL, hilo_contador_suspension_por_bloqueo, pid_bloqueo);
			pthread_detach(hilo_suspension);
			log_info(planificador_logger, "Sacamos pcb actual de running \n");
			pcb* pcb_destruir = (pcb*)list_remove(running, 0);
			pcb_destroy(pcb_destruir);
			sem_post(&sem_cpu_libre);
			argumentos_hilo_bloqueo * args_bloqueo = malloc(sizeof(argumentos_hilo_bloqueo));
			args_bloqueo -> tiempo_bloqueo = datos_bloqueo->tiempo_bloqueo;
			args_bloqueo -> pcb_actualizado = datos_bloqueo->pcb_a_bloquear;
			args_bloqueo -> rafaga_anterior = datos_bloqueo->rafaga_real_anterior;
			pthread_t hilo_bloqueo;
			pthread_create( & hilo_bloqueo, NULL, hilo_bloqueo_proceso, args_bloqueo);
			pthread_detach(hilo_bloqueo);
			break;
		case PASAR_A_READY: ;//Para arreglar error con la declaración de datos_bloqueo
			//Casteo los datos según lo necesario en el caso particular
		    interrupcion_pcb* datos_interrupcion = (interrupcion_pcb*) mensaje_cpu->datos;
			pcb* pcb_interrupcion = datos_interrupcion->pcb_a_interrumpir;
			log_info(planificador_logger, "Recibí pcb N° %d para pasar a ready\n", pcb_interrupcion->id);
			pcb_interrupcion->rafaga -= datos_interrupcion->rafaga_real_anterior;
			log_info(planificador_logger, "La ráfaga actualizada del proceso N° %d es: %f", pcb_interrupcion->id, pcb_interrupcion->rafaga);
			sem_wait( & semaforo_lista_ready);
			int indice_lista = list_add_sorted(ready, pcb_interrupcion, ordenar_por_estimacion_rafaga);
			sem_post( & semaforo_lista_ready);
			sem_post(&sem_hay_pcb_ready);
			log_info(planificador_logger, "Agregamos a ready al proceso N° %d en la posición %d", pcb_interrupcion->id, indice_lista);
			//Sacamos de la lista de running al proceso una vez que finalizamos la evaluación de prioridades
			sem_wait(&sem_sincro_running);
			pcb_destroy((pcb*)list_remove(running, 0));
			sem_post(&sem_cpu_libre);
			log_info(planificador_logger, "Sacamos al pcb de running");
			break;
		default:
			break;
  }
}


bool es_pid_a_desbloquear(void * pcb_recibido) {
  pcb * pcb_comparacion = (pcb * ) pcb_recibido;
  return pcb_comparacion -> id == pid_comparacion;
}

bool es_pid_en_exit(void* rel_consola_proceso){
	relacion_consola_proceso* relacion = (relacion_consola_proceso *) rel_consola_proceso;
	return relacion->pid == pid_comparacion_exit;
}

bool ordenar_por_estimacion_rafaga(void * unPcb, void * otroPcb) {
  pcb * pcbUno = (pcb * ) unPcb;
  pcb * pcbDos = (pcb * ) otroPcb;

  return pcbUno -> rafaga <= pcbDos -> rafaga;
}

double calcular_estimacion_rafaga(double rafaga_real_anterior, double estimacion_anterior) {
	log_info(planificador_logger, "Ráfagas para cálculo: real anterior: %f, estimada anterior: %f \n", rafaga_real_anterior, estimacion_anterior);
	double rafaga_calculada = alfa * rafaga_real_anterior + (1 - alfa) * estimacion_anterior;
	log_info(planificador_logger, "Ráfaga estimada obtenida: %f", rafaga_calculada);
	return rafaga_calculada;
}

void* hilo_bloqueo_proceso(void * argumentos) {
	//Hacemos wait al semaforo que controla que haya un unico dispositivo de I/O
	sem_wait(&sem_entrada_salida);
	log_info(planificador_logger, "Hilo bloqueo \n");
	argumentos_hilo_bloqueo * args = (argumentos_hilo_bloqueo *) argumentos;
	unsigned int tiempo_bloqueo = args -> tiempo_bloqueo;
	pcb * pcb_actualizado = args -> pcb_actualizado;
	free(args);
	//Simulo la duración de la entrada salida
	log_info(planificador_logger, "Comienza I/O proceso %d", pcb_actualizado->id);
	sleep(tiempo_bloqueo / 1000);
	//Busco si el proceso se encuentra en la lista de bloqueados
	sem_wait( & semaforo_pid_comparacion);
	pid_comparacion = pcb_actualizado -> id;
	pcb* pcb_bloqueado = (pcb*)list_remove_by_condition(bloqueado, es_pid_a_desbloquear);
	sem_post( & semaforo_pid_comparacion);
	if(pcb_bloqueado != NULL){
		log_info(planificador_logger, "Agregamos a ready el proceso %d después de bloqueo", pcb_bloqueado->id);
		//Agrego a ready el pcb bloqueado
		if(strcmp(algoritmo_planificacion, "SRT") == 0){
			sem_wait(&semaforo_lista_ready);
			list_add_sorted(ready, pcb_bloqueado, ordenar_por_estimacion_rafaga);
			sem_post(&semaforo_lista_ready);
			sem_post(&sem_hay_pcb_ready);
			//Si tengo algo en running evaluo el desalojo
			if(list_size(running) > 0){
				log_info(planificador_logger, "Enviamos mensaje para evaluar desalojo por llegada de nuevo proceso a ready (De bloqueado)");
				int mensaje_interrupt = 1;
				send(interrupt, &mensaje_interrupt, sizeof(int), 0);
			}
		}else{
			//Agrego el proceso a ready según FIFO
			sem_wait(&semaforo_lista_ready);
			list_add(ready, pcb_bloqueado);
			sem_post(&semaforo_lista_ready);
			sem_post(&sem_hay_pcb_ready);
		}
	}else{
		//Busco el proceso en la lista de bloqueados suspendidos
		sem_wait(&sem_sincro_suspension);
		sem_wait( & semaforo_pid_comparacion);
		pid_comparacion = pcb_actualizado -> id;
		pcb* pcb_suspendido = (pcb*)list_remove_by_condition(bloqueado_suspendido, es_pid_a_desbloquear);
		sem_post( & semaforo_pid_comparacion);
		if(pcb_suspendido != NULL){
			//Agrego el pcb encontrado a la lista de ready suspendido
			sem_wait(&semaforo_lista_ready_suspendido);
			list_add(ready_suspendido, pcb_suspendido);
			sem_post(&semaforo_lista_ready_suspendido);
			sem_post(&sem_sincro_ready);
			log_info(planificador_logger, "Agregado proceso N° %d a la lista de ready suspendido", pcb_suspendido->id);
		}else{
			log_error(planificador_logger, "No encontré el proceso %d en la lista de suspendidos ni de bloqueados", pcb_actualizado->id);
		}
	}
  sem_post(&sem_entrada_salida);
  return NULL;
}

void* hilo_contador_suspension_por_bloqueo(void* args){
	unsigned int pid = *((unsigned int*) args);
	free(args);
	sleep(tiempo_maximo_bloqueado/1000);
	mediano_plazo_bloqueado_suspendido(pid);
	return NULL;
}

//---------------------------------------------------------------
// ------------ MANEJO DE COMUNICACIÓN CON CPU DISPATCH ---------
//---------------------------------------------------------------

void* cpu_dispatch_handler(void* args){
	while(1){
		mensaje_cpu accion_recibida;
		recv(dispatch, &accion_recibida, sizeof(int), MSG_WAITALL);
		switch(accion_recibida){
			case PASAR_A_BLOQUEADO: ;
				log_info(planificador_logger, "Pasar a bloqueado \n");
				mensaje_dispatch* mensaje_bloqueo = malloc(sizeof(mensaje_dispatch));
				bloqueo_pcb* datos_bloqueo = malloc(sizeof(bloqueo_pcb));
				mensaje_bloqueo->datos = datos_bloqueo;
				mensaje_bloqueo->mensaje = accion_recibida;
				recv(dispatch, &(datos_bloqueo->tiempo_bloqueo), sizeof(unsigned int), 0);
				log_info(planificador_logger, "Tiempo bloqueo recibido: %d", datos_bloqueo->tiempo_bloqueo);
				recv(dispatch, &(datos_bloqueo->rafaga_real_anterior), sizeof(double), 0);
				log_info(planificador_logger, "rafaga_real_anterior recibida: %f", datos_bloqueo->rafaga_real_anterior);
				datos_bloqueo->pcb_a_bloquear = recibir_pcb(dispatch);
				log_info(planificador_logger, "pcb a bloquear recién recibido: \n pid: %d \n tam_proceso: %d \n pc: %d \n rafaga: %f \n cantidad de instrucciones: %d \n", datos_bloqueo->pcb_a_bloquear->id, datos_bloqueo->pcb_a_bloquear->tam_proceso, datos_bloqueo->pcb_a_bloquear->pc, datos_bloqueo->pcb_a_bloquear->rafaga, list_size(datos_bloqueo->pcb_a_bloquear->instrucciones));
				if(strcmp(algoritmo_planificacion, "SRT") == 0){
					planificador_de_corto_plazo_sjf_running(mensaje_bloqueo);
				}else{
					planificador_de_corto_plazo_fifo_running(mensaje_bloqueo);
				}
				break;
			case PASAR_A_READY: ;
				log_info(planificador_logger, "Pasar a ready en cpu dispatch handler \n");
				mensaje_dispatch* mensaje_interrupcion = malloc(sizeof(mensaje_dispatch));
				interrupcion_pcb* datos_interrupcion = malloc(sizeof(interrupcion_pcb));
				mensaje_interrupcion->datos = datos_interrupcion;
				mensaje_interrupcion->mensaje = accion_recibida;
				recv(dispatch, &datos_interrupcion->rafaga_real_anterior, sizeof(double), 0);
				datos_interrupcion->pcb_a_interrumpir = recibir_pcb(dispatch);
				if(strcmp(algoritmo_planificacion, "SRT") == 0){
					planificador_de_corto_plazo_sjf_running(mensaje_interrupcion);
				}else
					planificador_de_corto_plazo_fifo_running(mensaje_interrupcion);
				break;
			case PASAR_A_EXIT:
				log_info(planificador_logger, "Pasar a exit");
				exit_largo_plazo();
				break;
			default:
				//Loggear error: no se ha podido interpretar el mensaje de cpu. ERROR DE KERNEL
				break;
		}
	}
	return NULL;
}

//---------------------------------------------------------------
// ----------------- SERIALIZACION Y ENVIO DE PCB ---------------
//---------------------------------------------------------------

void* serializar_pcb(pcb* pcb_a_enviar, int bytes){

	void* memoria_asignada = malloc(bytes);
	int desplazamiento = 0;

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

void enviar_pcb(pcb* pcb_a_enviar, int socket_cliente)
{
	int bytes = 2*sizeof(int) + 3*sizeof(unsigned int) + 2*sizeof(double) + list_size(pcb_a_enviar->instrucciones) * sizeof(Instruccion);
	void* a_enviar = serializar_pcb(pcb_a_enviar, bytes);

	send(socket_cliente, a_enviar, bytes, MSG_WAITALL);

	free(a_enviar);
}

pcb* recibir_pcb(int socket_cliente){

	pcb* pcb_leido = pcb_create();

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
	//Recibo la rafaga restante
	recv(socket_cliente, &(pcb_leido->rafaga), sizeof(double), MSG_WAITALL);
	//Recibo la estimacion anterior
	recv(socket_cliente, &(pcb_leido->estimacion_anterior), sizeof(double), MSG_WAITALL);
	//Recibo la tabla de poaginas
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
