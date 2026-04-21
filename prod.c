#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <mqueue.h>
#include <string.h>

/* tamaño del buffer */
#define MAX_BUFFER 5 

/* cola de entrada de mensajes para el productor (recibe vacíos del consumidor) */
mqd_t almacen1;

/* cola de entrada de mensajes para el consumidor (recibe items del productor) */
mqd_t almacen2;

/* contadores de vocales del productor */
int vocales_prod[5] = {0, 0, 0, 0, 0};