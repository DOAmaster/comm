//modified by: Derrick Alden
//date:
//changes:
//
//program: client1.cpp
//author: Gordon Griesel
//date: Winter 2018
//about: simple TCP client that connects to a server
//
//reference:
//http://www.linuxhowtos.org/C_C++/socket.htm
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
void removeCRLF(char *str);
void error(const char *msg);

class Global {
public:
	int sockfd, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	char buffer[256];
	Global() { }
	int connectWithServer(char *host, int port) {
		printf("connectWithServer(%s, %i)...\n", host, port);
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0) { 
			error("ERROR opening socket");
			return 1;
		}
		server = gethostbyname(host);
		if (server == NULL) {
			fprintf(stderr, "ERROR, no such host\n");
			//exit(0);
			return 1;
		}
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		memmove(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
		serv_addr.sin_port = htons(port);
		int ret =
			connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)); 
		if (ret < 0) {
			error("ERROR connecting");
			return 1;
		}
		return 0;
	}
	void closeConnection() {
		close(sockfd);
	}
	int readFromServer(char *buffer) {
		memset(buffer, 0, 256);
		n = read(sockfd, buffer, 255);
		if (n < 0) {
			error("ERROR reading from socket");
			return 1;
		}
		return 0;
	}
	int writeToServer(char *mess) {
		int n = write(sockfd, mess, strlen(mess));
		if (n < 0) 
			error("ERROR writing to socket");
		return n;
	}
} g;

int main(int argc, char *argv[])
{
	char buffer[256];
	if (argc < 3) {
		printf("Usage: %s <hostname> <port>\n", argv[0]);
		printf("   example: %s sleipnir 8888\n", argv[0]);
		return 0;
	}
	g.connectWithServer(argv[1], atoi(argv[2]));
	//------------------------------------------------------
	//Get any message from server.
	while(true) {
	printf("read server...\n");
	g.readFromServer(buffer);
	printf("%s\n", buffer);
	//Send your own message to server.
	char message[256];
	printf("Enter a message: ");
	fgets(message, 255, stdin);
	g.writeToServer(message);
	}
	//------------------------------------------------------
	g.closeConnection();
	return 0;
}

void error(const char *msg)
{
	perror(msg);
	exit(0);
}




