#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORT 79 // Puerto "bien conocido" para Finger

int main() {
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
    server_addr.sin_addr.s_addr = INADDR_ANY; // Escuchar en todas las interfaces
    server_addr.sin_port = htons(PORT); // Puerto 79, convertido a orden de red
    
    // Asociar el socket al puerto
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
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

    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    printf("Esperando conexiones...\n");

    // Aceptar una conexión
    if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
        perror("Error al aceptar la conexión");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Conexión aceptada desde %s:%d.\n",
           inet_ntoa(client_addr.sin_addr),
           ntohs(client_addr.sin_port));

        char *message = "Bienvenido al servidor Finger.\n";
    if (send(client_fd, message, strlen(message), 0) == -1) {
        perror("Error al enviar mensaje al cliente");
    }

    printf("Mensaje enviado al cliente.\n");

    close(client_fd); // Cerrar la conexión con el cliente
    close(server_fd); // Cerrar el socket del servidor

    return 0;
}


