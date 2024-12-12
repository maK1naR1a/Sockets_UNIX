#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BUFFERSIZE 1024
#define PORT 79 // Puerto del servicio Finger

void usage(const char *progname) {
    printf("Uso: %s TCP/UDP [usuario] [usuario@host]\n", progname);
    exit(EXIT_FAILURE);
}

void handle_tcp(const char *host, const char *message);
void handle_udp(const char *host, const char *message);

int main(int argc, char *argv[]) {
    if (argc < 2 || (strcmp(argv[1], "TCP") != 0 && strcmp(argv[1], "UDP") != 0)) 
    {
        usage(argv[0]);
    }

    const char *protocol = argv[1];
    char user[BUFFERSIZE];
    char host[BUFFERSIZE];
    if (argc == 2)
    {
        strncpy(user, "\r\n", sizeof(user));
        strncpy(host, "127.0.0.1", sizeof(host) - 1);
    }
    else
    {
        char *at_pos;
        char *toParse = argv[2];

        if (toParse[0] == '@') 
        {                       // Only @host
            strncpy(user, "\r\n", sizeof(user));
            strncpy(host, toParse + 1, sizeof(host) - 1);
            host[sizeof(host) - 1] = '\0'; 
        } 
        else if ((at_pos = strchr(toParse, '@'))) 
        {
                                // User@host specified
            size_t user_len = at_pos - toParse;
            if (user_len >= sizeof(user)) 
            {
                fprintf(stderr, "Error: Username too long.\n");
                exit(EXIT_FAILURE);
            }

            strncpy(user, toParse, user_len);
            user[user_len] = '\0';

            strncpy(host, at_pos + 1, sizeof(host) - 1);
            host[sizeof(host) - 1] = '\0'; 
        } 
        else 
        {
                                // Only user specified
            strncpy(user, toParse, sizeof(user) - 1);
            user[sizeof(user) - 1] = '\0'; 

            strncpy(host, "127.0.0.1", sizeof(host) - 1);
        }
    }
    if (strcmp(protocol, "TCP") == 0) {
        handle_tcp(host, user);
    } else {
        handle_udp(host, user);
    }
    return 0;
}

void handle_tcp(const char *host, const char *message) {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFERSIZE];

    // Crear el socket TCP
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error al crear el socket TCP");
        exit(EXIT_FAILURE);
    }
    printf("Socket TCP creado exitosamente\n");


    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
        perror("Dirección IP no válida");
        close(sock);
        exit(EXIT_FAILURE);
    }
    printf("SERVER ADDR FOUND\n");

    // Conectar al servidor
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error al conectar con el servidor TCP");
        close(sock);
        exit(EXIT_FAILURE);
    }
    printf("SUCCESFULLY CONNECTED TO THE SERVER\n");
    

    // Enviar el mensaje
    if (send(sock, message, strlen(message), 0) == -1) {
        perror("Error al enviar el mensaje");
        close(sock);
        exit(EXIT_FAILURE);
    }
    printf("Message sent\n");
    printf("Waiting for message\n");

    // Recibir la respuesta
    ssize_t n;
    if ((n = recv(sock, buffer, BUFFERSIZE - 1, 0)) == -1) {
        perror("Error al recibir la respuesta");
    } else {
        buffer[n] = '\0';
        printf("Respuesta del servidor: \n%s\n", buffer);
    }
    printf("Message received\n");

    close(sock);
}

void handle_udp(const char *host, const char *message) {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFERSIZE];

    // Crear el socket UDP
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
    {
        perror("Error al crear el socket UDP");
        exit(EXIT_FAILURE);
    }
    printf("Socket UDP creado exitosamente\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) 
    {
        perror("Dirección IP no válida");
        close(sock);
        exit(EXIT_FAILURE);
    }
    printf("SERVER ADDR FOUND\n");
    printf("SUCCESFULLY CONNECTED TO THE SERVER\n");

    // Enviar el mensaje
    if (sendto(sock, message, strlen(message), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) 
    {
        perror("Error al enviar el mensaje");
        close(sock);
        exit(EXIT_FAILURE);
    }
    printf("Message sent\n");
    printf("Waiting for message\n");

    // Recibir la respuesta
    socklen_t addr_len = sizeof(server_addr);
    ssize_t n;
    if ((n = recvfrom(sock, buffer, BUFFERSIZE, 0, (struct sockaddr *)&server_addr, &addr_len)) == -1) 
    {
        perror("Error al recibir la respuesta");
    } else 
    {
        buffer[n] = '\0';
        printf("Respuesta del servidor: \n%s\n", buffer);
    }
    close(sock);
}
