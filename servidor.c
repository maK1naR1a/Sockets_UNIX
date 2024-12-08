#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pwd.h>
#include <signal.h>

#define PUERTO 8080

void serverTCP(int s, struct sockaddr_in clientaddr_in);

int main(int argc, char* argv[]) {
    int s_TCP, ls_tcp;
    ls_tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (ls_tcp == -1) {
        perror("Error creando socket TCP");
        exit(1);
    }

    struct sockaddr_in myaddr_in;
    memset((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));

    myaddr_in.sin_family = AF_INET;
    myaddr_in.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr_in.sin_port = htons(PUERTO);

    if (bind(ls_tcp, (const struct sockaddr *)&myaddr_in, sizeof(struct sockaddr_in)) == -1) {
        perror("Error en bind");
        exit(1);
    }

    if (listen(ls_tcp, 5) == -1) {
        perror("Error en listen");
        exit(1);
    }

    setpgrp();
    if (fork() == 0) {
        close(0);
        close(2);

        struct sigaction sa;
        sa.sa_handler = SIG_IGN;
        sa.sa_flags = 0;
        sigaction(SIGCHLD, &sa, NULL);

        struct sockaddr_in clientaddr_in;
        socklen_t addrlen = sizeof(clientaddr_in);

        while (1) {
            s_TCP = accept(ls_tcp, (struct sockaddr *)&clientaddr_in, &addrlen);
            if (s_TCP == -1) continue;

            if (fork() == 0) {
                close(ls_tcp);
                serverTCP(s_TCP, clientaddr_in);
                exit(0);
            }
            close(s_TCP);
        }
    }

    return 0;
}

void serverTCP(int s, struct sockaddr_in clientaddr_in) {
    char buffer[1024];
    ssize_t len = read(s, buffer, sizeof(buffer) - 1);
    if (len <= 0) {
        close(s);
        return;
    }

    buffer[len] = '\0';  // Termina el string
    char response[4096] = {0};

    if (strlen(buffer) > 1) {  // Usuario especificado
        buffer[strcspn(buffer, "\r\n")] = '\0';  // Quita nueva línea
        struct passwd *pw = getpwnam(buffer);
        if (pw) {
            snprintf(response, sizeof(response),
                     "Nombre: %s\nUID: %d\nGID: %d\nDirectorio: %s\nShell: %s\n",
                     pw->pw_name, pw->pw_uid, pw->pw_gid, pw->pw_dir, pw->pw_shell);
        } else {
            snprintf(response, sizeof(response), "Usuario '%s' no encontrado.\n", buffer);
        }
    } else {  // Lista de todos los usuarios
        FILE *fp = fopen("/etc/passwd", "r");
        if (!fp) {
            snprintf(response, sizeof(response), "Error abriendo /etc/passwd.\n");
        } else {
            char line[256];
            while (fgets(line, sizeof(line), fp)) {
                char *user = strtok(line, ":");
                if (user) {
                    strncat(response, user, sizeof(response) - strlen(response) - 1);
                    strncat(response, "\n", sizeof(response) - strlen(response) - 1);
                }
            }
            fclose(fp);
        }
    }

    write(s, response, strlen(response));
    close(s);
}
