#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int x;
int y;
int ans;

void *thread_multi(void *threadid)
{
	int i;
	for (i = 0; i < 10000; i++) {
		x = 5;
		y = 5;
		ans = x * y;
		printf("The ans of the plus is %i!\n", ans);
	}
	pthread_exit(NULL);
}

void *thread_plus(void *threadid)
{
	int i;
	for (i = 0; i < 10000; i++) {
		x = 6;
		y = 6;
		ans = x + y;
		printf("The ans of the plus is %i!\n", ans);
	}
	pthread_exit(NULL);
}

int main (int argc, char *argv[])
{
	pthread_t threads[2];
	int rc;
	rc = pthread_create(&threads[0], NULL, thread_multi, NULL);
	if (rc){
		printf("ERROR; return code from pthread_create() is %d\n", rc);
		exit(-1);
	}
	rc = pthread_create(&threads[1], NULL, thread_plus, NULL);
	if (rc){
		printf("ERROR; return code from pthread_create() is %d\n", rc);
		exit(-1);
	}
   pthread_exit(NULL);
}
