#include <string.h>
#include <stdio.h>

#define CMD_INVALID -1


int validate(char* head, char* command){
	int count = 0;
	int cmdlength = strlen(command);
	while((*head ==*command)&&(*command!='\0')){
		head++;
		command++;
		count++;
	}
	if (cmdlength==count){
		// full command match
		// head now points to next position in command buffer
		return count;
	}
	return CMD_INVALID;
}



int main(int argc, char** argv){

char word[10];
char another[10] = {'q','r','s','t','\0'};

memset(word,'\0',sizeof(word));

strcpy(word,"ab");	

printf("%d\n",strlen(word));
int rc = validate (word, argv[1]);
printf("validate rc = %d\n", rc);

char* achar = another;
word[0] = *achar;

printf("%s\n",word);
}

