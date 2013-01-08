#include <stdio.h>
#include <ctype.h>
#include <stdlib.h> //atoi

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
if (valid(max_connections, server_port)){
	printhelpmessage();
	exit(EXIT_SUCCESS);
}












}

