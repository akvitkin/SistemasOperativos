#ifndef PARSER_H_
#define PARSER_H_

#include "utils.h"


t_log* error_logger;
t_log* info_logger;

//---------------------------------------------------------------
// ----------------- DECLARACION DE FUNCIONES  ------------------
//---------------------------------------------------------------

void leer_y_enviar_archivo_de_instrucciones(char * pathArchivoInstrucciones, int conexion);
void lectura_y_asignacion_parametros(Instruccion * instruccionAux, FILE * file, int conexion);
void lectura_y_asignacion_un_parametro(Instruccion * instruccionAux, FILE * file, int i);
void lectura_y_asignacion_dos_parametro(Instruccion * instruccionAux, FILE * file, int i);

#endif /* PARSER_H_ */
