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

#undef DEBUG
#include "utility.h"
#include <assert.h>

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


//precondition - first char of buffer matches command
//postcondtion - head will point to next unprocessed char(if any)
//
//three possible outcomes
//	full match of command - may be more in buffer: return CMD_COMPLETE
//	partial match of command - need more chars: return CMD_INCOMPLETE
//	enough chars to count as invalid command, maybe more in buffer: return CMD_INVALID
// head will point to next unprocessed char (if any)
int validate(char** head, char** tail, char* command){
	int count = 0;
	int commandlength = strlen(command);
	char* savedhead = *head;
	while((**head == *command)&&(*head!=*tail)){
		(*head)++;
		command++;
		count++;
	}
	if (commandlength==count){
		TRACE
		//cmd complete, return command index
		if (strncmp(savedhead,"load", strlen("load"))==0){
			return CMD_LOAD;
		}else if (strncmp(savedhead,"uptime",strlen("uptime"))==0){
			return CMD_UPTIME;
		}else if (strncmp(savedhead,"exit", strlen("exit"))==0){
			return CMD_EXIT;
		}
		TRACE
		return CMD_INVALID;
	}else if (*head==*tail){
		TRACE
		//we have a partial correct match
		*head=savedhead;
		return CMD_INCOMPLETE;
	}else if ((*head<*tail)&&(commandlength>count)){
		TRACE
		//partial match to command failed,
		//reset head, then eat
		// the first char as the trigger for command invalid
		// and allow following chars to be retested subsequent calls
//		*head=savedhead;
//		(*head)++;


		//if we had ctl c in any of the buffer, quit now
		while(savedhead<*tail){
			if(*savedhead==(char)3){
				//control c in recv buffer
				//quit immediately
				return CMD_CONTROL_C;
			}
			savedhead++;
		}

		//late update on requirements,
		// treat 'lx' as a single error count, so instead of resetting
		// eat chars until we land at a valid start of command
		while((*head<*tail)&&(**head!='u')&&(**head!='l')&&(**head!='e')){
			(*head)++;
		}
		return CMD_INVALID;

	}

	//should not reach this point unless there are scenarios that
	//we havent explictly handled correctly
	TRACE
	return CMD_INVALID;


}

// client parsing, char by char processing of cmd buffer
// commandbuffer is start of buffer locations
// head is the pointer to the current unprocessed command - may be partial
// tail is pointer to the current last byte recd from socket
// here we will process a command if complete command is recd, if not
// we we return that CMD_INCOMPLETE so that socket can try to fill in the
// rest of the current(last command)

int parse( char** head,  char** tail){
#ifdef DEBUG
//	printf("%x  %c \n", (unsigned int)*head, (unsigned int)**head);
//	printf("%x  %c \n", (unsigned int)*tail, (unsigned int)**tail);
#endif
	int matched=0;
	// head == tail means buffer is empty. both should = command_buffer
	// head < tail means partial valid command still in buffer.
		// head must be pointing to beginning of next command.

	// head > tail error
		TRACE
		if (**head == 'u'){
			matched=validate(head,tail,"uptime");
		}else if (**head == 'e'){
			matched=validate(head,tail,"exit");
		}else if (**head == 'l'){
			matched=validate(head,tail ,"load");
		}else if (**head == (char) 3){
			TRACE
			matched=CMD_CONTROL_C;
		}else {
			TRACE
#ifdef DEBUG
			int i;
			for (i=0;i<5;i++){
				printf("\t%c\n",(*head)[i]);
			}
#endif
			TRACE
			printf("old cmd char='%c'\n",**head);
			(*head)++;

			if (*head!=*tail)
				printf("next cmd char='%c'\n",**head);
			return CMD_INVALID;
		}
		//head should be pointing to next unprocessed char if more chars left
		//or head and tail both pointing to start of buffer if command buffer
		//has been fully processed
	printf("matched=%d\n",matched);
	return matched;
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
void decrementclientcount(int sock){
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
	TRACE
	int flag = MSG_NOSIGNAL;;
	//char resp[BUFSIZE];
	int resp=0;
	int bytes=0;
	//memset(resp, '\0', BUFSIZE);

	switch (cmd) {
	case CMD_UPTIME:
		//sprintf(resp,"%ld", time(NULL));
		resp=time(NULL);
		break;
	case CMD_LOAD:
		//sprintf(resp,"%d", load());
		resp=load();
		break;
	case CMD_EXIT:
		//sprintf(resp,"%d", 0);
		resp=0;
		break;
	case CMD_CONTROL_C:
		resp=-1;
		break;
	default:
		TRACE
		//sprintf(resp,"%d", CMD_INVALID);
		resp=-1;
		break;
	}
	TRACE
	unsigned int sent=0;
	bytes=send(socket,&resp,sizeof(resp),flag);
	printf("sending(socket:%d) %d to cmd %d \n",socket, resp, cmd);
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
		//this was written before I understood responses are simple
		//integers. highly unlikely we cant send a 4 byte integer
		//but leaving it here in case we want to adapt for strings or
		//arrays of integers in response
		while(sent<sizeof(resp)){
			TRACE
			bytes=send(socket,(&resp)+sent,sizeof(resp)-sent,flag);
			printf("sending(socket:%d) resp=%d\n",socket,resp);
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
	TRACE
	if (cmd==CMD_EXIT){
		TRACE
		return 1;
	}
	// if we get here, cmd processing done, ok to continue
	TRACE
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
	int flag=MSG_NOSIGNAL;
	int errorcount=0;
	printf("new client started to handle socket fd = %d\n", sock);
	// set recv timeout incase caller sits idle too long
	struct timeval tv;
	tv.tv_usec=0;
	//todo reset this back to 30 seconds after testing
	tv.tv_sec = 300;
	if (setsockopt(sock,SOL_SOCKET, SO_RCVTIMEO,(void *)&tv,(socklen_t)sizeof(struct timeval))<0){
		printf("setsockopt for SO_RCVTIMEO failed\n");
		perror("so_rcvtimeo");
	}



	incrementclientcount();
	TRACE
	int cmdbufsize = BUFSIZE*2;
	char cmdbuffer[cmdbufsize];
	memset(cmdbuffer, '\0', BUFSIZE);
	char* head_cmd = cmdbuffer;
	char* tail_cmd = cmdbuffer;
	int done = 0;
	do {
		memset(buffer, '\0', BUFSIZE);

		bytes = recv(sock, buffer, BUFSIZE-1, flag);

		TRACE
		if (bytes==0){
			printf("socket: %d remote closed connection\n", sock);
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
		buffer[BUFSIZE]='\0';
		printf("Server(%d) Recd: '%s'\n",sock,buffer);
		//copy to cmd_buffer
		//since there may be a residual command portion in cmdbuffer, we
		//  copy to tail to append to existing cmdbuffer commands (if any).
		int i;
		for (i=0;i<bytes;i++){
			tail_cmd[i]=buffer[i];
		}
		tail_cmd=tail_cmd+i;

		//printf("head_cmd = %x", (unsigned int)head_cmd);
		int cmd = parse(&head_cmd, &tail_cmd);
		//printf("head_cmd = %x", (unsigned int)head_cmd);
		if (cmd == CMD_CONTROL_C){
			done = 1;
			if (SENDERRORONCONTROL_C){
				TRACE
				done = docmd(cmd,sock);
			}
			break;
		}else if (cmd!=CMD_INCOMPLETE){
			TRACE

			if (cmd>=0) {
				TRACE
				errorcount=0;
				printf("reset(%d) errorcount=%d\n",sock,errorcount);
			}else if (cmd==CMD_INVALID){
				errorcount++;
				printf("errorcount(%d)=%d\n",sock, errorcount);
			}
			TRACE
			//handling 3rd consec error for multiple commands in a packet
			// can just disconnect or send response then send disconnect
			if ((errorcount>=MAXREMOTEERRORS)&& !SENDERRORON3RD_ERROR_SEP_PACKETS){
				done=1;
				break;
			}
			done=docmd(cmd,sock);
			if (done){
				TRACE
				//recd an exit in a string of commands, do not process next cmd
				break;
			}
			TRACE
			while(head_cmd!=tail_cmd){
				TRACE
				//theres more commands in command buffer, lets do
				// them before we ask socket for more to do
				cmd = parse( &head_cmd, &tail_cmd);
				if (cmd == CMD_CONTROL_C){
					TRACE
					done = 1;
					if (SENDERRORONCONTROL_C){
						done = docmd(cmd,sock);
					}
					break;
				}
				TRACE
#ifdef DEBUG
				printf("cmd(%d)=%d\n",sock,cmd);
#endif
				if (cmd==CMD_INVALID){
					TRACE
					errorcount++;
					printf("incr(%d) errorcount=%d\n",sock,errorcount);
					if ((errorcount>=MAXREMOTEERRORS)&&(!SENDERRORON3RD_ERROR_SAME_PAKCETS)){
						TRACE
						done=1;
						break;
					}
					done=docmd(cmd,sock);
					TRACE
				}else if (cmd>=0){
					errorcount=0;
					printf("reset(%d) errorcount=%d\n",sock, errorcount);
					done=docmd(cmd,sock);
				}else{ //cmd==CMD_INCOMPLETE
					TRACE
#ifdef DEBUG
					printf("cmd(%d)=%d\n",sock,cmd);
//					printf("head_cmd = %lx\n", (long long int)head_cmd);
//					printf("tail_cmd = %lx\n", (long long int)tail_cmd);
#endif
					//CMD_INCOMPLETE - goback to recv for more data

					//mv partial command to front of cmdbuffer
					if (head_cmd!=cmdbuffer){
						i=0;
						while(head_cmd<tail_cmd){
							cmdbuffer[i] = *head_cmd;
							i++;
							head_cmd++;
						}
						head_cmd=cmdbuffer;
						//cmdbuffer[i]='\0';
						//this is where recv should start appending.
						tail_cmd=&cmdbuffer[i];
						//we must exit this loop with head_cmd<tail_cmd
						//to preserver our partial 'so far valid' cmd
						break;
					}

				}
			}
			// if cmdbuffer hasnt been setup by partial cmd handling
			if (head_cmd!=cmdbuffer){
				//buffer fully processed reset to start of cmdbuffer for next recv
				head_cmd=cmdbuffer;
				tail_cmd=cmdbuffer;
			}
			//this was reset either by completed commands or by partial last cmd
			//eitherway this is necessary to prevent cmd buffer overflows when
			//appending from tcp buffer into command buffer
			assert(head_cmd==cmdbuffer);

		}else if (cmd==CMD_INCOMPLETE){
			//this is the first command in tcp buffer,
			//potentially valid command rec'd but incomplete
			done=0;  //not done, need more bytes
					// head_cmd and tail_cmd are already set up correctly
			continue;
		}else{
			//this branch is for complete commands within seperate packet
			TRACE
			errorcount++;
			printf("incr(%d) errorcount=%d\n",sock,errorcount);
			if ((errorcount>=MAXREMOTEERRORS)&&(!SENDERRORON3RD_ERROR_SEP_PACKETS)){
				// this is the 3rd consec error and we dont want to respond, just disc
				done=1;
				break;
			}
			TRACE
			done=docmd(INVALID,sock);
		}
		TRACE
		if (errorcount>=MAXREMOTEERRORS){
			done=1;
		}

	}while(!done);
	TRACE
	printf("closing socket fd=%d\n",sock);
	close(sock);
	decrementclientcount(sock);
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
		TRACE
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
			printf("server socket %d received connection from: %s\n", new_sockfd, address);
			//handle_client(new_sockfd);
			int socknum=new_sockfd;
			pthread_t thread;
			int rc = pthread_create(&thread, NULL, handle_client, (void*)&socknum);
			if (rc!=0){
				printf("server pthread error %d for socket %d\n",rc, new_sockfd);
			}else{
				//non blocking, will release memory when thread is completed
				pthread_detach(thread);
				TRACE
			}
			TRACE

		}
		TRACE
	}
	TRACE




	return EXIT_SUCCESS;
}

