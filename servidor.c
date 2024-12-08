#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

#define PUERTO 8080

void serverTCP(int s, struct sockaddr_in clientaddr_in);

int main(int argc, char* argv[]) {
    // Creación del socket
    int s_TCP, ls_tcp;  // Descriptores de conexión del socket
    ls_tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (ls_tcp == -1) {
        perror(argv[0]);
        fprintf(stderr, "%s: unable to create socket TCP\n", argv[0]);
        exit(1);
    }

    // Bind del socket
    struct sockaddr_in myaddr_in;
    memset((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));

    myaddr_in.sin_family = AF_INET;
    myaddr_in.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr_in.sin_port = htons(PUERTO);

    if (bind(ls_tcp, (const struct sockaddr *)&myaddr_in, sizeof(struct sockaddr_in)) == -1) {
        perror(argv[0]);
        fprintf(stderr, "%s: unable to bind address TCP\n", argv[0]);
        exit(1);
    }

    // Crear cola de escucha
    if (listen(ls_tcp, 5) == -1) {
        perror(argv[0]);
        fprintf(stderr, "%s: unable to listen on socket TCP\n", argv[0]);
        exit(1);
    }
    // Socket ready to listen

    // Make the proccess a Daemon
    setpgrp();
    switch (fork()) {
        case -1:
            perror(argv[0]);
            fprintf(stderr, "%s: unable to fork Daemon\n", argv[0]);
            exit(1);

        case 0:
            close(0);  // stdin
            close(2);  // stderr

            struct sigaction sa, vec;
            sa.sa_handler = SIG_IGN;
            sa.sa_flags = 0;

            if (sigaction(SIGCHLD, &sa, NULL) == -1) {
                perror("sigaction(SIGCHLD)");
                fprintf(stderr, "%s: unable to register SIGCHLD signal\n", argv[0]);
                exit(1);
            }

            vec.sa_handler = (void *)exit;
            vec.sa_flags = 0;

            if (sigaction(SIGTERM, &vec, NULL) == -1) {
                perror("sigaction(SIGTERM)");
                fprintf(stderr, "%s: unable to register SIGTERM signal\n", argv[0]);
                exit(1);
            }

            struct sockaddr_in clientaddr_in;
            socklen_t addrlen = sizeof(clientaddr_in);

            while (1) {
                s_TCP = accept(ls_tcp, (struct sockaddr *)&clientaddr_in, &addrlen);
                if (s_TCP == -1) exit(1);

                switch (fork()) {
                    case -1:
                        exit(1);

                    case 0:
                        close(ls_tcp);
                        serverTCP(s_TCP, clientaddr_in);
                        exit(0);

                    default:
                        close(s_TCP);
                }
            }
    }

    return 0;
}

void serverTCP(int s, struct sockaddr_in clientaddr_in) {
    // Ejemplo de funcionalidad del servidor
    char buffer[256] = "Mensaje del servidor\n";
    write(s, buffer, strlen(buffer));
    close(s);
}
