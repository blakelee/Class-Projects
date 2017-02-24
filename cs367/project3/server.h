typedef struct msg {
	char *text;
	size_t len;
	size_t num;
} message;

typedef struct clientDetails {
	int authenticated;
	int fd;
	double lastActivity; //Time since last activity
	double slowRead;
	char *user;
	char *secret;
	message *inMsg;
	message *outMsg;
	struct clientDetails *next;
} client;

client *addClient(int fd);
void addCredentials(client *cl, char *user, char *secret);
client *deleteClient(client **head, client *cl);
client *readClient(client **head, client *cl);

int authenticate(client *c);
client *checkActivity(client **head, int *numClients);
void eventLoop(int socket);

int acceptClient(int fd);

client *writeClients(client **head, client *cl);

void writeAll(client **head, char *from, char *text, int len);
void writeDirected(client **head, char *to, char *text, int len);

int checkNulls(char *buf, int len);
int writeCredentials(client *cl);


message *makeMessage(message *old, char *text, int len);


double get_wallTime();
void FD_SET_CLIENT_READ(client **head, fd_set *rd);
void FD_SET_CLIENT_WRITE(client **head, fd_set *wr);
int getMaxFd(client **head, int socket);

int listen_on(unsigned short port, int queue);