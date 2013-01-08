#include <stdio.h>
#include <ctype.h>
#include <stdlib.h> //atoi
#include<sys/types.h>
#include <sys/socket.h>

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
int get_main_socket(unsigned int port)
{
	int sockfd;
	struct addrinfo hints,*servinfo	;
	int rc ;

	memset(&hints, 0, sizeof(hints);
	hints.ai_family=AF_INET; //assume we only want ipv4
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

	if(listen(sockfd, BACKLOG) == -1){
		perror("listen");
		return -1;
	}
	
	
}

// should take in 2 parameters 
// maxclients, server port number
int main(int argc, char** argv)
{

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







return EXIT_SUCCESS;
}

