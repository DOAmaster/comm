//Handle multiple socket connections with select and fd_set on Linux.
//This does not need forks or threads.
//http://www.binarytides.com/multiple-socket-connections-fdset-select-linux/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
//FD_SET, FD_ISSET, FD_ZERO macros
#include <sys/time.h>
#define TRUE   1
#define FALSE  0
#define PORT 8888
 
int main(int argc , char *argv[])
{
	int opt = TRUE;
	const int MAX_CLIENTS = 30;
	int client_socket[MAX_CLIENTS] = {0};
	int nclients = 0;
	int master_socket, addrlen, new_socket, activity, sd;
	int max_sd;
	struct sockaddr_in address;
	//data buffer to hold 1K
	char buffer[1025];
	char message[] = "Connected to 4490 game server #3";
	//set of socket descriptors
	fd_set readfds;
	//create a master socket
	if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}
	//set master socket to allow multiple connections,
	// this is just a good habit, it will work without this
	if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR,
			(char *)&opt, sizeof(opt)) < 0) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	//type of socket created
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);
	//bind the socket to localhost port 8888
	if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	printf("Listener on port %d \n", PORT);
	//try to specify maximum of 3 pending connections for the master socket
	if (listen(master_socket, 3) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}
	//accept the incoming connection
	addrlen = sizeof(address);
	max_sd = master_socket;
	printf("Waiting for connections ...\n");
	while (TRUE) {
		//clear the socket set
		FD_ZERO(&readfds);
		//add master socket to set
		FD_SET(master_socket, &readfds);
		//add children sockets to set
		for (int i=0; i<nclients; i++) {
			FD_SET(client_socket[i], &readfds);
		}
		//Wait for an activity on one of the sockets,
		// timeout is NULL, so wait indefinitely.
		printf("select is blocking now...\n");
		activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
		if ((activity < 0) && (errno != EINTR)) {
			printf("select error");
		}
		//If something happened on the master socket,
		// then its an incoming connection
		if (FD_ISSET(master_socket, &readfds)) {
			new_socket = accept(master_socket, (struct sockaddr *)&address,
				(socklen_t*)&addrlen);
			if (new_socket < 0) {
				perror("accept");
				exit(EXIT_FAILURE);
			}
			//inform user of socket number used in send and receive commands
			printf("New connection, socket fd: %d, ip: %s, port: %d\n",
				new_socket, inet_ntoa(address.sin_addr),
				ntohs(address.sin_port));
			//send new connection greeting message
			int slen = strlen(message);
			int ret = send(new_socket, message, slen, 0);
			if (ret != slen) {
				perror("send");
			} else {
				printf("Welcome message sent successfully.\n");
			}
			//add new socket to array of socket handles
			client_socket[nclients] = new_socket;
			printf("Adding to list of sockets as %i at index %i\n",
				new_socket, nclients);
			++nclients;
			printf("%i clients are currently connected.\n", nclients);
			//keep max socket value updated.
			max_sd = master_socket;
			for (int i=0; i<nclients; i++) {
				if (client_socket[i] > max_sd)
					max_sd = client_socket[i];
			}
		}
		//else it's some IO operation on some other socket :)
		for (int i=0; i<nclients; i++) {
			sd = client_socket[i];
			if (FD_ISSET(sd, &readfds)) {
				//Check if it was for closing,
				// and also read the incoming message
				int messlen = read(sd, buffer, 1024);
				buffer[messlen] = '\0';
				printf("incoming message: **%s**\n", buffer);
				if (messlen == 0) {
					//Somebody disconnected, get his details and print
					getpeername(sd, (struct sockaddr*)&address,
						(socklen_t*)&addrlen);
					printf("Host disconnected, ip: %s, port: %d\n",
						inet_ntoa(address.sin_addr), ntohs(address.sin_port));
					//Close the socket and mark as 0 in list for reuse
					close(sd);
					//Move the last client socket to this now-vacant spot.
					//Order is not important.
					//A now-unsued array index may get overwritten in the
					//following statement, but it will not cause an error
					//and doesn't really matter.
					client_socket[i] = client_socket[--nclients];
					printf("%i clients are currently connected.\n", nclients);
					//keep max socket value updated.
					max_sd = master_socket;
					for (int i=0; i<nclients; i++) {
						if (client_socket[i] > max_sd)
							max_sd = client_socket[i];
					}
				} else {
					//Echo back the message that came in
					//set the string terminating NULL byte on
					//the end of the data read
					printf("sending back **%s**\n", buffer);
					send(sd, buffer, strlen(buffer), 0);
				}
			}
		}
	}
	return 0;
}

