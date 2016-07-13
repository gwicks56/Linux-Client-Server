/*
Geordie Wicks
ID: 185828
username: gwicks

With thanks to Wikipedia and Stack Overflow, as per Usual
*/



#ifndef _SERVER_H_INCLUDED
#define _SERVER_H_INCLUDED
#define LOG "log.txt"
#define MAX_ATTEMPS 10
#define SECRET_LEN 4


typedef struct _Arguments
{
	int clientSock;                      /* socket descriptor for client */
	char code[SECRET_LEN + 1];
	char clientAddr[BUFSIZ];   /*client address*/
	char serverAddr[BUFSIZ];   /*server address*/
} Arguments;

typedef struct _Node
{
	pthread_t thread;
	struct _Node* next;
} Node;





void clearLinkedList(Node* head);




/*get the status of server*/
int isRunning();

/*increase number of successes*/
void increaseSuccess();
/*increase the number of failure*/
void increaseFailure();

/*add log message*/
void addLog(char* msg);
/* Signal Handler for SIGINT */
void ctrlCSignalHandler(int sig_num);


/* serve one client */
void *serveClient(void *args);

#endif