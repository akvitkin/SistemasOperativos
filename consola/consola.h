#ifndef CONSOLA_H_
#define CONSOLA_H_

#include "parser.h"
#include <commons/string.h>
#include <time.h>
#include <math.h>

char * ip;
char * puerto_kernel;
t_config * consola_config;
struct timespec tiempo_inicio;
struct timespec tiempo_fin;

#endif /* CONSOLA_H_ */
