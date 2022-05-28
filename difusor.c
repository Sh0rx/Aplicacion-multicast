/*
** Fichero: difusor.c
** Autores:
** Jorge Mohedano Sanchez DNI 70943813Z
** Saul Matias Jimenez
** Usuario: i0943813
*/

#include <sys/types.h>   /* basic system data types */
#include <sys/socket.h>  /* basic socket definitions */
#include <sys/time.h>    /* timeval{} for select() */
#include <time.h>                /* timespec{} for pselect() */
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>   /* inet(3) functions */
#include <errno.h>
#include <fcntl.h>               /* for nonblocking */
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>    /* for S_xxx file mode constants */
#include <sys/uio.h>             /* for iovec{} and readv/writev */
#include <unistd.h>
#include <sys/wait.h>
#include <net/if.h>

#define INTERFAZ "eth0"
#define GRUPOMULTICASTIPV6 "ff15::33"
#define MAX 4096

void manejadoraSIGINT(int s);

int sUDP;

// ./difusor "Hola que tal" ff15::33 eth0 4343 10 15
int main(int argc, char *argv[]) {
    
    int interfaz;

    int saltos, puerto, intervalo;
    char interfaz_str[INET6_ADDRSTRLEN];
    char grupo[INET6_ADDRSTRLEN];
    //int sUDP;
    struct sockaddr_in6 dir_difusor, dir_multicast;
    char mensaje[MAX];

    sigset_t sigset;                // SeNal SIGINT
    struct sigaction s;
    s.sa_flags = 0;
    s.sa_handler = manejadoraSIGINT;

    bzero(&dir_difusor, sizeof(dir_difusor));
    bzero(&dir_multicast, sizeof(dir_multicast));

    //Comprobamos los parámetros
    if(argc == 1) {
        strcpy(mensaje, "Hola que tal");
        strcpy(grupo, "ff15::33");
        strcpy(interfaz_str, "eth0");
        puerto = 4343;
        saltos = 10;
        intervalo = 15;
    } else if(argc == 7) {
        strcpy(mensaje, argv[1]);
        strcpy(grupo, argv[2]);
        strcpy(interfaz_str, argv[3]);
        puerto = atoi(argv[4]);
        saltos = atoi(argv[5]);
        intervalo = atoi(argv[6]);
    } else {
        printf("Uso: ./difusor \"mensaje_a_difundir\" IPv6Multicast interfaz puerto saltos intervalo\n\n");
        return 0;
    }

    interfaz = if_nametoindex(interfaz_str);

    printf("DIFUSOR MULTICAST\n");
    printf("- Mensaje: %s\n- Grupo: %s\n- Interfaz: %s\n- Puerto: %d\n- Saltos: %d\n- Intervalo: %d segundos\n",
    mensaje, grupo, interfaz_str, puerto, saltos, intervalo);
    fflush(stdout);

    // Registramos la seNal para la manejadora SIGINT
    if(sigfillset(&sigset) == -1) {
        perror("Error al registrar la seNal");
        exit(1);
    }

    if(sigdelset(&sigset, SIGINT) == -1) {
        perror("Error al registrar la seNal");
        exit(1);
    }

    if(sigfillset(&s.sa_mask) == -1) {
        perror("Error al registrar la seNal");
        exit(1);
    }

    if(sigaction(SIGINT, &s, NULL) == -1) {
        perror("Error al registrar la seNal");
        exit(1);
    }

    // Creamos socket UDP
    if((sUDP = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
        perror("Error al crear el socket");
        exit(1);
    }

    // Configuramos la interfaz
    /*interfaz_index = if_nametoindex(interfaz);
    if(setsockopt(sUDP, IPPROTO_IPV6, IPV6_MULTICAST_IF, (char *)&interfaz, sizeof(interfaz))==-1) {
        perror("Error al configurar interfaz");
        exit(1);
    }

    // Configuramos el numero de saltos
    if(setsockopt(sUDP, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &saltos, sizeof(saltos))==-1) {
        perror("Error al configurar el TTL");
        exit(1);
    }*/

    // Inicializamos las estructuras con nuestra IP y puerto efímero
    dir_difusor.sin6_family = AF_INET6;
    dir_difusor.sin6_addr = in6addr_any;
    dir_difusor.sin6_port = 0;

    dir_multicast.sin6_family = AF_INET6;
    dir_multicast.sin6_port = htons(puerto);

    // Pasamos la direccion multicast a binario
    if(inet_pton(AF_INET6, grupo, &dir_multicast.sin6_addr) == -1) {
        perror("Error al pasar dir_multicast a binario (inet_pton)");
        exit(1);
    }

    // Bind a nuestra direccion
    if(bind(sUDP, (const struct sockaddr *)&dir_difusor, sizeof(dir_difusor)) < 0) {
        perror("Error en bind");
        exit(1);
    }

    // Configuramos la interfaz
    //interfaz_index = if_nametoindex(interfaz);
    if(setsockopt(sUDP, IPPROTO_IPV6, IPV6_MULTICAST_IF, (char *)&interfaz, sizeof(interfaz))==-1) {
        perror("Error al configurar interfaz");
        exit(1);
    }

    // Configuramos el numero de saltos
    if(setsockopt(sUDP, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &saltos, sizeof(saltos))==-1) {
        perror("Error al configurar el TTL");
        exit(1);
    }

    // Enviamos el mensaje
    while(1) {
        sendto(sUDP, mensaje, strlen(mensaje), 0, (struct sockaddr *)&dir_multicast, sizeof(dir_multicast));
        sleep(intervalo);
    }

    return 0;
}

// Manejadora seNal SIGINT
void manejadoraSIGINT (int s) {
    close(sUDP);    // Cerramos el socket UDP
    printf("\nSIGINT capturado\n");
    exit(0);
}