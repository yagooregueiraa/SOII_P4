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
int vocales_cons[5] = {0, 0, 0, 0, 0};

void consume_item(char c, FILE *archivo) {
    /* comprobacion de que sea vocal */
    switch (c) {
        case 'a': case 'A': vocales_cons[0]++; break;
        case 'e': case 'E': vocales_cons[1]++; break;
        case 'i': case 'I': vocales_cons[2]++; break;
        case 'o': case 'O': vocales_cons[3]++; break;
        case 'u': case 'U': vocales_cons[4]++; break;
    }
    fputc(c, archivo); // almacena el item consumido
}

void consumidor(FILE *archivo, int T){
    char item;
    char vacio = '0'; // caracter que representa un hueco vacio en el buffer
    srand(time(NULL) ^ getpid());

    for (int i=0; i < MAX_BUFFER; i++) {
        if (mq_send(almacen2, &vacio, sizeof(char), 0) == -1) {
            perror("mq_send inicial");
            return;
        }
    }

    while(1) {
        if (mq_receive(almacen1, &item, sizeof(char), NULL) == -1) {
            perror("mq_receive item");
            break;
        }

        if (item == '\0') {
            printf("[Consumidor] Señal de fin recibida.\n");
            break;
        }

        consume_item(item, archivo);

        usleep(rand() % (T + 1));

        if (mq_send(almacen2, &vacio, sizeof(char), 0) == -1) {
            perror("mq_send vacio");
            break;
        }
    }

    printf("\n[Consumidor] Escritura finalizada. Vocales: A:%d E:%d I:%d O:%d U:%d\n",
           vocales_cons[0], vocales_cons[1], vocales_cons[2], vocales_cons[3], vocales_cons[4]);
}

int main (int argc, char **argv){

    if (argc !=3) {
        fprintf(stderr, "Uso: %s <archivo_entrada> T\n", argv[0]);
        return 1;
    }

    FILE *archivo_entrada = fopen(argv[1], "w");
    if (archivo_entrada == NULL) {
        perror("Error al abrir el archivo de entrada");
        return 1;
    }

    int T = atoi(argv[2]);
    
    
    struct mq_attr attr;
    attr.mq_maxmsg = MAX_BUFFER;    /* máximo número de mensajes en la cola */
    attr.mq_msgsize = sizeof(char); /* tamaño de cada mensaje (1 byte) */


    /* Apertura de los buffers (colas de mensajes). */
    almacen1 = mq_open("/ALMACEN1", O_CREAT | O_RDONLY, 0777, &attr);
    almacen2 = mq_open("/ALMACEN2", O_CREAT | O_WRONLY, 0777, &attr);

    if ((almacen1 == -1) || (almacen2 == -1)) {
        perror("mq_open");
        return 1;
    }

    printf("[Consumidor] Iniciado. Escribiendo en '%s' con T=%d us\n", argv[1], T);

    consumidor(archivo_entrada, T);

    fclose(archivo_entrada);
    mq_close(almacen1);
    mq_close(almacen2);

    
    /* Eliminamos las colas del sistema (limpieza).
    * Lo hace el consumidor porque es el último en terminar. */
    mq_unlink("/ALMACEN1");
    mq_unlink("/ALMACEN2");

    printf("[Consumidor] Programa acabado");
    return 0;
}