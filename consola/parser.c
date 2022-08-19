#include "parser.h"

void leer_y_enviar_archivo_de_instrucciones(char * pathArchivoInstrucciones, int conexion) {
  log_info(info_logger, "Leyendo el archivo %s", pathArchivoInstrucciones);
  char palabraLeida[10] = "\0";
  char c;
  char control = '0';
  Instruccion instruccionAux;
  //Abrir archivo
  FILE * file = fopen(pathArchivoInstrucciones, "r+");

  if (!file) {
    //Debería usar el logger para los errores
    log_error(error_logger, "No se pudo abrir el archivo de instrucciones solicitado");
  }
  //Cicla el archivo leyendo las instrucciones hasta llegar al EOF
  while (control != EOF) {
    c = fgetc(file);
    int contadorCaracter = 0;
    //Lee el identificador de la instrucción
    while (c != ' ') {
      palabraLeida[contadorCaracter] = c;
      c = fgetc(file);
      contadorCaracter++;
      if (strcmp(palabraLeida, "EXIT") == 0) break;
    }
    instruccionAux.params[0] = 0;
    instruccionAux.params[1] = 0;

    log_info(info_logger, "Leí correctamente la palabra: %s", palabraLeida);

    //Arma la instrucción en función del identificador leído

    if (strcmp(palabraLeida, "NO_OP") == 0) {
      instruccionAux.tipo = NO_OP;
    }

    if (strcmp(palabraLeida, "I/O") == 0) {
      instruccionAux.tipo = I_O;
    }

    if (strcmp(palabraLeida, "READ") == 0) {
      instruccionAux.tipo = READ;
    }

    if (strcmp(palabraLeida, "WRITE") == 0) {
      instruccionAux.tipo = WRITE;
    }

    if (strcmp(palabraLeida, "COPY") == 0) {
      instruccionAux.tipo = COPY;
    }

    if (strcmp(palabraLeida, "EXIT") == 0) {
      instruccionAux.tipo = EXIT;
    }

    lectura_y_asignacion_parametros( & instruccionAux, file, conexion);

    memset(palabraLeida, '\0', 10);

     enviar_instruccion(instruccionAux, conexion);

    //Consume el EOF o \n
    control = fgetc(file);

  }
  int fin_envio_instrucciones = -1;
  send(conexion, &fin_envio_instrucciones, sizeof(int), 0);

}

void lectura_y_asignacion_parametros(Instruccion * instruccionAux, FILE * file, int conexion) {
  int i = 0;
  switch (instruccionAux -> tipo) {
  case NO_OP:
	  fscanf(file, "%d", &i);
	  for(int cant_enviados = 1; cant_enviados < i; cant_enviados ++){
		  enviar_instruccion(*instruccionAux, conexion);
	  }
	  break;
  case I_O:
  case READ:
    lectura_y_asignacion_un_parametro(instruccionAux, file, i);
    break;
  case WRITE:
  case COPY:
    lectura_y_asignacion_dos_parametro(instruccionAux, file, i);
    break;
  case EXIT:
  	  break;
  }
}

void lectura_y_asignacion_un_parametro(Instruccion * instruccionAux, FILE * file, int i) {
  fscanf(file, "%d", & i);
  instruccionAux -> params[0] = i;

}

void lectura_y_asignacion_dos_parametro(Instruccion * instruccionAux, FILE * file, int i) {
  lectura_y_asignacion_un_parametro(instruccionAux, file, i);
  fgetc(file);
  fscanf(file, "%d", & i);
  instruccionAux -> params[1] = i;
}
