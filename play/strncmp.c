#include <string.h>
#include <stdio.h>

int main(int argc, char** argv){
	char buffer[30];
	strcpy(buffer,"test");
	if(strncmp(buffer,"tes",strlen("tes"))==0){
		printf("match");
	}else{
		printf("nomatch");
	}


}

