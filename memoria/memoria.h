#ifndef MEMORIA_H_
#define MEMORIA_H_


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <commons/log.h>
#include <commons/string.h>
#include <stdint.h>
#include <commons/collections/list.h>
#include <semaphore.h>
#include <commons/config.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include "utils.h"


t_config* memoria_config;
#endif /* MEMORIA_H_ */


//---------------------------------------------------------------
// ----------------- ENUMS Y VARIABLES GLOBALES  ----------------
//---------------------------------------------------------------

unsigned int cursor;
unsigned int pid_comparacion;
int conexion_kernel;
int conexion_cpu;
int conexion;
int tam_memoria;
int tam_pagina;
int entradas_por_tabla;
int retardo_memoria;
int marcos_por_proceso;
int retardo_swap;
char* ip_memoria;
char* algoritmo_reemplazo;
char* path_swap;
char* puerto_escucha;
void* base_memoria;
t_list* tablas_primer_nivel;
t_list* tablas_segundo_nivel;
t_list* marcos_disponibles;
t_list* listado_memoria_actual_por_proceso;
t_list* relaciones_proceso_cursor;
t_log* logger_memoria;
pthread_t *hilo_kernel_handler;
sem_t semaforo_entrada_salida;
sem_t semaforo_pid_comparacion;

typedef struct
{
	unsigned int id_segundo_nivel;
} entrada_primer_nivel;

typedef struct {
	unsigned int posicion_cursor;
	unsigned int pid_asociado;
} cursor_por_proceso;

typedef struct
{
	uint32_t marco;
	unsigned int numero_pagina;
	bool presencia;
	bool uso;
	bool modificado;
} entrada_segundo_nivel;

typedef struct{
	int tabla_segundo_nivel;
	double entrada_tabla_segundo_nivel;
	double direccion_fisica;
} datos_direccion;

typedef enum {
	INICIALIZAR_ESTRUCTURAS,
	SUSPENDER,
	DESTRUIR_ESTRUCTURAS
}accion_memoria_con_kernel;


typedef enum {
	OBTENER_TABLA_SEGUNDO_NIVEL,
	OBTENER_NUMERO_MARCO,
	LEER,
	ESCRIBIR,
	SOLICITAR_VALORES_GLOBALES
} accion_memoria_con_cpu;



//---------------------------------------------------------------
// ----------------- DECLARACION DE FUNCIONES  ------------------
//---------------------------------------------------------------

int calcular_cantidad_tablas_necesarias(unsigned int tamanio_proceso);
void inicializar_kernel_handler(pthread_t *hilo_kernel_handler);
void inicializar_listas_procesos();
void inicializar_marcos_disponibles();
void abrirArchivoConfiguracion(char* path_archivo_config);
void inicializar_cpu_handler(pthread_t *hilo_cpu_handler);
void enviar_globales();
void crear_archivo_swap(unsigned int tamanio_proceso, unsigned int pid);
void* conexion_kernel_handler(void* args);
int ejecutar_escritura(datos_direccion direccion, unsigned int valor_escritura);
unsigned int ejecutar_lectura(datos_direccion direccion);
void enviar_proceso_swap (unsigned int pid, int nro_tabla_paginas);
char* obtener_nombre_archivo_swap(unsigned int pid);
void* leer_marco_completo(uint32_t numero_marco);
void liberar_entrada_primer_nivel(void* entrada);
void liberar_entrada_segundo_nivel(void* entrada);
void destruir_estructuras(unsigned int pid, int nro_tabla_paginas);
void* buscar_pagina_en_swap(int numero_pagina, unsigned int pid);
void escribir_marco_en_memoria(uint32_t numero_marco, void* pagina_a_escribir);
void escribir_pagina_en_swap(unsigned int numero_pagina, void* contenido_pagina_reemplazada, unsigned int pid);
void inicializar_listado_memoria_actual_proceso(int tabla_primer_nivel, unsigned int pid);
void buscar_paginas_en_tabla_segundo_nivel(void* entrada_1er_nivel);
void cargar_paginas_presentes(void* entrada_2do_nivel);
bool ordenar_por_numero_marco(void * unaEntrada, void * otraEntrada);
void reemplazar_pagina(entrada_segundo_nivel* pagina_a_reemplazar, unsigned int pid, int nro_tabla_primer_nivel);
void reemplazar_pagina_clock(entrada_segundo_nivel* pagina_a_reemplazar, unsigned int pid);
entrada_segundo_nivel* clock_modificado_primer_paso();
entrada_segundo_nivel* clock_modificado_segundo_paso();
entrada_segundo_nivel* buscar_pagina_modif_sin_uso();
void reemplazar_paginas(entrada_segundo_nivel* pagina_a_swap, entrada_segundo_nivel* pagina_a_memoria, unsigned int pid);
void reemplazar_pagina_clock_modificado(entrada_segundo_nivel* pagina_a_reemplazar, unsigned int pid);
void crear_cursor_por_proceso(unsigned int pid);
void mover_cursor();
void setear_cursor_del_proceso(unsigned int pid);
void guardar_cursor_del_proceso(unsigned int pid);
void eliminar_cursor_del_proceso(unsigned int pid);
bool es_cursor_del_proceso_actual(void* rel_proceso_cursor);
void cursor_por_proceso_destroy(void* rel_proceso_cursor);
