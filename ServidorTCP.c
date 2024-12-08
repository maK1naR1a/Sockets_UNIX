#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORT 79 // Puerto "bien conocido" para Finger

int main(int argc, char* argv[]) {
    int FIN = 0;
    int server_fd;
    struct sockaddr_in server_addr;

    // Crear el socket TCP
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    printf("Socket TCP creado exitosamente.\n");

    // Configurar la dirección del servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET; // IPv4
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Escuchar en todas las interfaces
    server_addr.sin_port = htons(PORT); // Puerto 79, convertido a orden de red
    
    // Asociar el socket al puerto
    if (bind(server_fd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error al asociar el socket");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Socket asociado al puerto %d.\n", PORT);

    // Configurar el socket para escuchar conexiones
    if (listen(server_fd, 5) == -1) {
        perror("Error al escuchar en el socket");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Servidor escuchando en el puerto %d.\n", PORT);
    
    // Hacerlo demonio
    setpgrp();
    switch(fork())
    {
        case -1:  
        // Unable to fork
            perror(argv[0]);
            fprintf(stderr, "%s: unable to fork Daemon \n", argv[0]);
            exit(EXIT_FAILURE);

        case 0 : 
            close(0);
            close(2);

            int client_fd;
            struct sigaction sa;

            sa.sa_handler = NULL;
            sa.sa_flags = 0;
            if (sigaction(SIGCHILD, &sa, NULL) == -1)
            {
                perror("sigaction(SIGCHILD)");
                fprintf(stderr, "%s: unable to register SIGCHILD signal \n", argv[0]);
                exit(1);
            }

            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);

            printf("Esperando conexiones...\n");

            while(!FIN)
            {
                // Aceptar una conexión
                if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len)) == -1)
                {
                    perror("Error al aceptar la conexión");
                    close(server_fd);
                    exit(EXIT_FAILURE);
                }
                switch (fork())
                {
                    case -1:
                    // Can't fork
                    perror("Error al crear nieto");
                    close(server_fd);
                    exit(EXIT_FAILURE);

                    case 0:
                    // Niece created for each connection
                        close(server_fd); //Close the listen socket inherited from the daemon
                        serverTCP(client_fd, client_addr);
                        exit(0);
                    
                    default:
                    // Daemon
                        close(client_fd); // The daemon closes the new accept socket when 
                                          // a new child is created. Prevents of running out of FD space.
                }
            }
    return 0;
}


