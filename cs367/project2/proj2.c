/* Blake Oliveira
 * CS 367 - Project 2
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

void receiveMessage(int fd);
void sendMessage(int fd);
void authenticate(int fd, char *userid, char *secret);
void keepAlive(int fd);
void eventLoop(int fd);
int connect_to(char* address, unsigned short port);
double get_wallTime();

//Global time to keep checking
double time = 0;

int main() {

	char *userid = "oliveib";
	char *secret = "cinquefoil";
	char *ip = "140.160.136.195";
	unsigned short port = 14367;

	//Initial connection to server given by startup references
	int fd = connect_to(ip, port);
	dprintf(1, "Connected to %s:%d\n", ip, port);
	
	authenticate(fd, userid, secret);
	eventLoop(fd);

	return 0;
}

//Reads from socket
void receiveMessage(int fd) {

	char buf[1024];
	//Read socket
	read(fd, buf, 1024);

	char *ack = &buf[0];
	char *msg = ack;

	//Gets past the ack/sender to the actual msg
	while(*ack != '\0') {
		msg = ack;
		for(; *msg != '\0'; msg++);
		msg++;

		//If neither ackownledgements, print
		if(strcmp("OK", ack) || strcmp("ERROR", ack)) {
			//Print "sender: msg" to stdout
			if(ack[0] != '\n')
				dprintf(1, "<%s> %s\n", ack, msg);
		}

		ack = msg;
		//Reget the ack if any
		for(; *ack != '\0'; ack++);
		ack++;
	}
}

/*Reads from stdin and sends through socket
 * Usage: send a message to all by just typing
 * send a message to a directed user by typing :user
 * then your message. The message and user are seperated
 * by a space.
 */
void sendMessage(int fd) {
	char msg[1024];
	char buf[1024] = "";
	char to[32] = "";
	char *pMsg = &msg[0];

	//Read from stdin
	if(fgets(msg, 1024, stdin) != msg)
		return;

	//This is our to specifier
	if(msg[0] == ':') {
		int read = sscanf(&msg[1], "%s", to);
		pMsg = &msg[read + 2];
	}

	int len = snprintf(buf, 1024, "%s%c%s%c", to, 0, pMsg, 0);

	//Send to socket
	if(write(fd, buf, len) < 0) {
		perror("socket");
		exit(-1);
	}

	//Update time so we don't timeout
	time = get_wallTime();
}

/* Initial authentication where the reply should be OK or ERROR
 * If it is neither, we will exit
 */
void authenticate(int fd, char *userid, char *secret) {

	char buf[1024] = "";

	//Build a string such that "userid\0secret\0"
	int len = snprintf(buf, 1024, "%s%c%s%c", userid, 0, secret, 0);

	if(write(fd, buf, len) < 0) {
		perror("socket");
		exit(-1);
	}

	read(fd, buf, 1024);

	char *msg = &buf[0];
	char *ack = msg;

	//Gets past the ack to the actual msg
	for(; *msg != '\0'; msg++);
		msg++;

	if(!strcmp("OK", ack))
		dprintf(1, "Successfully connected as user: %s\n", userid);
	else {
		dprintf(2, "%s", msg);
		exit(-1);
	}

}

/* Checks if the time passed since last communication is two seconds
 * Send null strings if greater than two seconds
 */
void keepAlive(int fd) {
	
	double curTime = get_wallTime();
	int err;

	if(curTime - time > 2.0) {
		err = write(fd, "\0\0", 2);
		time = get_wallTime();
	}

	if(err < 0) {
		perror("Couldn't keep connection alive\n");
		exit(-1);
	}
}

/* The main logic of sending and receiving messages
 * Includes a loop of checking for IO and keeping the connection
 * alive in the case neither are detected
 */
void eventLoop(int fd) {

	//Our keep alive timeout
	struct timeval timeout = { 2, 0 };
	fd_set reads;

	//Creating an fd set with stdin and the socket
	FD_ZERO(&reads);
	FD_SET(STDIN_FILENO, &reads);
	FD_SET(fd, &reads);

	while(0 <= select(fd + 1, &reads, NULL, NULL, &timeout)) {
		if(FD_ISSET(STDIN_FILENO, &reads))
			sendMessage(fd);
		else if(FD_ISSET(fd, &reads))
			receiveMessage(fd);
		else
			keepAlive(fd);

		FD_SET(STDIN_FILENO, &reads);
		FD_SET(fd, &reads);
	}
}

/* Create a TCP connection to a remote server.
* address  The IPv4 dotted notation for an address (e.g., 1.2.3.4)
* port     The 16-bit port number
*  * NOTES: There are a lot of early returns in this function.  Most of this
* function we developed in class.  I added a few niceties.  Rather than report
* a failure's error number, I pass the number to strerror.  This function
* returns the human readable string associated with that error.
*/
int connect_to(char* address, unsigned short port) {

	struct sockaddr_in addr;

	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	if(!inet_aton(address, &addr.sin_addr)) {
		dprintf(2, "Could not convert \"%s\" to an address\n", address);
		return -1;
	}

	int fd = socket(PF_INET, SOCK_STREAM, 0);

	if(fd == -1) {
		dprintf(2, "Call to socket failed because (%d) \"%s\"\n", errno, strerror(errno));
		return -1;
	}

	if(connect(fd, (struct sockaddr*)&addr, sizeof(addr))) {
		dprintf(2, "Failed to connect to %s:%d because (%d) \"%s\"\n", address, port, errno, strerror(errno));
		close(fd);
		return -1;
	}

	return fd;
}

/* Gets the time for comparisons
 */
double get_wallTime() {
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return (double)(tp.tv_sec + tp.tv_usec / 1000000.0);
}