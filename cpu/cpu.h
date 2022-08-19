#ifndef CPU_H_
#define CPU_H_

#include <commons/log.h>
#include <stdint.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <semaphore.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include "utils.h"

//---------------------------------------------------------------
// ------------------------- ESTRUCTURAS ------------------------
//---------------------------------------------------------------

typedef struct{
	int tabla_segundo_nivel;
	double entrada_tabla_segundo_nivel;
	double direccion_fisica;
} datos_direccion;

typedef struct
{
	int pagina;
	int marco;
} entrada_tlb;

//---------------------------------------------------------------
// ----------------- ENUMS Y VARIABLES GLOBALES  ----------------
//---------------------------------------------------------------

typedef enum {
	OBTENER_TABLA_SEGUNDO_NIVEL,
	OBTENER_NUMERO_MARCO,
	LEER,
	ESCRIBIR
} mensaje_memoria;

char*	reemplazo_tlb;
char*	ipMemoria;
char*	puertoMemoria;
char* 	ipKernel;
char*	puertoEscuchaDispatch;
char*	puertoEscuchaInterrupt;
int entradas_por_tabla;
int tamanio_pagina;
int conexion_memoria;
int conexion_dispatch;
int conexion_interrupt;
int	entradas_tlb ;
int	retardo_NOOP;
int pagina_comparacion_tlb;
int nro_tabla_primer_nivel;
unsigned int pid_en_ejecucion;
struct timespec tiempo_inicio;
struct timespec tiempo_fin;
bool detener_ejecucion;
bool contador_rafaga_inicializado;
bool hay_interrupciones;
bool atendiendo_interrupcion;
sem_t sem_tlb_pagina_comparacion;
sem_t sem_sincro_contador;
sem_t sem_contador;
t_config* cpu_config;
t_log * cpu_logger;
t_list* tabla_tlb;



//---------------------------------------------------------------
// ----------------- DECLARACION DE FUNCIONES  ------------------
//---------------------------------------------------------------

void* interrupcion_handler(void* args);
void* decode (pcb* pcb_decode, Instruccion * instruccion_decode);
void* fetch(pcb* pcb_fetch);
void abrirArchivoConfiguracion();
void ciclo(pcb* paquetePcb);
void ejecutar_NO_OP();
void ejecutar_I_O(pcb* pcb_a_bloquear, unsigned int tiempo_bloqueo);
void ejecutar_exit(pcb * pcb_exit);
void inicializar_hilo_conexion_interrupt(pthread_t* hilo_interrupcion_handler);
void atender_interrupcion(pcb* pcb_interrumpido);
datos_direccion mmu(unsigned int dir_logica, int numero_tabla_primer_nivel);
void ejecutar_WRITE(unsigned int direccion_logica, unsigned int valor_a_escribir, int tabla_paginas);
unsigned int fetch_operands(unsigned int direccion_logica, int tabla_paginas);
void ejecutar_READ(unsigned int direccion_logica, int tabla_paginas);
void recibir_valores_globales_memoria();
void entrada_tlb_destroy(void* entrada_a_destruir);
void agregar_entrada_tlb(double numero_pagina, int numero_marco);
entrada_tlb* tlb(double numero_pagina);
bool pagina_encontrada(void* entrada);
void setear_contador_rafaga();

#endif /* CPU_H_ */
