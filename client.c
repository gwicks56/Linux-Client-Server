/*
Geordie Wicks
ID: 185828
username: gwicks

With thanks to Wikipedia and Stack Overflow, as per Usual
*/





#include <stdio.h>      
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <stdlib.h>  
#include <string.h> 
#include <unistd.h> 

#define SECRET_LEN 4



/*
the server implements  the TCP client
that plays the codebreaker role in mastermind game
*/

/*main function*/
int main(int argc, char *argv[])
{
    int sock;                      /* socket descriptor */
    struct sockaddr_in serverAddr; /* server address */
    unsigned short serverPort;     /* server port */
    char *serverIP;                /* server IP address*/
    char buffer[BUFSIZ];           /* buffer */
	int done = 0;
	int numBytes;                  /* number of bytes received or sent*/
	int correctPositions;
	int correctCharacters;

    if (argc != 3)     /* check number of arguments */
	{
		fprintf(stderr, "prompt: ./client [host name/IP address] [port number]\n");
		fprintf(stderr, "where [host name/IP address] corresponds to host of your sever\n");
		fprintf(stderr, "and [port number] is the valid port number (e.g., 6543).\n");
		exit(1);
	}

    serverIP = argv[1];             /* server address */
    serverPort = atoi(argv[2]);     /* server port */


    /* create stream socket using TCP */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		fprintf(stderr, "Could not create socket\n");
		exit(1);
	}

    /* create server address structure */
    memset(&serverAddr, 0, sizeof(serverAddr));     /* zero structure */
    serverAddr.sin_family      = AF_INET;             /* internet address family */
    serverAddr.sin_addr.s_addr = inet_addr(serverIP);   /* server IP address */
    serverAddr.sin_port        = htons(serverPort); /* server port */

    /* connection to the server */
    if (connect(sock, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0){
		fprintf(stderr, "Could not connect\n");
		exit(1);
	}

	/*receive welcome message*/
	if ((numBytes = recv(sock, buffer, BUFSIZ - 1, 0)) <= 0){
		fprintf(stderr, "Could not receive\n");
		exit(1);
	}
	buffer[numBytes] = '\0';
	printf("%s\n", buffer);

	/*while has not yet done*/
	while (done == 0)
	{
		/*read guess code*/
		printf("Please enter secret code: ");
		scanf("%s", buffer);

		/* Send the string to the server */
		if (send(sock, buffer, strlen(buffer), 0) != strlen(buffer)){
			fprintf(stderr, "Could not send\n");
			exit(1);
		}
    
		/*receive welcome message*/
		if ((numBytes = recv(sock, buffer, BUFSIZ - 1, 0)) <= 0){
			fprintf(stderr, "Could not receive\n");
			exit(1);
		}
		buffer[numBytes] = '\0';

		printf("Received: %s\n", buffer);

		/*check result*/
		if (strcmp(buffer, "SUCCESS") == 0 || strcmp(buffer, "FAILURE") == 0)
		{
			done = 1;
		}else if (strcmp(buffer, "INVALID") == 0)
		{
			printf("Invalid guess\n");
		}else{
			 sscanf (buffer,"[%d:%d]", &correctPositions, &correctCharacters);
			 printf("You guessed %d character(s) correctly and in the correct position\n", correctPositions);
			 printf("You guessed %d character(s) correctly, but in the wrong position\n", correctCharacters);
		}

		/*check if contains SUCCESS or FAILURE*/
		if(strstr(buffer, "SUCCESS") != NULL) {
			done = 1;
			printf("Congratulation!\n");
		}else if(strstr(buffer, "FAILURE") != NULL) {
			done = 1;
			printf("Sorry! You reached all attempts\n");
		}
	}

    close(sock);

    exit(0);
}