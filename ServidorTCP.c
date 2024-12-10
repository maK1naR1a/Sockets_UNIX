#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>
#include <signal.h>  // Asegúrate de incluir esta biblioteca
#include <errno.h>


#define ADDRNOTFOUND	0xffffffff	/* return address for unfound host */
#define BUFFERSIZE	1024	/* maximum size of packets to be received */
#define TAM_BUFFER 10
#define MAXHOST 128
#define PORT 79 // Puerto "bien conocido" para Finger
#define LOG_FILE "peticiones.log"

// TODO: Create pararell socket for UDP, make the conections , create serverUDP();
// TODO: Make signals for terminating
// TODO: Makefile
// TODO: Lanzaservidor.sh (try on nogal)

/*
 *			M A I N
 *
 *	This routine starts the server.  It forks, leaving the child
 *	to do all the work, so it does not have to be run in the
 *	background.  It sets up the sockets.  It
 *	will loop forever, until killed by a signal.
 *
 */
void serverTCP(int client_fd, struct sockaddr_in client_addr);
void serverUDP(int s, char * buffer, struct sockaddr_in clientaddr_in);

void log_event(const char *client_ip, int client_port, const char *protocol,
               const char *command, const char *response);
void handle_finger_request(char* buffer, char* response);

void errout(char *hostname);
    
int FIN = 0;
void terminate(){ FIN = 1; };

int main(int argc, char* argv[]) 
{
    int passiveSocketTCP_fd;
    int socketTCP_fd; 
    int socketUDP_fd;

    int nReadBytes;

    struct sigaction sa = {.sa_handler = SIG_IGN}; //Ignore SIGCHLD
    struct sigaction term; // Will handle SIGTERM

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    int addrlen;

    fd_set readmask;
    int nFDs_ready, hls;  // Number of File Descriptors ready, Highest Listening Socket 

    char buffer[BUFFERSIZE];	/* buffer for packets to be read into */


    // Crear el socket TCP
    if ((passiveSocketTCP_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error al crear el socket TCP");
        exit(EXIT_FAILURE);
    }
    printf("Socket TCP creado exitosamente.\n");

    // Crear socket UDP
    if ((socketUDP_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Error al crear el socket UDP");
        exit(EXIT_FAILURE);
    }
    printf("Socket UDP creado exitosamente.\n");


	/* clear out address structures */
	memset ((char *)&server_addr, 0, sizeof(struct sockaddr_in));
   	memset ((char *)&client_addr, 0, sizeof(struct sockaddr_in));

    addrlen = sizeof(struct sockaddr_in);

    // Configurar la dirección del servidor
    server_addr.sin_family = AF_INET; // IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY; // Escuchar en todas las interfaces
    server_addr.sin_port = htons(PORT); // Puerto 79, convertido a orden de red
    

    // Asociar los sockets al servidor
    if (bind(passiveSocketTCP_fd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error al asociar el socket");
        close(passiveSocketTCP_fd);
        exit(EXIT_FAILURE);
    }
    printf("Socket TCP asociado al servidor %d.\n", PORT);

    if (bind(socketUDP_fd, (const struct sockaddr *)&server_addr, sizeof(sockaddr_in)) == -1) {
        perror("Error al asociar el socket UDP");
        close(socketUDP_fd);
        exit(EXIT_FAILURE);
    }
    printf("Socket UDP asociado al servidor %d.\n", PORT);


    // Configurar el socket TCP para escuchar conexiones
    if (listen(passiveSocketTCP_fd, 5) == -1) {
        perror("Error al escuchar en el socket");
        close(passiveSocketTCP_fd);
        exit(EXIT_FAILURE);
    }
    printf("Cola de escucha creada por el puerto %d.\n", PORT);
    
    
    // Hacer el proceso demonio
    setpgrp();

    switch(fork())
    {
        case -1:  
        // Unable to fork
            perror("Unable to fork daemon");
            exit(EXIT_FAILURE);

        case 0 : 
            close(0);
            close(2);

            if (sigaction(SIGCHLD, &sa, NULL) == -1)
            {
                perror("sigaction(SIGCHLD)");
                exit(EXIT_FAILURE);
            }
            // Registramos SIGTERM
            term.sa_handler = (void *) terminate;
            term.sa_flags = 0;
            if (sigaction(SIGTERM, &term, (struct sigaction *) 0) == -1)
            {
                perror("sigaction(SIGTERM)");
                exit(EXIT_FAILURE);
            }


            while(!FIN)
            {
                // Metemos en el conjunto los socket
                FD_ZERO(&readmask);
                FD_SET(passiveSocketTCP_fd, &readmask);
                FD_SET(socketUDP_fd, &readmask);

                // Choose the hls
                if (passiveSocketTCP_fd > socketUDP_fd) hls = socketTCP_fd;
                else hls = socketUDP_fd;

                nFDs_ready = select(hls+1, &readmask, (fd_set *)0, (fd_set *)0, NULL);
                if (nFDs_ready < 0) 
                {
                    if (errno == EINTR) 
                    {
                        FIN=1;
                        close (passiveSocketTCP_fd);
                        close (socketUDP_fd);
                        perror("\nFinalizando el servidor. Senal recibida en select\n "); 
                    }
                }
                else
                {
                    // Comprobamos que tipo de socket es (TCP/UDP)
                    if (FD_ISSET(passiveSocketTCP_fd, &readmask)) // Si es TCP...
                    {
                        socketTCP_fd = accept(passiveSocketTCP_fd, (struct sockaddr *) &client_addr, &addrlen);
                        if (socketTCP_fd == -1) exit(EXIT_FAILURE);

                        switch(fork())
                        {
                            case -1:
                                exit(EXIT_FAILURE);
                            case 0:
                                close(passiveSocketTCP_fd);
                                serverTCP(socketTCP_fd, client_addr);
                                exit(EXIT_SUCCESS);
                            default: 
                                close(socketTCP_fd); // To not run out of FD space
                        }
                    }
                    if (FD_ISSET(socketUDP_fd, &readmask)) // Si es UDP...
                    {
                        nReadBytes = recvfrom(socketUDP_fd, buffer, BUFFERSIZE - 1, 0,
                                            (struct sockaddr *)&client_addr, &addrlen);
                        if (nReadBytes == -1)
                        {
                            perror("Error reading bytes");
                            exit(EXIT_FAILURE);
                        }
                        buffer[nReadBytes] = '\0';
                        serverUDP (socketUDP_fd, buffer, client_addr);
                    }
                }
            } // Fin bucle infinito atencion clientes
            close(passiveSocketTCP_fd); // TODO : Es necesario????, lo es el de abajo o ambos
            close(socketTCP_fd);
            close(socketUDP_fd);

            printf("\nSERVIDOR FINALIZADO!\n");

    	default:
            exit(EXIT_SUCCESS);
    }
}

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
    if (fclose(log_file) != 0) {
        perror("Error al cerrar el archivo de log");
        exit(EXIT_FAILURE);
    }
}

/*
 *				S E R V E R T C P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */
void serverTCP(int s, struct sockaddr_in clientaddr_in)
{
	int reqcnt = 0;		/* keeps count of number of requests */
	char buf[TAM_BUFFER];		/* This example uses TAM_BUFFER byte messages. */
	char hostname[MAXHOST];		/* remote host's name string */

	int len, len1, status;
    struct hostent *hp;		/* pointer to host info for remote host */
    long timevar;			/* contains time returned by time() */
    
    struct linger linger;		/* allow a lingering, graceful close; */
    				            /* used when setting SO_LINGER */
    				
	/* Look up the host information for the remote host
	 * that we have connected with.  Its internet address
	 * was returned by the accept call, in the main
	 * daemon loop above.
	 */
	 
     status = getnameinfo((struct sockaddr *)&clientaddr_in,sizeof(clientaddr_in),
                           hostname,MAXHOST,NULL,0,0);
     if(status){
           	/* The information is unavailable for the remote
			 * host.  Just format its internet address to be
			 * printed out in the logging information.  The
			 * address will be shown in "internet dot format".
			 */
			 /* inet_ntop para interoperatividad con IPv6 */
            if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
            	perror(" inet_ntop \n");
             }
    /* Log a startup message. */
    time (&timevar);
		/* The port number must be converted first to host byte
		 * order before printing.  On most hosts, this is not
		 * necessary, but the ntohs() call is included here so
		 * that this program could easily be ported to a host
		 * that does require it.
		 */
	printf("Startup from %s port %u at %s",
		hostname, ntohs(clientaddr_in.sin_port), (char *) ctime(&timevar));

		/* Set the socket for a lingering, graceful close.
		 * This will cause a final close of this socket to wait until all of the
		 * data sent on it has been received by the remote host.
		 */
	linger.l_onoff  =1;
	linger.l_linger =1;
	if (setsockopt(s, SOL_SOCKET, SO_LINGER, &linger,
					sizeof(linger)) == -1) {
		errout(hostname);
	}

		/* Go into a loop, receiving requests from the remote
		 * client.  After the client has sent the last request,
		 * it will do a shutdown for sending, which will cause
		 * an end-of-file condition to appear on this end of the
		 * connection.  After all of the client's requests have
		 * been received, the next recv call will return zero
		 * bytes, signalling an end-of-file condition.  This is
		 * how the server will know that no more requests will
		 * follow, and the loop will be exited.
		 */
	while (len = recv(s, buf, TAM_BUFFER, 0)) {
		if (len == -1) errout(hostname); /* error from recv */
			/* The reason this while loop exists is that there
			 * is a remote possibility of the above recv returning
			 * less than TAM_BUFFER bytes.  This is because a recv returns
			 * as soon as there is some data, and will not wait for
			 * all of the requested data to arrive.  Since TAM_BUFFER bytes
			 * is relatively small compared to the allowed TCP
			 * packet sizes, a partial receive is unlikely.  If
			 * this example had used 2048 bytes requests instead,
			 * a partial receive would be far more likely.
			 * This loop will keep receiving until all TAM_BUFFER bytes
			 * have been received, thus guaranteeing that the
			 * next recv at the top of the loop will start at
			 * the begining of the next request.
			 */
		while (len < TAM_BUFFER) {
			len1 = recv(s, &buf[len], TAM_BUFFER-len, 0);
			if (len1 == -1) errout(hostname);
			len += len1;
		}
			/* Increment the request count. */
		reqcnt++;
			/* This sleep simulates the processing of the
			 * request that a real server might do.
			 */
		sleep(1);
			/* Send a response back to the client. */
		if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER) errout(hostname);
	}

		/* The loop has terminated, because there are no
		 * more requests to be serviced.  As mentioned above,
		 * this close will block until all of the sent replies
		 * have been received by the remote host.  The reason
		 * for lingering on the close is so that the server will
		 * have a better idea of when the remote has picked up
		 * all of the data.  This will allow the start and finish
		 * times printed in the log file to reflect more accurately
		 * the length of time this connection was used.
		 */
	close(s);

		/* Log a finishing message. */
	time (&timevar);
		/* The port number must be converted first to host byte
		 * order before printing.  On most hosts, this is not
		 * necessary, but the ntohs() call is included here so
		 * that this program could easily be ported to a host
		 * that does require it.
		 */
	printf("Completed %s port %u, %d requests, at %s\n",
		hostname, ntohs(clientaddr_in.sin_port), reqcnt, (char *) ctime(&timevar));
}

/*
 *	This routine aborts the child process attending the client.
 */
void errout(char *hostname)
{
	printf("Connection with %s aborted on error\n", hostname);
	exit(1);     
}

/*
 *				S E R V E R U D P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */
void serverUDP(int s, char * buffer, struct sockaddr_in clientaddr_in)
{
    struct in_addr reqaddr;	/* for requested host's address */
    struct hostent *hp;		/* pointer to host info for requested host */
    int nc, errcode;

    struct addrinfo hints, *res;

	int addrlen;
    
   	addrlen = sizeof(struct sockaddr_in);

      memset (&hints, 0, sizeof (hints));
      hints.ai_family = AF_INET;
		/* Treat the message as a string containing a hostname. */
	    /* Esta funci�n es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta. */
    errcode = getaddrinfo (buffer, NULL, &hints, &res); 
    if (errcode != 0){
		/* Name was not found.  Return a
		 * special value signifying the error. */
		reqaddr.s_addr = ADDRNOTFOUND;
      }
    else {
		/* Copy address of host into the return buffer. */
		reqaddr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
	}
     freeaddrinfo(res);
// TODO : Aqui va la llamada/utilidad Finger, con el sendto se deberia enviar la respuesta del finger
	nc = sendto (s, &reqaddr, sizeof(struct in_addr),
			0, (struct sockaddr *)&clientaddr_in, addrlen);
	if ( nc == -1) {
         perror("serverUDP");
         printf("%s: sendto error\n", "serverUDP");
         return;
         }   
 }

void handle_finger_request(char* buffer, char* response)
{
    // TODO: From buffer string execute command and parse output to string response
}


void terminar();