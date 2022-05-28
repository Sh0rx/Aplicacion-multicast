/*
** Fichero: suscriptor.c
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
struct ipv6_mreq ipv6mreq;

// ./suscriptor ff15::33 eth0 4343
int main(int argc, char *argv[]) {

    int interfaz;

    int saltos, puerto, intervalo;
    char interfaz_str[INET6_ADDRSTRLEN];
    char grupo[INET6_ADDRSTRLEN];
    //int sUDP;
    struct sockaddr_in6 dir_suscriptor;
    //struct ipv6_mreq ipv6mreq;
    char mensaje[MAX];

    int r;
	struct sockaddr_in6 dir_difusor;
	char dir_difusor_str[INET6_ADDRSTRLEN];
    socklen_t len;
    len = sizeof(struct sockaddr_in6);

    sigset_t sigset;                // SeNal SIGINT
    struct sigaction s;
    s.sa_flags = 0;
    s.sa_handler = manejadoraSIGINT;

    bzero(&dir_suscriptor, sizeof(dir_suscriptor));
    bzero(&ipv6mreq, sizeof(ipv6mreq));
    bzero(&dir_difusor, sizeof(dir_difusor));

    //Comprobamos los parámetros
    if(argc == 1) {
        strcpy(grupo, GRUPOMULTICASTIPV6);
        strcpy(interfaz_str, INTERFAZ);
        puerto = 4343;
    } else if(argc == 4) {
        strcpy(grupo, argv[1]);
        strcpy(interfaz_str, argv[2]);
        puerto = atoi(argv[3]);
    } else {
        printf("Uso: ./suscriptor IPv6Multicast interfaz puerto\n\n");
        return 0;
    }

    ipv6mreq.ipv6mr_interface = if_nametoindex(interfaz_str);

    printf("SUSCRIPTOR MULTICAST\n");
    //fflush(stdout);
    printf("- Grupo: %s\n- Interfaz: %s\n- Puerto: %d\n", grupo, interfaz_str, puerto);
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

    // Inicializamos las estructuras con nuestra IP y puerto efímero
    dir_suscriptor.sin6_family = AF_INET6;
    dir_suscriptor.sin6_addr = in6addr_any;
    dir_suscriptor.sin6_port  = htons(puerto);

    // Convertimos la direccion multicast a binario
    if(inet_pton(AF_INET6, grupo, &ipv6mreq.ipv6mr_multiaddr) == -1) {
        perror("Error al pasar dir_multicast a binario (inet_pton)");
        exit(1);
    }

    // Bind a nuestra direccion
    if(bind(sUDP, (const struct sockaddr *)&dir_suscriptor, sizeof(dir_suscriptor)) < 0) {
        perror("Error en bind");
        exit(1);
    }

    // Nos unimos al grupo multicast
    if(setsockopt(sUDP, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &ipv6mreq, sizeof(ipv6mreq))==-1)
    {
        perror("Error al unirme al grupo multicast (setsockopt)");
        exit(1);
    }

    //Recibimos los mensajes y mostramos IP del difusor
    while(1) {

	    r = recvfrom(sUDP, mensaje, MAX - 1, 0,(struct sockaddr *)&dir_difusor, &len);
	    if(r==-1) {
            perror("Error al reibir el mensaje (recvfrom)");
            exit(1);
	    } else {
            mensaje[r]='\0';
		    inet_ntop(AF_INET6, &(dir_difusor.sin6_addr), dir_difusor_str, INET6_ADDRSTRLEN * sizeof(char));
		    printf("%s\n(Difusor: %s)\n", mensaje, dir_difusor_str);
        }
    }

    return 0;
}

// Manejadora seNal SIGINT
void manejadoraSIGINT (int s) {
    printf("\nSIGINT capturado\n");

    // Abandonamos el grupo multicast
    if(setsockopt(sUDP, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, &ipv6mreq, sizeof(ipv6mreq))==-1)
    {
        perror("Error al abandonar grupo multicast (setsockopt)");
        exit(1);
    }
    close(sUDP);    // Cerramos el socket UDP
    exit(0);
}