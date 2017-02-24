/* Blake Oliveira
 * CS 367 - Project 3
 */

#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h> 
#include <errno.h> 
#include <fcntl.h> 
#include <string.h> 
#include <unistd.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>

#include "server.h"

//Defines
#define KEEPALIVE 5
#define LOGGING 1
#define MAXCLIENTS 256
#define MSGLEN 1024

//Global variables
char *MULTICASTIP;
unsigned short MULTICASTPORT;
int port;

/* @desc Creates a loopback that starts the server
 *	listening to any incoming connections
 * @param void
 * @return 0 - Shouldn't get here because a server will run
 * until interrupted
 */
int main() {

	MULTICASTIP = "239.3.6.7";
	MULTICASTPORT = 15367;

	srand(time(NULL));
	port = (rand() % (7010 - 7000)) + 7000;

	int socket = listen_on(port, MAXCLIENTS);


	dprintf(2, "Accepting connections on port %d\n", port);

	if(socket > 0)
		eventLoop(socket);

	return 0;
}

/* @desc Sends a UDP datagram to the multicast IP and receives
 * A packet back either saying OK or ERROR
 * This information is used to authenticate a user/password
 * @param client *c - The specific client we are authenticating
 * @return int: 1, client is successfully authenticated. 0 - client has wrong
 *	secret word and needs to disconnect.
 */
int authenticate(client *c) {
	int s = socket(PF_INET, SOCK_DGRAM, 0);

	char buf[MSGLEN];
	char inbuf[MSGLEN];

	int len = 0;
	len = snprintf(buf, MSGLEN, "%s%c%s%c", c->user, 0, c->secret, 0);

	memset(inbuf, 0, MSGLEN);

	struct sockaddr_in addr;
	bzero(&addr, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(MULTICASTPORT);
	inet_aton(MULTICASTIP, &addr.sin_addr);

	sendto(s, buf, len, 0, (struct sockaddr*)&addr, sizeof(addr));
	recv(s, inbuf, MSGLEN, 0);

	return (!strcmp(inbuf, "OK")) ? 1 : 0;
}

/* @desc Checks if the time passed since last communication 
 * is greater than KEEPALIVE as well as the number of out messages. 
 * If not then it closes that connection.
 * @param client *head - the pointer to our list of clients to check
 *  int *numClients - number of active clients
 * @return client *head
 */
client *checkActivity(client **head, int *numClients) {
	client *temp;
	int tNumClients = 0;
	double curTime = get_wallTime();

	if(!head) return 0;
	temp = *head;

	while(temp){

		if(curTime - temp->lastActivity > KEEPALIVE || checkNulls(temp->outMsg->text, temp->outMsg->len) > 4) {
			if(LOGGING)
				dprintf(2, "Dropping connection user: %s\n", temp->user);
			client *newTemp = temp->next;
			*head = deleteClient(head, temp);
			temp = newTemp;
		}
		else {
			tNumClients++;
			temp = temp->next;
		}
	}
	
	*numClients = tNumClients;

	if(*head)
		return *head;
	else 
		return 0;
}

/* @desc The main logic of forwarding messages. 
 *	Checks server to see if there are any incoming connections. Accepts 
 *	connections and authenticates to make sure their user and secret are valid
 *	Checks the clients if they need to be read or written
 *  Check if user/secret is set and make them if needed
 *	If they need to be read and nothing is read, close the connection
 *	Checks client if most recent activity is > timeout period
 *	Sets the read and write set
 * @param socket - the loopback of incoming messages to self
 * @return void
 */
void eventLoop(int socket) {

	fd_set rd, wr;
	int max_fd = socket, c, numClients = 0;
	client *head = 0;
	struct timeval timeout = { 5, 0 };
	FD_ZERO(&rd); 
	FD_ZERO(&wr);
	FD_SET(socket, &rd);

	while(0 <= select(max_fd + 1, &rd, &wr, NULL, &timeout)) {
		
		head = checkActivity(&head, &numClients);

		if(FD_ISSET(socket, &rd)) {
			if(numClients < MAXCLIENTS) {
				c = acceptClient(socket);
				client *cl = addClient(c);

				client *temp = head;
				cl->next = temp;
				head = cl;
			}
		}

		client *cur = head;
		while(cur){
			
			if(!cur->authenticated) {

				int credentialsWritten;
				credentialsWritten = writeCredentials(cur);

				if(credentialsWritten) {
					if(authenticate(cur))
						write(cur->fd, "OK\0\0", 4);
					else {
						write(cur->fd, "ERROR\0Failed to authenticate\0", 29);
						head = deleteClient(&head, cur);
					}
				}
			}

			else if(cur->authenticated) {
				if(FD_ISSET(cur->fd, &rd))
					head = readClient(&head, cur);

				if(FD_ISSET(cur->fd, &wr))
					head = writeClients(&head, cur);
			}
			
			cur = cur->next;
		}

		FD_ZERO(&rd);
		FD_ZERO(&wr);

		FD_SET(socket, &rd);
		FD_SET_CLIENT_READ(&head, &rd);
		FD_SET_CLIENT_WRITE(&head, &wr);

		max_fd = getMaxFd(&head, socket);

		timeout.tv_sec = KEEPALIVE;
		timeout.tv_usec = 0;
	}
}

/* @desc accepts the new client
 * @param int socket - the fd of the socket
 * @return the client's new fd or -1 on error
 */
int acceptClient(int fd) {
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	bzero(&addr, len);

	int client = accept(fd, (struct sockaddr*)&addr, &len);
	if(client == -1) {
		fprintf(stderr, "Call to accept failed because (%d) \"%s\"\n",
			errno, strerror(errno));
		return -1;
	}
	return client;
}

/* @desc writes the clients outbound message
 * @param client **head - the head incase we delete client.
 *  client *cl the current client we're writing
 * @return *head - if a deletion occurs and the head changes
 *  we return the new head
 */
client *writeClients(client **head, client *cl) {

	int numMsg = checkNulls(cl->outMsg->text, cl->outMsg->len);

	while(numMsg >= 2) {
		//This happens because we malloc when there isn't a message
		if(cl->outMsg->len == 0)
			return *head;

		char *to = cl->outMsg->text;
		int len = cl->outMsg->len;

		int wrote = 0;
		if((wrote = write(cl->fd, to, len)) <= 0)
			return *head = deleteClient(head, cl);

		int remaining = len - wrote;
		cl->outMsg = makeMessage(0, cl->outMsg->text + wrote, remaining);
		
		numMsg = checkNulls(cl->outMsg->text, cl->outMsg->len);
	}
	return *head;
}

/* @desc Writes the credentials to the client. 
*  Attempts to deal with a user message that is only half read
* @param client *cl - our malloced client we assign user/secret to
* @return 1 if successful
*  0 if unsuccessful
*/
int writeCredentials(client *cl) {
	
	int len = 0, userLen = 0;
	char msg[MSGLEN] = "", *user, *secret;

	if((len = read(cl->fd, msg, MSGLEN)) < 0) {
		if(LOGGING)
			perror("SERVER: read");
	}

	cl->outMsg = makeMessage(cl->outMsg, msg, len);
	int nulls = checkNulls(cl->outMsg->text, len);

	if (nulls >= 2) {
		userLen = strlen(cl->outMsg->text) + 1;
		user = cl->outMsg->text;
		secret = cl->outMsg->text + userLen;
		addCredentials(cl, user, secret);
		int totLen = strlen(cl->outMsg->text + userLen) + 1 + userLen;
		cl->outMsg = makeMessage(0, cl->outMsg->text + totLen, cl->outMsg->len - totLen);
		cl->authenticated = 1;
		return 1;
	}
	else
		return 0;
}

/* @desc parses the message setting the user/text appropriately
 *	makes sure that there are two nulls and it's not a half message
 *	otherwise it stores the message and doesn't pass it on
 *	If the message is two nulls then we simply update the lastActivity
 * @param client **head - the list of clients 
 *  client *cl - the client with fd we're parsing
 * @return *head - new head if it is modified
 */
client *readClient(client **head, client *cl) {

	char buf[MSGLEN] = "";
	int len, numNulls, toLen, fromLen, textLen, inLen, outLen;
	len = numNulls = toLen = fromLen = textLen = inLen = outLen = 0;

	if((len = read(cl->fd, buf, MSGLEN)) <= 0)
		return *head = deleteClient(head, cl);

	if(buf[0] == '\0' && buf[1] == '\0' && len == 2) {
		cl->lastActivity = get_wallTime();
		return *head;
	}

	cl->inMsg = makeMessage(cl->inMsg, buf, len);

	while(cl->inMsg->num >= 2) {
		cl->lastActivity = get_wallTime();

		char *to = cl->inMsg->text;
		char *from = cl->user;
		char *text = &to[strlen(to) + 1];
		int inText = strlen(to) + strlen(text) + 2;
		int fromText = strlen(from) + strlen(text) + 2;
		char newText[fromText];
		snprintf(newText, fromText, "%s%c%s%c", from, 0, text, 0);


		if(!strcmp(to, ""))
			writeAll(head, from, newText, fromText);
		else
			writeDirected(head, to, newText, fromText);

		char *remainingText = cl->inMsg->text + inText;
		cl->inMsg = makeMessage(0, remainingText, cl->inMsg->len - inText);
	}

	return *head;
}

/* @desc allocates a message and text
 * @param message *old - old message to get data from
 *  char *text - text to append
 *  int len - length of the new message string
 * @return msg - the created message
 */
message *makeMessage(message *old, char *text, int len) {
	
	message *msg = (message *)malloc(sizeof(message));
	char *oldtext = 0;
	int oldnum = 0, oldlen = 0, num = 0;

	if(old) {
		oldnum = old->num;
		oldtext = old->text;
		oldlen = old->len;
	}

	if(oldlen + len > 0) {
		msg->text = (char *)malloc((oldlen + len) * sizeof(char));
		num = oldnum + checkNulls(text, len);
	}
	else
		//Always malloc something so we don't get any issues when freeing
		msg->text = (char *)malloc(sizeof(char));


	memcpy(msg->text, oldtext, oldlen);
	memcpy(msg->text + oldlen, text, len);

	msg->len = oldlen + len;
	msg->num = num;

	if(old) {
		free(old->text);
		free(old);
	}

	return msg;
}

/* @desc Writes a message to everyone but sender
 * @param client **head - the list of clients
 *  char *from - who the message is from
 *  char *text - text to be written
 *  int len - length of the text
 * @return void
 */
void writeAll(client **head, char *from, char *text, int len) {
	client *cur = *head;
	while(cur) {
		if(cur->authenticated) {
			if(strcmp(from, cur->user))
				cur->outMsg = makeMessage(cur->outMsg, text, len);

			cur = cur->next;
		}
	}
}

/* @desc Writes a message to a specific person
 * @param client **head - the list of clients
 *  char *to - who the message is to
 *  char *text - text to be written
 *  int len - length of the text
 * @return void
 */
void writeDirected(client **head, char *to, char *text, int len) {
	client *cur = *head;
	while(cur) {
		if(cur->authenticated) {
			if(!strcmp(to, cur->user)) {
				cur->outMsg = makeMessage(cur->outMsg, text, len);
				return;
			}
		}
		cur = cur->next;
	}
}

/* @desc Get the number of nulls in a message
 * @param char *buf - text to iterate through
 *  int len - length of the text
 * @return int - the number of \0's found
 */
int checkNulls(char *buf, int len) {
	int i, numNulls = 0;

	for(i = 0; i < len; i++) {
		if(buf[i] == '\0')
			numNulls++;
	}
	return numNulls;
}

/* @desc gets the current time
 * @param void
 * @return double - current time
 */
double get_wallTime() {
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return (double)(tp.tv_sec + tp.tv_usec / 1000000.0);
}

/* @desc macro for setting the fd_set with the fd's of all the clients.
 *  also finds the numClients which may be changed from other activities
 * @param client **head - the head of the clients we are going through.
 *  fd_set rd - the read set we are adding to.
 * @return void
 */
void FD_SET_CLIENT_READ(client **head, fd_set *rd) {
	
	client *cur = *head;

	while(cur) {
		FD_SET(cur->fd, rd);

		cur = cur->next;
	}
}

/* @desc macro for setting the fd_set with the fd's of all the clients
 *  the have a message in the outbound buffer
 * @param client **head - the head of the clients we are going through.
 *  fd_set wr - the write set we are adding to. 
 * @return void
 */
void FD_SET_CLIENT_WRITE(client **head, fd_set *wr) {
	client *cur = *head;

	while(cur) {
		if(checkNulls(cur->outMsg->text, cur->outMsg->len) >= 2)
			FD_SET(cur->fd, wr);

		cur = cur->next;
	}
}

/* @desc Gets the max fd from our list of clients and compares it to the socket
 * @param client **head - the list of the clients we are going through.
 *  int socket - fd to compare to
 * @return int - max fd from our list and socket
 */
int getMaxFd(client **head, int socket) {
	int maxFd = 0;
	client *cur = *head;
	while(cur) {
		if(cur->fd > maxFd)
			maxFd = cur->fd;
		cur = cur->next;
	}

	return (maxFd > socket) ? maxFd : socket;
}

/* @desc Create a new server socket
 * @param unsigned short port - The port to listen on
 *  int queue - The maximum number of clients waiting to connect
 * @return the fd of the new socket
 */
int listen_on(unsigned short port, int queue) {

	int fd = socket(PF_INET, SOCK_STREAM, 0);
	if(fd == -1) {
		fprintf(stderr, "Call to socket failed because (%d) \"%s\"\n",
			errno, strerror(errno));
		return -1;
	}

	struct sockaddr_in addr;
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	if(bind(fd, (struct sockaddr*)&addr, sizeof(addr))) {
		fprintf(stderr, "Call to bind failed because (%d) \"%s\"\n",
			errno, strerror(errno));
		close(fd);
		return -1;
	}

	if(listen(fd, queue)) {
		fprintf(stderr, "Call to listen failed because (%d) \"%s\"\n",
			errno, strerror(errno));
		close(fd);
		return -1;
	}

	return fd;
}

/* @desc allocates a new client without a message
 * @param int fd - the fd of our new client
 * @return client *cl - our new client
 */
client *addClient(int fd) {
	client *cl = (client *)malloc(sizeof(client));
	cl->fd = fd;
	cl->next = 0;
	cl->authenticated = 0;
	cl->slowRead = get_wallTime();
	cl->lastActivity = get_wallTime();
	cl->inMsg = makeMessage(0, 0, 0);
	cl->outMsg = makeMessage(0, 0, 0);
	return cl;
}

/* @desc mallocs the credentials we are given
 * @param client *cl - the client we're assigning the credentials to
 *  char *user - the local string passed in
 *  char *secret - the local secret string passed in
 * @ return void
 */
void addCredentials(client *cl, char *user, char *secret) {
	char *pUser = malloc((strlen(user) + 1) * sizeof(char));
	char *pSecret = malloc((strlen(secret) + 1) * sizeof(char));

	strcpy(pUser, user);
	strcpy(pSecret, secret);

	cl->authenticated = 1;
	cl->user = pUser;
	cl->secret = pSecret;
}

/* @desc Delete a client from our linkedlist of clients.
 *  If there are no clients, it doesn't do anything.
 *  If there are only two clients, it sets previous and next appropriately
 * @param client *cl - the client that we want to delete
 * @return void
 */
client *deleteClient(client **head, client *cl) {

	client *cur = *head;
	client *prev = 0;
	client *newHead = *head;

	if(!cl) return newHead;
	if(!cur) return 0;

	while(cur) {
		
		if(cur->fd == cl->fd)
			break;

		prev = cur;
		cur = cur->next;
	}


	//Not head
	if(prev) {
		if(cur->next)
			prev->next = cur->next;
		else prev->next = 0;
	}
	//Head
	else {
	//There were at least 3 items
		if(newHead->next)
			newHead = newHead->next;
		else
			newHead = 0;
	}

	free(cur->inMsg->text);
	free(cur->outMsg->text);
	free(cur->inMsg);
	free(cur->outMsg);
	free(cur->user);
	free(cur->secret);
	close(cur->fd);
	memset(cur, 0, sizeof(client));
	free(cur);

	return newHead;
}