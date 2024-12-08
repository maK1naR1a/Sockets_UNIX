#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORT 79 // Puerto del servidor

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <tcp|udp> <mensaje> <direccion_servidor|null>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int sock_fd;
    struct sockaddr_in server_addr;
    char buffer[1024] = {0};
    strncpy(buffer, argv[2], sizeof(buffer) - 1);

    // Determinar la dirección del servidor
    const char *server_ip = strcmp(argv[3], "null") == 0 ? "127.0.0.1" : argv[3];

    // Crear el socket según el tipo especificado
    if (strcmp(argv[1], "tcp") == 0) {
        if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("Error al crear el socket TCP");
            exit(EXIT_FAILURE);
        }
    } else if (strcmp(argv[1], "udp") == 0) {
        if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
            perror("Error al crear el socket UDP");
            exit(EXIT_FAILURE);
        }
    } else {
        fprintf(stderr, "Error: Protocolo no soportado. Use 'tcp' o 'udp'.\n");
        exit(EXIT_FAILURE);
    }

    // Configurar la dirección del servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Error en inet_pton");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "tcp") == 0) {
        // Conectar al servidor TCP
        if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            perror("Error al conectar con el servidor TCP");
            close(sock_fd);
            exit(EXIT_FAILURE);
        }

        // Enviar mensaje al servidor
        if (send(sock_fd, buffer, strlen(buffer), 0) == -1) {
            perror("Error al enviar datos TCP");
            close(sock_fd);
            exit(EXIT_FAILURE);
        }

        // Recibir respuesta del servidor
        int bytes_received = recv(sock_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
        } else {
            perror("Error al recibir datos TCP");
        }
    } else if (strcmp(argv[1], "udp") == 0) {
        // Enviar mensaje al servidor UDP
        if (sendto(sock_fd, buffer, strlen(buffer), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            perror("Error al enviar datos UDP");
            close(sock_fd);
            exit(EXIT_FAILURE);
        }

        // Recibir respuesta del servidor
        socklen_t addr_len = sizeof(server_addr);
        int bytes_received = recvfrom(sock_fd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&server_addr, &addr_len);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
        } else {
            perror("Error al recibir datos UDP");
        }
    }

    // Cerrar el socket
    close(sock_fd);

    return 0;
}
