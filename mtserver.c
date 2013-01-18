#include <stdio.h>
#include <ctype.h>
#include <stdlib.h> //atoi
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>  //memset
#include <unistd.h>
#include <arpa/inet.h>
#include "mtserver.h"

#include <time.h>
#include <pthread.h>

#define DEBUG
#include "utility.h"

time_t start, end;
unsigned int clientcount=0;
pthread_mutex_t cl_cnt_mutex = PTHREAD_MUTEX_INITIALIZER;



void printhelpmessage(void)
{
	printf("usage: mtserver maxclients portnumber\n");

}

// check main arg input
int valid(int connections, int port){
	if (connections<=0)
		return 0;
	if (port<=0)
		return 0;
	return 1;
}

///////////////////////////////
// allocate main server socket
// heavily based on beej's guide example server
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


////////////////////////////////
// client command handlers
int parse(char* commandbuffer){
	char* ptr;

	if((ptr = strstr(commandbuffer, "uptime"))!=NULL){
		return CMD_UPTIME;
	}else if ((ptr=strstr(commandbuffer,"load"))!=NULL){
		return CMD_LOAD;
	}else if ((ptr=strstr(commandbuffer,"exit"))!=NULL){
		return CMD_EXIT;
	}
	return CMD_INVALID;
}

// spec says to return unix time, not server uptime
//double uptime(void){
//	double diff;
//
//	time(&end);
//	diff = difftime(end,start);
//	return diff;
//
//}


/////////////////////////////////////
//client count shared variable for load reporting
//these have to have mutex protecting all reads and writes
int load(){
	unsigned int load;
	pthread_mutex_lock(&cl_cnt_mutex);
	load=clientcount;
	pthread_mutex_unlock(&cl_cnt_mutex);
	return load;
}

void incrementclientcount(void){
	pthread_mutex_lock(&cl_cnt_mutex);
	clientcount++;
#ifdef DEBUG
	printf ("incrementclientcount to=%d\n", clientcount);
#endif
	pthread_mutex_unlock(&cl_cnt_mutex);

}
void decrementclientcount(void){
	pthread_mutex_lock(&cl_cnt_mutex);
	if (clientcount>0)
		clientcount--;
	else
		printf("error decrementing client count = %d\n", clientcount);
	pthread_mutex_unlock(&cl_cnt_mutex);
}
unsigned int getclientcount(void){
	pthread_mutex_lock(&cl_cnt_mutex);
	int count = clientcount;
	pthread_mutex_unlock(&cl_cnt_mutex);
	return count;
}



/////////////////////////////////////////
//execute cmd and send results to socket
//return 1 if socket is shutdown,
//return 0 if socket can continue servicing cmds
int docmd(int cmd, int socket){
	int flag = 0;
	char resp[BUFSIZE];
	int bytes=0;
	memset(resp, '\0', BUFSIZE);

	switch (cmd) {
	case CMD_UPTIME:
		sprintf(resp,"%ld", time(NULL));
		break;
	case CMD_LOAD:
		sprintf(resp,"%d", load());
		break;
	case CMD_EXIT:
		sprintf(resp,"%d", 0);
		break;
	default:
		sprintf(resp,"%d", CMD_INVALID);
		break;
	}

	unsigned int sent=0;
	bytes=send(socket,resp,strlen(resp),flag);
	if (bytes==0){
		TRACE
		printf("socket(%d) , remote closed connection\n", socket);
		return 1;
	}else if (bytes<0){
		TRACE
		printf("socket(%d) , send error\n", socket);
		perror("Server send error");
		return 1;
	}else{
		sent=bytes;
		while(sent<strlen(resp)){
			bytes=send(socket,resp+sent,strlen(resp)-sent,flag);
			sent+=bytes;
			if (bytes==0){
				printf("socket(%d), remote closed connection", socket);
				return 1;
			}else if (bytes<0){
				printf("socket(%d) , send error\n", socket);
				perror("Server send error");
				return 1;
			}
		}
	}
	if (cmd==CMD_EXIT){
//		close(socket);
//		printf("socket(%d) , closed\n", socket);
//		decrementclientcount();
		return 1;
	}
	// if we get here, cmd processing done, ok to continue
	return 0;
}




////////////////////////////////////////
//client handler per client (per thread)
// incoming request message
//	message must be complete within a single packet
//void handle_client(int sock){
void *handle_client(void* arg){
	int sock = *((int*)arg);
	char buffer[BUFSIZE];
	int bytes;
	int flag=0;
	int errorcount=0;

	// set recv timeout incase caller sits idle too long
	struct timeval tv;
	tv.tv_usec=0;
	tv.tv_sec = 30 ; //30 second timeout value
	if (setsockopt(sock,SOL_SOCKET, SO_RCVTIMEO,(void *)&tv,(socklen_t)sizeof(struct timeval))<0){
		printf("setsockopt for SO_RCVTIMEO failed\n");
		perror("so_rcvtimeo");
	}

	incrementclientcount();
	TRACE
	int done = 0;
	do {
		memset(buffer, '\0', BUFSIZE);
		bytes = recv(sock, buffer, BUFSIZE, flag);
		TRACE
		if (bytes==0){
			printf("socket: %d remote closed connection\n", sock);
//			close(sock);
//			decrementclientcount();
//			return 0;
			done=1;
			break;
		}else if (bytes==-1){
			//timeout will trigger this branch as well
			printf("recv error\n");
			perror("recv");
			done=1;
			break;
		}
		TRACE
		buffer[BUFSIZE-1]='\0';
		printf("%s\n",buffer);
		int cmd = parse(buffer);
		if (cmd>=0){
			done=docmd(cmd,sock);
			errorcount=0;
		}else{
			done=docmd(INVALID,sock);
			errorcount++;
		}
		if (errorcount>=3){
			done=1;
		}

	}while(!done);
	close(sock);
	decrementclientcount();
	return 0;

}

/////////////////////////////////////////////
// take in 2 parameters
// maxclients, server port number
int main(int argc, char** argv)
{

	struct sockaddr_storage remote_address;
	socklen_t  size_remote_addr;
	int new_sockfd;
	char address[INET6_ADDRSTRLEN];
	//int server_sleep_seconds = 1;

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


	time(&start); // start timer;

	//allocate memory to hold max_connections threads
	//pthread_t *threads = (pthread_t*)malloc(max_connections*sizeof(pthread_t));
	//int *socknum = (int*)malloc(max_connections*sizeof(int));

	// create a socket
	int serversocket=get_main_socket(argv[2],max_connections);
	if (serversocket<0){
		printf("failed to get main socket");
		exit(EXIT_FAILURE);
	}


	while(1){
		size_remote_addr = sizeof (remote_address);
		TRACE
		new_sockfd = accept(serversocket, (struct sockaddr*)&remote_address, &size_remote_addr);
		unsigned int clients = getclientcount();
		if (clients<max_connections){
			printf("connection %d <= max %d, new_sockfd=%d\n", clients+1,max_connections,new_sockfd);
		}else{
			printf("connection %d exceeded %d, closing %d\n", clients+1,max_connections, new_sockfd);
			close(new_sockfd);
//			while(getclientcount()>=(max_connections-1)){
//#ifdef DEBUG
//				printf(".");
//#endif
//				sleep(server_sleep_seconds);
//
//			}
			printf("maxconn exceeded: socket closed\n");
			continue;
		}
		TRACE
		if (new_sockfd == -1){
			perror("Server Accept error");
			continue; // this one failed try again with next one
		}
		//get the remote ip address
		inet_ntop(remote_address.ss_family, get_in_addr((struct sockaddr*)&remote_address),
				address, sizeof(address));
		if (address==NULL){
			perror ("Server unable to get remote address");
			continue;
		}else{
			TRACE
			//fixme : note that clients can and will close in order different that open order
			//  need to handle finding empty slot for clients as long as we are less than max
			printf("server socket %d received connection from: %s", new_sockfd, address);
			//handle_client(new_sockfd);
			int socknum=new_sockfd;
			pthread_t thread;
			int rc = pthread_create(&thread, NULL, handle_client, (void*)&socknum);
			if (rc!=0){
				printf("server pthread error %d for socket %d\n",rc, new_sockfd);
			}else{
				//non blocking, will release memory when thread is completed
				pthread_detach(thread);
			}

		}
	}




	return EXIT_SUCCESS;
}

