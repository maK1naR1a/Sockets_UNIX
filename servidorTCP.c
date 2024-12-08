#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <signal.h>  // Asegúrate de incluir esta biblioteca


#define PORT 79 // Puerto "bien conocido" para Finger
#define LOG_FILE "peticiones.log"

// TODO: Create pararell socket for UDP, make the conections , create serverUDP();
// TODO: Makefile
// TODO: Lanzaservidor.sh (try on nogal)

// Prototipo de la función serverTCP
void serverTCP(int client_fd, struct sockaddr_in client_addr);
void handle_finger_request(char* buffer, char* response);



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

            sa.sa_handler = SIG;
            sa.sa_flags = 0;
            if (sigaction(SIGCHLD, &sa, NULL) == -1)
            {
                perror("sigaction(SIGCHLD)");
                fprintf(stderr, "%s: unable to register SIGCHLD signal \n", argv[0]);
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
    close(server_fd);
    return 0;
}

void serverTCP(int client_fd, struct sockaddr_in client_addr)
{
    char buffer[1024];
    char response[1024] = {0};

    int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) 
    {
        buffer[bytes_received] = '\0'; // Terminar la cadena recibida
        handle_finger_request(buffer, response);
        send(client_fd, response, strlen(response), 0);

        // Registrar la conexión
        log_event(inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port),
                    "TCP", buffer, response);
    }
    close(client_fd);
}

// TODO: serverUDP(...)

// Función para registrar eventos en el archivo de log
void log_event(const char *client_ip, int client_port, const char *protocol,
               const char *command, const char *response) {
    FILE *log_file = fopen(LOG_FILE, "a");
    if (!log_file) {
        perror("Error al abrir el archivo de log");
        return;
    }

    // Obtener la hora actual
    time_t now = time(NULL);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));

    // Registrar la información
    fprintf(log_file, "[%s] Conexión desde %s:%d usando %s\n", time_str, client_ip, client_port, protocol);
    fprintf(log_file, "Comando recibido: %s\n", command);
    fprintf(log_file, "Respuesta enviada: %s\n", response);
    fclose(log_file);
}

void handle_finger_request(char* buffer, char* response)
{
    // TODO: From buffer string execute command and parse output to string response
}
