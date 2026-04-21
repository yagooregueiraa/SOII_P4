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

char produce_item(FILE *archivo)
{
    int c = fgetc(archivo);

    if (c == EOF)
        return '\0';
    /* comprobacion de que sea vocal */
    switch (c) {
        case 'a': case 'A': vocales_prod[0]++; break;
        case 'e': case 'E': vocales_prod[1]++; break;
        case 'i': case 'I': vocales_prod[2]++; break;
        case 'o': case 'O': vocales_prod[3]++; break;
        case 'u': case 'U': vocales_prod[4]++; break;
    }
    return (char)c;
}

void productor(FILE* fp,int T){

    char item;
    char msg_vacio;

    while(1){

        /*Recibimos el mensaje vacio del consumidor*
        * Esto se bloquea si el buffer esta lleno */
        if (mq_receive(almacen2, &msg_vacio, sizeof(char), NULL) == -1) {
            printf("[ERROR] mq_receive (vacío)");
            break;
        }

        item = produce_item(fp);
        /*Espera un valor aleatorio entre 0 y T*/
        usleep(rand()%T);

        /* Si llegamos al final del fichero, enviamos marca de fin y salimos */
        if (item == '\0') {
            mq_send(almacen1, &item, sizeof(char), 0);
            printf("[Productor] Fin de fichero, señal enviada.\n");
            break;
        }

        /*Enviamos el item al consumidor por almacen1*/
        if (mq_send(almacen1, &item, sizeof(char), 0) == -1) {
            printf("[ERROR] mq_send (item)");
            break;
        }
    }
}

int main (int argc, char **argv){

    srand(time(NULL)); /* inicializamos la semilla para los aleatorios */

    /*Si los argumentos no son exactamente 3 damos error*/
    if (argc != 3) {
        printf("Uso: %s archivo.txt T\n", argv[0]);
        return 1;
    }

    /*Cogemos los argumentos en variables locales*/
    char *archivo = argv[1];
    int T = atoi(argv[2]);
    
    struct mq_attr attr;
    attr.mq_maxmsg = MAX_BUFFER;    /* máximo número de mensajes en la cola */
    attr.mq_msgsize = sizeof(char); /* tamaño de cada mensaje (1 byte) */

    /* Borrado de los buffers de entrada por si existían de una ejecución previa */
    mq_unlink("/ALMACEN1");
    mq_unlink("/ALMACEN2");

    /* Apertura de los buffers (colas de mensajes). */
    almacen1 = mq_open("/ALMACEN1", O_CREAT | O_WRONLY, 0777, &attr);
    almacen2 = mq_open("/ALMACEN2", O_CREAT | O_RDONLY, 0777, &attr);

    if ((almacen1 == -1) || (almacen2 == -1)) {
        perror("mq_open");
        return 1;
    }

    FILE *fp = fopen(archivo,"r");
    if (fp == NULL){
        printf("[ERROR]: No se pudo abrir el archivo para su lectura");
        mq_close(almacen1);
        mq_close(almacen2);
        return 1;
    }

    printf("[Productor] Iniciado. Leyendo de '%s' con T=%d us\n", archivo, T);

    productor(fp,T);

    /* Imprimimos resultados */
    printf("[-]Productor: Vocales contadas: A:%d E:%d I:%d O:%d U:%d\n",vocales_prod[0], vocales_prod[1], vocales_prod[2],vocales_prod[3], vocales_prod[4]);

    fclose(fp);
    mq_close(almacen1);
    mq_close(almacen2);

    printf("[Productor] Programa acabado");
    return 0;
}