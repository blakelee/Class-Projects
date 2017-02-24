/* Blake Oliveira
 * Project 1 CS 367
 * Winter January 25, 2016
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

/* Reads read() without blocking */
void socket_read(int fd, char *buf) {

	ssize_t count = read(fd, buf, 40);

	if(count == -1)
		dprintf(2, "Read failed because (%d) \"%s\"\n", errno, strerror(errno));
}

/* Compares two strings based on the length of the first string
 * It only checks the first chars of buf where the str length is <= the length of msg
 */
int compare(char *msg, char *buf) {
	

	int i = 0;

	for(; msg[i]; i++) {
		if (msg[i] != buf[i])
			return 0;
	} 

	return 1;
}

/* Connects to each server and answers the questions as they come up
 */ 
int main() {
	char ip[20] = "140.160.136.195";
	unsigned short port = 12367;
	char *userid = "oliveib\n";
	char *secret = "cinquefoil\n";

	char buf[40];

	//Initial connection to server given by startup references
	int fd = connect_to(ip, port);
	dprintf(1, "Connected to %s:%d\n", ip, port);

	while(fd != -1) {
		socket_read(fd, buf);

		if(compare("userid:", buf))
			write(fd, userid, strlen(userid));

		else if(compare("secret:", buf))
			write(fd, secret, strlen(secret));

		else if(compare("good job", buf)) {
			dprintf(1, "%s\n", buf);
			return 0;
		}
		else {
			sscanf(buf, "%s %hu", ip, &port);
			close(fd);
			fd = connect_to(ip, port);

			if(fd != -1)
				dprintf(1, "Connected to %s:%d\n", ip, port);
		}
	}
}
