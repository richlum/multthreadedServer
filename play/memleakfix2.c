#include<stdio.h>
#include<pthread.h>
// this demonstrates the problem with the prev fix blocking when
// client takes time to execute

void run(void* c) {
	int cnt = *((int*)c);
	printf("%d  thread working\n",cnt);
	sleep(1);
   pthread_exit(0);
}

int main () {
   pthread_t thread;
   int rc;
   long count = 0;
   while(1) {
      if(rc = pthread_create(&thread, 0, run, (void*)&count) ) {
         printf("ERROR, rc is %d, so far %ld threads created\n", rc, count);
         perror("Fail:");
         return -1;
      }
      count++;
	  printf("%ld ",count);
	  pthread_join(thread,NULL);
   }
	
   return 0;
}
