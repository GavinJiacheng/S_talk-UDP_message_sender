#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include "list.h"


LIST* list;
LIST* ReceiveLIst;
int do_threads = 1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
struct sockaddr_in remoteAddr;

void* input_message(void* arg){
	char buff[1024];
	int s = *((int*)arg); // to get the real number from the address rather than the (int) arg;
	while(do_threads){
		fgets(buff,1024,stdin);
		if ((strcmp(buff,"!\n") == 0)){
			do_threads =0;
			close(s);
			exit(1);
		}
		ListAdd(list,buff);
	}
}

void* receive_message(void* arg){
	char rece[1024];
	int s = *((int*)arg);
	int len = sizeof(remoteAddr);
	while(do_threads){
		int ret = recvfrom(s,rece,1024,0,(struct sockaddr *)&remoteAddr,&len);
		if (ret > 0)
		{
			pthread_mutex_lock(&mutex);
			ListAdd(ReceiveLIst,rece);
			pthread_mutex_unlock(&mutex);
		}
	}
}

void* send_message(void* arg){
	char* sendData;
	int s = *((int*)arg);
	int len = sizeof(remoteAddr);
	while(do_threads){
		if(ListCount(list) !=0){
			pthread_mutex_lock(&mutex);
			sendData = ListRemove(list);
			pthread_mutex_unlock(&mutex);
			sendto(s,sendData,1024,0,(struct sockaddr*)&remoteAddr,len);
		}
	}
}

void* put_message(void* arg){
	char* buff;
	int s = *((int*)arg);
	while(do_threads){
		if(ListCount(ReceiveLIst) != 0){
			pthread_mutex_lock(&mutex);
			buff = ListRemove(ReceiveLIst);
			pthread_mutex_unlock(&mutex);
			printf("They: ");
			fputs(buff,stdout);
		}
	}
}

int main(int argc, char **argv){
	if(argc < 4){
		printf("The argument is not valid.\n Please type in [./s-talk][my port number][remote machine name][remote port number].\n");
		exit(1);
	}
	list = ListCreate();
	ReceiveLIst = ListCreate();

	int s;
	int ret1, ret2, ret3, ret4;
	struct hostent* remoteName;
	if((remoteName = gethostbyname(argv[2])) == NULL){
		printf("Hostname invalid.\n");
		exit(1);
	}
	pthread_t thread1, thread2, thread3, thread4;

	s = socket(AF_INET, SOCK_DGRAM, 0);
	if(s < 0){
		perror("Socket failed to create.\n");
		exit(1);
	}
	printf("Socket created.\n");

	struct sockaddr_in serAddr; //my sddress

	//memset(&serAddr.sin_zero, 0, 8);
	serAddr.sin_family = AF_INET;
	serAddr.sin_addr.s_addr = INADDR_ANY;
	serAddr.sin_port = htons(atoi(argv[1]));



	if(bind(s, (struct sockaddr*) &serAddr, sizeof(struct sockaddr_in)) < 0){
		printf("Binding Failed");
		close(s);
		exit(1);
	}

	//int nAddrLen = sizeof(remoteAddr);
	//memset(&remoteAddr.sin_zero, 0, 8);
	remoteAddr.sin_family = AF_INET;
	remoteAddr.sin_addr.s_addr = *((unsigned long*)remoteName->h_addr);
	remoteAddr.sin_port = htons(atoi(argv[3]));

	printf("Please enter message(input '!' to exit): \n");

	ret1=pthread_create(&thread1, NULL, input_message, &s); //(intptr_t) is used to hold  any int even if int is larger enough!!
	// see here : https://stackoverflow.com/questions/44298339/cast-to-pointer-from-integer-of-different-size-wint-to-pointer-cast
	// Or we can use &s here make it as a real pointer.
	if(ret1<0){
		printf("Inputting message failed.\n");
		close(s);
		exit(1);
	}
	ret2=pthread_create(&thread2, NULL, send_message, &s);
	if(ret2<0){
		printf("Sending message failed.\n");
		close(s);
		exit(1);
	}
	ret3=pthread_create(&thread3, NULL, receive_message, &s);
	if(ret3<0){
		printf("Receiving message failed.\n");
		close(s);
		exit(1);
	}
	ret4=pthread_create(&thread4, NULL, put_message, &s);
	if(ret4<0){
		printf("Putting message failed.\n");
		close(s);
		exit(1);
	}

	pthread_join(thread1,NULL);
	pthread_join(thread2,NULL);
	pthread_join(thread3,NULL);
	pthread_join(thread4,NULL);

	close(s);
	printf("can you see me???\n");
	pthread_exit(NULL);
	return 0;
}
