
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
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>  
#include <sys/resource.h>
#include <sys/time.h>

#include "server.h"  

/*
this file implements  the TCP server
that plays the role of game master
in the MasterMind game
*/

void *serveClient(void *arg);           


/*
	Helper function so i dont have to hold down control all the time whilst testing
	Blatantly stolen from Stack Overflow user iharob
	
*/
void toUpper(char str[]){
	int i;
	for (i = 0; i < strlen(str); i++)
	{
		if (str[i] <= 'z' && str[i] >= 'a')
		{
			str[i] = str[i] - 32;
		}
	}
}


/*lock variable is used in synchronization*/
pthread_mutex_t lock;

/* rusage logging*/
struct timeval u_start, u_end;
struct timeval s_start, s_end;
struct rusage usage;


/*head to linked list*/
Node* threadHead = NULL;

/*thread attribute*/
pthread_attr_t attr;


/*number of clients*/
int numClients = 0;

/*number of clients*/
int numSuccessClients = 0;

/*number of clients*/
int numFailureClients = 0;

/*clear linked list*/
void clearLinkedList(Node* head){
	Node* temp;
	Node* current = threadHead;
	while (current != NULL)
	{
		temp = current;
		current = current->next;
		free(temp);
	}
}

/*check if server is running*/
int running = 1;

/*get the status of server*/
int isRunning(){

	int result = 0;

	/*lock*/
	pthread_mutex_lock(&lock);

	result = running;

	/*unlock*/
	pthread_mutex_unlock(&lock);

	return result;
}

/*increase number of successes*/
void increaseSuccess(){

	/*lock*/
	pthread_mutex_lock(&lock);

	numSuccessClients++;

	/*unlock*/
	pthread_mutex_unlock(&lock);

}

/*increase the number of failures*/
void increaseFailure(){

	/*lock*/
	pthread_mutex_lock(&lock);

	numFailureClients++;

	/*unlock*/
	pthread_mutex_unlock(&lock);

}

/*add log message*/
void addLog(char* msg){

	FILE *pFile;

	/*lock*/
	pthread_mutex_lock(&lock);

	pFile=fopen(LOG, "a");
	fprintf(pFile, "%s", msg);
	fclose(pFile); /*close file*/

	/*unlock*/
	pthread_mutex_unlock(&lock);
}

/* Signal Handler for SIGINT */
void ctrlCSignalHandler(int sig_num)
{
	int rc;
	void* status;
	char temp[BUFSIZ];




	/* rusage logging*/
	getrusage(RUSAGE_SELF, &usage);
	u_end = usage.ru_utime;
	s_end = usage.ru_stime;
	
	/*stop running*/
	/*lock*/
	pthread_mutex_lock(&lock);

	running = 0;

	/*unlock*/
	pthread_mutex_unlock(&lock);

	/* free attribute and wait for the other threads */
	pthread_attr_destroy(&attr);

	Node* current = threadHead;
	while (current != NULL)
	{
		rc = pthread_join(current->thread, &status);

		current = current->next;
	}


	/*destroy mutex*/
	pthread_mutex_destroy(&lock);

	/*clear linked list*/
	clearLinkedList(threadHead);

	/*write statistics to file*/
	sprintf (temp, "\n\n%d client connected\n", numClients);
	addLog(temp);
	sprintf (temp, "%d client success\n", numSuccessClients);
	addLog(temp);
	sprintf (temp, "%d client failure\n\n", numFailureClients);
	addLog(temp);
	sprintf(temp, "%ld KB maximum memory available determined using ru_maxrss\n", usage.ru_maxrss);
	addLog(temp);
	sprintf(temp, "User CPU Time: %ld.4%lds\n", u_end.tv_sec, u_end.tv_usec);
	addLog(temp);
	sprintf(temp, "System CPU Time: %ld.4%lds\n\n", s_end.tv_sec, s_end.tv_usec);
	addLog(temp);
	int pid = getpid();
	sprintf(temp, "The PID of server is: %d \n\n", pid);
	addLog(temp);

	char path[40], line[50];

	/* 
	   scan the /proc/<PID>/status file for interesting information
	   and save it to the Log file.
	*/

	FILE* statusf;

	snprintf(path, 40, "/proc/%d/status", pid);
	statusf = fopen(path, "r");

	if(!statusf)
	{
		sprintf(temp, "The /proc/status file could not be accessed\n");
		addLog(temp);
		exit(0);
	}
	
	char *vmHWM;
	int count = 0;
	sprintf(temp, "The relevant /proc/<PID>/status information is: \n\n");
	addLog(temp);


	while((fgets(line, sizeof line, statusf) != NULL) && (count < 24))
	{
		vmHWM = line;
		if(count > 10)
		{
			sprintf(temp, ": %s", vmHWM);
			addLog(temp);
		}
		count++;
	}

    fclose(statusf);
	exit(0);
}

int main(int argc, char *argv[])
{
	int serverSock;                  /* socket descriptor for server */
	int clientSock;                  /* socket descriptor for client */
	struct sockaddr_in serverAddr;	 /* server address */
	struct sockaddr_in clientAddr;	 /* client address */
	unsigned short serverPort;       /* server port */
	unsigned int clientLen;          /* client address data structure length */
	char secretCode[SECRET_LEN + 1]; /* secret code*/
	pthread_t threadID;              /* thread ID from pthread_create() */
	Arguments *threadArgs;   		 /* pointer to argument structure for thread */
	char buffer[BUFSIZ];
	int i;
	int randomSecretCode;  			 /*random secret code or default*/
	char temp[BUFSIZ];

	srand(time(NULL));

	getrusage(RUSAGE_SELF, &usage);
	u_start = usage.ru_utime;
	s_start = usage.ru_stime;
	

	if (argc != 3 && argc != 2)     /* check number of arguments */
	{
		fprintf(stderr, "prompt: ./server [port number] [default secret code]\n");
		fprintf(stderr, "where [port number] is a valid port number (e.g., 6543)\n");
		fprintf(stderr, "where [default secret code] is a valid secret code (e.g., AAAA)\n");
		exit(1);
	}

	/* handle the SIGINT (Ctrl-C) signal handler */
	signal(SIGINT, ctrlCSignalHandler);

	serverPort = atoi(argv[1]);  /* server port */

	if (argc == 3)
	{
		strcpy(secretCode, argv[2]);  /* secret code */
		randomSecretCode = 0;
	}else{
		randomSecretCode = 1;
	}
	

	/* Create socket */
	if ((serverSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		fprintf(stderr, "Could not create server socket\n");
		exit(1);
	}

	/* create local address structure */
	memset(&serverAddr, 0, sizeof(serverAddr));       /* zero structure */
	serverAddr.sin_family = AF_INET;                  /* internet address family */
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);   /* any incoming interface */
	serverAddr.sin_port = htons(serverPort);          /* server port */

	/* bind to the local address */
	if (bind(serverSock, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0){
		fprintf(stderr, "Could not bind\n");
		exit(1);
	}

	/* listen for incoming connections */
	if (listen(serverSock, 0) < 0){
		fprintf(stderr, "Could not listen\n");
		exit(1);
	}

	if (pthread_mutex_init(&lock, NULL) != 0)
	{
		fprintf(stderr, "Could not initialize mutex\n");
		exit(1);
	}

	FILE *pFile;

	/*create new file or empty existing file*/
	pFile=fopen(LOG, "w");
	fclose(pFile); /*close file*/

	/* Initialize and set thread detached attribute */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	for (;;)
	{
		/* get the size of client address */
		clientLen = sizeof(clientAddr);

		/* Wait for connection */
		if ((clientSock = accept(serverSock, (struct sockaddr *) &clientAddr, 
			&clientLen)) < 0){
				fprintf(stderr, "Could not accept\n");
				exit(1);
		}
		numClients++;

		time_t timer;
		struct tm* tm_info;
		char timeBuf[BUFSIZ];
		time(&timer);
		tm_info = localtime(&timer);
		strftime(timeBuf, BUFSIZ - 1, "%m %d %Y %H:%M:%S", tm_info);

		sprintf (temp, "[%s](%s)(%d) client connected\n", timeBuf, inet_ntoa(clientAddr.sin_addr), numClients);
		addLog(temp);

		if (randomSecretCode == 1)
		{
			for (i = 0; i < SECRET_LEN; i++)
			{
				secretCode[i] = rand() % 6 + 'A';
			}
			secretCode[SECRET_LEN] = '\0';
		}
			

		/* Create separate memory for client argument */
		if ((threadArgs = (Arguments*) malloc(sizeof(Arguments)))  == NULL){
			fprintf(stderr, "Could not create arguments for thread\n");
			exit(1);
		}

		threadArgs->clientSock = clientSock;
		strcpy(threadArgs->code, secretCode);
		strcpy(threadArgs->clientAddr, inet_ntoa(clientAddr.sin_addr));
		strcpy(threadArgs->serverAddr, inet_ntoa(serverAddr.sin_addr));
		/*add node to linked list as new head*/
		Node* node = (Node*)malloc(sizeof(Node));
		node->next = threadHead;
		threadHead = node;

		/* create client thread */
		if (pthread_create(&(node->thread), &attr, serveClient, (void *) threadArgs) != 0){
			fprintf(stderr, "Could not create thread\n");
			exit(1);
		}

		time(&timer);
		tm_info = localtime(&timer);
		strftime(timeBuf, BUFSIZ - 1, "%m %d %Y %H:%M:%S", tm_info);
		sprintf (buffer, "[%s](%s) server secret = %s\n", timeBuf, 
			inet_ntoa(serverAddr.sin_addr), secretCode);
		addLog(buffer);

	}/*end for*/
}

/* serve a client */
void *serveClient(void *args)
{
	int clientSock;                   /* socket descriptor for client connection */
	void *status;
	char buffer[BUFSIZ];
	int len;
	int done = 0;
	char code[SECRET_LEN + 1];
	int numTries = 0; 				/*number of attemps*/
	int correctPositions;
	int correctCharacters;
	int i, j;
	int invalid;
	char clientAddr[BUFSIZ];   		/*client address*/ 
	char serverAddr[BUFSIZ];   		/*server address*/
	char temp[BUFSIZ];
	char buffer2[BUFSIZ];
	time_t timer;
	struct tm* tm_info;
	char timeBuf[BUFSIZ];

	/* make sure that the thread resources are deallocated */
	pthread_detach(pthread_self()); 

	/* Extract socket file descriptor from argument */
	clientSock = ((Arguments *) args) -> clientSock;
	strcpy(code, ((Arguments *) args) ->code); 
	strcpy(clientAddr, ((Arguments *) args) ->clientAddr); 
	strcpy(serverAddr, ((Arguments *) args) ->serverAddr); 

	free(args);              /* free resource */

	/* Informative welcome message  :)*/
	sprintf (buffer, "Welcome to master mind game. You have %d guesses. Please guess of the secret code", MAX_ATTEMPS);
	if (send(clientSock, buffer, strlen(buffer), 0) != strlen(buffer)){
		fprintf(stderr, "Could not send\n");
	}

	/*run until correct or out of guesses*/
	while (isRunning() == 1 && done == 0 && numTries < MAX_ATTEMPS){

		/* receive message from client */
		if ((len = recv(clientSock, buffer, BUFSIZ - 1, 0)) < 0){
			fprintf(stderr, "Could not receive\n");
		}
		buffer[len] = '\0';
		toUpper(buffer);

		time(&timer);
		tm_info = localtime(&timer);
		strftime(timeBuf, BUFSIZ - 1, "%m %d %Y %H:%M:%S", tm_info);

		sprintf (temp, "[%s](%s) (soc_id %d) client's guess = %s\n", timeBuf, clientAddr, clientSock, buffer);
		addLog(temp);

		if (strcmp(code, buffer) == 0)
		{
			/*correct code*/
			done = 1;
			
		}else{
			correctPositions = 0;
			correctCharacters = 0;		

			strcpy(buffer2, code);

			/*check invalid character*/
			invalid = 0;
			for (i = 0; invalid == 0 && i <  SECRET_LEN; i++)
			{
				if (buffer[i] < 'A' || buffer[i] > 'F') {
					invalid = 1;
				}
			}
			
			/*found number of correct positions*/
			for (i = 0; invalid == 0 && i < SECRET_LEN; i++)
			{
				
				if (buffer[i] == buffer2[i])
				{
					correctPositions++;
					buffer2[i] = '-'; /*mark as used*/
					buffer[i] = '.'; /*mark as used*/
				}
			}

			/*found number of correct characters*/
			for (i = 0; invalid == 0 && i <  SECRET_LEN; i++)
			{
				for (j = 0; j < SECRET_LEN; j++)
				{
					if (buffer[i] == buffer2[j])
					{
						buffer2[j] = '-'; /*mark as used*/
						break;
					}
				}
				if (j < SECRET_LEN)
				{
					correctCharacters++;
				}
			}
			
			if (invalid == 0){
				sprintf (buffer, "[%d:%d]", correctPositions, correctCharacters);

				time(&timer);
				tm_info = localtime(&timer);
				strftime(timeBuf, BUFSIZ - 1, "%m %d %Y %H:%M:%S", tm_info);

				sprintf (temp, "[%s](%s) server's hint = %s\n", timeBuf, serverAddr, buffer);
				addLog(temp);

			}else{
				strcpy (buffer, "INVALID");
			}
				
			if (send(clientSock, buffer, strlen(buffer), 0) != strlen(buffer)){
				fprintf(stderr, "Could not send\n");
			}
		}
		numTries++;
	}/*end if check running*/

	if (isRunning() == 1)
	{
		if (numTries < MAX_ATTEMPS)
		{
			strcpy(buffer, "SUCCESS");
			if (send(clientSock, buffer, strlen(buffer), 0) != strlen(buffer)){
				fprintf(stderr, "Could not send\n");
			}

			time(&timer);
			tm_info = localtime(&timer);
			strftime(timeBuf, BUFSIZ - 1, "%m %d %Y %H:%M:%S", tm_info);

			sprintf (temp, "[%s](%s)(soc_id %d) SUCCESS game over\n", timeBuf, clientAddr, clientSock);
			addLog(temp);

			increaseSuccess();

		}else{
			strcpy(buffer, "FAILURE");
			if (send(clientSock, buffer, strlen(buffer), 0) != strlen(buffer)){
				fprintf(stderr, "Could not send\n");
			}

			time(&timer);
			tm_info = localtime(&timer);
			strftime(timeBuf, BUFSIZ - 1, "%m %d %Y %H:%M:%S", tm_info);

			sprintf (temp, "[%s](%s)(soc_id %d) FAILURE game over\n", timeBuf, clientAddr, clientSock);
			addLog(temp);

			increaseFailure();
		}
	}

	pthread_exit(status);

	return NULL;
}