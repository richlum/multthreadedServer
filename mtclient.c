/*
 * mtclient.c
 *
 *  Created on: 2013-01-12
 *      Author: rlum
 */

/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <time.h>

//#define PORT "3490" // the port client will be connecting to

#define MAXDATASIZE 100 // max number of bytes we can get at once

int doload(int sockfd){
	printf("---doload---\n");
	char buf[MAXDATASIZE];
	int numbytes;
    sprintf(buf,"load");

    if ((numbytes = send (sockfd,buf, strlen(buf),0)) == -1){
        perror("doload, send");
         exit(1);
    }

    if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }

    buf[numbytes] = '\0';

    printf("client: received '%s'\n",buf);
    int rc = atoi(buf);
    return rc;

}

int douptime(int sockfd){
	printf("---douptime---\n");
	char buf[MAXDATASIZE];
	int numbytes;
    sprintf(buf,"uptime");
    time_t ctime = time(NULL);

    if ((numbytes = send (sockfd,buf, strlen(buf),0)) == -1){
        perror("uptime, send");
         exit(1);
    }

    if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }

    buf[numbytes] = '\0';

    printf("client: received '%s'\n",buf);
    int rc = atoi(buf);
    int et = ctime-rc;
    if (et>=0)
    	printf("\tsleep ok: elaspsed time = %d\n",et);
    else
    	printf("\terror: time delta = %d\n", et);

    return et;

}

int doinvalid(int sockfd){
	printf("---doinvalid---\n");
	char buf[MAXDATASIZE];
	int numbytes;
    sprintf(buf,"invalid instruction");

    if ((numbytes = send (sockfd,buf, strlen(buf),0)) == -1){
        perror("doinvalid, send");
         exit(1);
    }

    if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }

    buf[numbytes] = '\0';

    printf("client: received '%s'\n",buf);
    int rc = atoi(buf);
    if (rc!=-1)
    	printf("\terror: invalid instruction returned %d\n", rc);
    else
    	printf("\tinvalidinstr ok\n");

    return rc;
}

int dosleep(int sockfd){
	printf("---dosleep---\n");
	char buf[MAXDATASIZE];
	int numbytes;
	// send nothing, just wait

    if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
    	printf("should timeout , rc = %d\n", numbytes);
    	perror("dosleep");
        exit(1);
    }

    buf[numbytes] = '\0';

    if (strlen(buf)>0)
    	printf("error during sleep: client received '%s'\n",buf);
    else
    	printf("dosleep ok\n");
    int rc = atoi(buf);
    return rc;

}


int doexit(int sockfd){
	printf("---doexit---\n");
	char buf[MAXDATASIZE];
	int numbytes;
    sprintf(buf,"exit");

    if ((numbytes = send (sockfd,buf, strlen(buf),0)) == -1){
        perror("exit, send");
         exit(1);
    }

    if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
    	printf("should timeout , rc = %d\n", numbytes);
    	perror("recv");
        exit(1);
    }

    buf[numbytes] = '\0';
    int rc = atoi(buf);

    if (rc!=0)
    	printf("\terror,doexit returned non zero value = %d\n",rc);
    else
    	printf("\tdoexit ok\n");

    return rc;

}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    int sockfd;
    //int numbytes;
    //char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    if (argc != 3) {
        fprintf(stderr,"usage: client hostname port\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure

    printf("cmd load = %d\n",doload(sockfd));
    printf("cmd uptime= %d\n", douptime(sockfd));
    printf("cmd invalidrc = %d\n", doinvalid(sockfd));
    printf("cmd sleeptimeoutrc = %d\n", dosleep(sockfd));
    printf("cmd exitrc = %d\n ", doexit(sockfd)	);

    close(sockfd);

    return 0;
}


