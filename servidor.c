#include "stdio.h"
#include <sys/types.h>
#include <sys/socket.h>

#define PUERTO 81023

int main (int argc, char* argv)
{
    // Creacion del socket
    int s_TCP;                        // Descriptor de conexion del socket
    ls_tcp = socket (AF_INET, SOCK_STREAM, 0);
    if (ls_tcp == -1)
    {
        perror(argv[0]);
        fprintf(stderr, "%s: unable to create socket TCP\n", argv[0]);
        exit(1);
    }
    // Socket created

    // Bind del socket
    struct sockaddr_in myaddr_in;
    memset ((char *)&myaddr_in, 0 , sizeof(struct sockaddr_in));

    myaddr_in.sin_family = AF_INET;
    myaddr_in.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0 - Cualquiera de mis ip's
    myaddr_in.sin_port = htons(PUERTO);

    if (bind(ls_TCP, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1)
    {
        perror(argv[0]);
        fprintf(stderr, "%s: unable to bind address TCP\n", argv[0]);
        exit(1);
    }
    // Socket binded

    // Create listen queue
    if (listen(ls_tcp, 5) == -1)
    {
        perror(argv[0]);
        fprintf(stderr, "%s: unable to listen on socket TCP\n", argv[0]);
        exit(1);
    }
    // Socket ready to listen

    // Make the proccess a Daemon
    setpgrp();
    switch(fork())
    {
        case -1:  
        // Unable to fork
            perror(argv[0]);
            fprintf(stderr, "%s: unable to fork Daemon \n", argv[0]);
            exit(1);

        case 0 : 
        // Child
            close(stdin);
            close(stderr);
            if (sigaction(SIGCHILD, &sa, NULL) == -1)
            {
                perror("sigaction(SIGCHILD)");
                fprintf(stderr, "%s: unable to register SIGCHILD signal \n", argv[0]);
                exit(1);
            }

            //Use to unbind terminal death with the program
            vec.sa_handler = (void *)finalizar;
            vec.sa_flags = 0;
            if (sigaction(SIGTERM, &vec, (struct sigaction *) 0) == -1)
            {
                perror("sigaction(SIGTERM)");
                fprintf(stderr, "%s: unable to register SIGTERM signal \n", argv[0]);
                exit(1);
            }
            while(!FIN)
            {
                s_TCP = accept(ls_tcp, (struct sockaddr *)&clientaddr_in, &addrlen); // Handler de socket ya conectado a un cliente especifico
                if (s_TCP == -1) exit(1);   
                switch (fork())
                {
                case -1:
                // Can't fork
                    exit(1);

                case 0:
                // Niece created for each connection
                    close(ls_tcp); //Close the listen socket inherited from the daemon
                    serverTCP(s_TCP, clientaddr_in);
                    exit(0);
                
                default:
                // Daemon
                    close(s_tcp); // The daemon closes the new accept socket when 
                                  // a new child is created. Prevents of running out of FD space.
                }
            }
    }// Proccess made a Daemon and running serverTCP
 }

 void serverTCP(int s, struct sockaddr_in clientaddr_in)
 {
    // TODO: Aqui se configura lo que hara el servidor  
 }