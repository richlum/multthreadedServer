#include <stdio.h>
#include <string.h>

void bar(char** p){
	(*p)++;



}

void foo(char** p){
	(*p)++;
	(*p)++;
	bar(p);
}



int main (int argc, char** argv){

	char mystuff[200];
	sprintf(mystuff,"12345678901234567790");
	char* ptr= mystuff;

	foo(&ptr);

	printf("%s\n",ptr);
}
