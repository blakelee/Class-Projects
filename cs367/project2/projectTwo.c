/*
*Sam Jamalzada
*CSIC 367 
*Project Two: Chat server
*February 16, 2016
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
#include <time.h>


/*
* declaration of structure
* a character buffer
* a size_t to indicate buffer size
* a size_t to indicate read buffer content
*/
struct myContext{
  char *buffer;
  size_t size;
  size_t length;
};

/*
* addMsg method
* pram: pointer to initialized myContext structure 
* pram: a character buffer
* pram: size_t to indicate size of buffer
* adds a message to the buffer in context
*/
void addMsg(struct myContext *context, char *buff, size_t buffSize){
  for(int i = 0; i < buffSize; i++){
    context->buffer[context->length] = buff[i];
    context->length++;
  }
}

/*
* removeMsg method
* pram: pointer to initialized myContext structure 
* pram: size_t to indicate size of message to be removed 
* removes a complete message from buffer using memmove 
*/
void removeMsg(struct myContext *context, size_t n){
  memmove(context->buffer, context->buffer + n, context->length - n);
  context->length = context->length - n;
}



/*
* completeMsg method
* pram: pointer to initialized myContext structure 
* reads buffer to determine is it contains full message 
* returns size of message or 0
*/
size_t completeMsg(struct myContext *context){
  int countNull = 0;
  for(int i = 0; i < context->length; i++){
    if(context->buffer[i] == '\0'){
      countNull++; 
      if(countNull == 2)
        return i;
    }
  }
  return 0;
}



/*
* createContext method
* initializes myContext structure
* returns a myContext structure
*/
struct myContext createContext(){
  struct myContext context;
  char *buff = malloc(80);
  context.buffer = buff;
  context.size = 80;
  context.length = 0;
  return context;
}




/*
* expandContext method
* pram: pointer to initialized myContext structure 
* doubles old buffer, copies content 
* frees old buffer
*/
void expandContext(struct myContext *context){
  char *buff = malloc(context->size * 2);
  memcpy(buff, context->buffer, context->length);
  free(context->buffer); 
  context->buffer = buff;
}



/*
* authentication method
* pram: a file descriptor
* builds user information
* writes user information to file descriptor 
*/
int authentication(int fd, char *user){
  char buffer[80];
  char *info;
  int len = snprintf(buffer, 80, "%s%c%s%c", user, 0, "cinquefoil", 0);
  info = &buffer[0];
  write(fd, info, len);
  size_t count = read(fd, buffer, 80);
  write(STDOUT_FILENO, buffer, count);
  printf("\n");
  if(!strcmp("OK", buffer)){
    return 1;
  }
  return 0;
}



/*
* msg method
* pram: a file descriptor 
* reads from the standard in
* writes to buffer
* builds final string
* writes final message to socket
*/
void msg(int fd){
  char buffer[1024];
  char user[1024];
  char data[1024];
  char final[1024];

  size_t count = read(STDIN_FILENO, buffer, 1024);
  if(buffer[0] == '@'){
    memmove(buffer, buffer+1, strlen(buffer));
    sscanf(buffer, "%s %s", user, data);
    snprintf(final, count-1, "%s%c%s%c", user, '\0', data, '\0');
    write(fd, final, count-1);
  }
  else{
    snprintf(final, count+1, "%c%s%c", '\0', buffer, '\0');
    write(fd, final, count+1);
  }
}


/*
* receive method
* pram: a file descriptor
* pram: pointer to initialized myContext structure 
* reads from the file descriptor 
* stores reads the data into buffer in context
* writes buffer back out to standard out
* invokes removeMsg(), expandContext(), completeMsg(), addMsg()
* return 0 when nothing is read from socket
* else returns 1
*/
int receive(int fd, struct myContext *context) {
  char *buff = malloc(80);
  ssize_t count = read(fd, buff, 80);
  if((context->length + count) > context->size){
    expandContext(context);
  }
  addMsg(context, buff, count);

  size_t length = completeMsg(context);
  while(length){
    write(STDOUT_FILENO, context->buffer, length);
    printf("\n");
    removeMsg(context, length);
    length = completeMsg(context);
  }

  return count;
}


/*
* heartbeat method
* pram: a file descriptor
* writes null message to socket
*/
void heartbeat(int fd){
  char *empty = "\0\0";
  write(fd, empty, 2);
}



/*
* eventLoop method
* pram: a file descriptor
* uses select to determine which function needs to be called
* repeats until server disconnects 
*/
void eventLoop(int fd){
  int receiving = 1; 
  struct myContext context = createContext();
  struct timeval timeout = {2, 0};

  fd_set reads;
  FD_ZERO(&reads);
  FD_SET(0, &reads);
  FD_SET(fd, &reads);
  while(receiving && 0 <= select(fd+1, &reads, NULL, NULL, &timeout)){
    if(FD_ISSET(0, &reads)){
      msg(fd);
    }

    else if(FD_ISSET(fd, &reads)){
      receiving = receive(fd, &context);
    }

    else{
      heartbeat(fd);
    }

    timeout.tv_sec = 2; 
    timeout.tv_usec = 0; 
    FD_ZERO(&reads);
    FD_SET(0, &reads);
    FD_SET(fd, &reads);
  }
}



/*
* NOTE: function is the work of Aron Clauson
* connects to specified IP address and port number
*/
int connect_to (const char* address, int port){
  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  if(!inet_aton(address, &addr.sin_addr)){
    fprintf(stderr, "Could not convert \"%s\" to an address\n", address);
    return -1;
  }

  int fd = socket(PF_INET, SOCK_STREAM, 0);
  if(fd == -1){
    fprintf(stderr, "Call to socket failed because (%d) \"%s\"\n",
      errno, strerror(errno));
    return -1;
  }

  if(connect(fd, (struct sockaddr*)&addr, sizeof(addr))){
    fprintf(stderr, "Failed to connect to %s:%d because (%d) \"%s\"\n",
      address, port, errno, strerror(errno));
    close(fd);
    return -1;
  }
  return fd;
}



/*
* main method
* invokes connect_to function for connecting to the server
* invokes authentication method for initial setup
* invokes eventloop function for continued communication with server
*/
int main(int argc, char **argv){
  int fd = connect_to("127.0.0.1", 7000);
  if (fd != -1) {
    if(authentication(fd, argv[1])){
      eventLoop(fd);
      return EXIT_SUCCESS;
    }  
  }
  return EXIT_FAILURE;  
}
