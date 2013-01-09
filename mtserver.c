#include <stdio.h>
#include <ctype.h>
#include <stdlib.h> //atoi
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>  //memset
#include <unistd.h>
#include <arpa/inet.h>

void printhelpmessage(void)
{
	printf("usage: mtserver maxclients portnumber\n");

}

int valid(int connections, int port){
	if (connections<=0)
		return 0;
	if (port<=0)
		return 0;
	return 1;
}


// based on beej's guide example server
int get_main_socket(char* port, unsigned int connections)
{
	int sockfd;
	struct addrinfo hints;
	struct addrinfo *p;
	struct addrinfo *servinfo;
	int rc ;
	int true = 1;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family= AF_INET; //assume we only want ipv4
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;


	if ((rc=getaddrinfo(NULL, port, &hints, &servinfo))!=0){
		printf("getaddrinfo failed: %s\n", gai_strerror(rc));
		return -1;
	}
	// servinfo is output - linked list of addresses we could bind to

	for (p = servinfo; p!=NULL; p=p->ai_next){
		if ((sockfd=socket(p->ai_family, p->ai_socktype,p->ai_protocol))
				== -1){
			perror("server socket failed");
			continue;  // this one didnt work, try next
		}	
		//SOL_SOCKET is level required to allow immediate socket reuse
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,&true, sizeof(int))
				== -1){
			perror("server setsocket failed");
			continue;
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
			close(sockfd);
			perror("server bind failed");
			continue;
		}
		//if we get here, we have a bound socket
		break;
	}

	if (p==NULL){
		printf("server unable to bind to socket\n");
		return -2;
	}

	freeaddrinfo(servinfo);

	if(listen(sockfd, connections) == -1){
		perror("listen");
		return -1;
	}

	//this is our  listening socket
	return sockfd;

}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


// should take in 2 parameters 
// maxclients, server port number
int main(int argc, char** argv)
{
	struct sockaddr_storage remote_address;
	socklen_t  size_remote_addr;
	int new_sockfd;
	char address[INET6_ADDRSTRLEN];

	if (argc!=3){
		printhelpmessage();
		exit(EXIT_SUCCESS);
	}

	unsigned int max_connections = atoi(argv[1]);
	unsigned int server_port = atoi(argv[2]);
	if (!valid(max_connections, server_port)){
		printhelpmessage();
		exit(EXIT_SUCCESS);
	}

	// create a socket
	/**
int main_socket = socket (AF_INET, SOCK_STREAM, 0);
if (main_socket < 0){
	printf("Unable to assign main socket");
	perror("main_socket");
}
	 **/
	int serversocket=get_main_socket(argv[2],max_connections);
	if (serversocket<0){
		printf("failed to get main socket");
		exit(EXIT_FAILURE);
	}


	while(1){
		size_remote_addr = sizeof (remote_address);
		new_sockfd = accept(serversocket, (struct sockaddr*)&remote_address, &size_remote_addr);
		if (new_sockfd == -1){
			perror("Server Accept error");
			continue; // this one failed try again with next one
		}
		//get the remote ip address
		inet_ntop(remote_address.ss_family, get_in_addr((struct sockaddr*)&remote_address),
				address, sizeof(address));
		printf("server received connection from: %s", address);
	}




	return EXIT_SUCCESS;
}

