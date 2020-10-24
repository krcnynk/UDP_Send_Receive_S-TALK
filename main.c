#include "list.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h> 
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>

#define MAXBUFLEN 100
#define NUM_THREADS 4

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cv1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t  cv2 = PTHREAD_COND_INITIALIZER;
pthread_cond_t  cv3 = PTHREAD_COND_INITIALIZER;
pthread_cond_t  cv4 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexClose = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cvClose = PTHREAD_COND_INITIALIZER;
bool Close = false;
pthread_t threads[NUM_THREADS];

struct arg_struct {
    struct addrinfo* remote;
    List* list;
    int socket;
};

int x = 0;
int y = 0;
void* inputFromKeyboard(void* arg)
{   
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    List* sharedListOUT = arg;
    while(1)
    {
        int nread;
        char buffer[MAXBUFLEN];
        char* line;
        if((nread = read(STDIN_FILENO, buffer, sizeof(buffer)-1)) > 0)
        {
            line = malloc(nread+1);
            line[nread] = '\0';
            memcpy(line, buffer, nread);
            //write(STDOUT_FILENO,line, nread);
            pthread_mutex_lock(&mutex1);
            while(List_prepend(sharedListOUT, (void*) line) == -1) 
            {
                pthread_cond_wait(&cv1, &mutex1); //Wait if the list is full
            }
            pthread_cond_signal(&cv2);
            pthread_mutex_unlock(&mutex1);
        }
    }
}

void* outBound(void* arguments)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    int s = ((struct arg_struct*)arguments)->socket;
    List* sharedListOUT = ((struct arg_struct*)arguments)->list;
    struct addrinfo* remote = ((struct arg_struct*)arguments)->remote;
    int nsend;
    int remainder;
    char* toBeFreed;
    while(1)
    {
        pthread_mutex_lock(&mutex1);
        while(List_count(sharedListOUT) < 1)
        {
            pthread_cond_wait(&cv2, &mutex1);
        }
        toBeFreed = List_trim(sharedListOUT);
        pthread_cond_signal(&cv1); // Removed so signal
        pthread_mutex_unlock(&mutex1);
        remainder = 0;
        while(remainder < strlen(toBeFreed))
        {
            nsend = sendto(s, toBeFreed, strlen(toBeFreed), 0, remote->ai_addr, remote->ai_addrlen);
            remainder += nsend;
        }

        if(strlen(toBeFreed) == 2 && toBeFreed[0] == '!') //CANCEL
        {
            for(int i = 0; i<4 ; i++)
                pthread_cancel(threads[i]);
        }
        free(toBeFreed);
    }
}

void* inBound(void* arguments)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    int s = ((struct arg_struct*)arguments)->socket;
    List* sharedListIN = ((struct arg_struct*)arguments)->list;
    struct addrinfo* remote = ((struct arg_struct*)arguments)->remote;

    while(1)
    { 
        int nread;
        char* line;
        char buffer[MAXBUFLEN];
        if( (nread = recvfrom(s, buffer, sizeof(buffer), 0, remote->ai_addr, &remote->ai_addrlen)) > 0)
        {
            line = malloc(nread + 1);
            line[nread] = '\0';
            memcpy(line, buffer, nread);

            pthread_mutex_lock(&mutex2);
            while(List_prepend(sharedListIN, line) == -1) 
            {
                pthread_cond_wait(&cv3, &mutex2); 
            }
            pthread_cond_signal(&cv4);
            pthread_mutex_unlock(&mutex2);
        }
    }
}

void* printCharacters(void* arg)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    List* sharedListIN = arg;
    char* toBeFreed;
    while(1)
    {
        pthread_mutex_lock(&mutex2);
        while(List_count(sharedListIN) < 1)
        {
            pthread_cond_wait(&cv4, &mutex2);
        }
        toBeFreed = List_trim(sharedListIN);
        pthread_cond_signal(&cv3);
        pthread_mutex_unlock(&mutex2);

        int remaining = 0;
        while(remaining < strlen( (char*)toBeFreed))
        {
            int nwrite = write(STDOUT_FILENO, toBeFreed, strlen( (char*)toBeFreed) );
            remaining += nwrite;
        }

        if(strlen( (char*)toBeFreed) == 2 && toBeFreed[0] == '!') //CANCEL
        {
            for(int i = 0; i<4 ; i++)
                pthread_cancel(threads[i]);
        }
        free(toBeFreed);
    }
}


int main(int argc, char **argv)
{  
    if(argc <3)
        return -1;

    int result_code;
    List* sharedListIN = List_create();
    List* sharedListOUT = List_create();
    char* inport = argv[1]; // in port
    char* service = argv[3]; // out port
    char* hostname = argv[2]; // name
    int s;
    unsigned short int port;
    struct sockaddr_in addr;
    struct addrinfo hints,*remote_res;
    int status;

    port = atoi(inport); 
    s = socket(AF_INET, SOCK_DGRAM, 0);

    addr.sin_family = AF_INET;  
    addr.sin_port = htons(port);  
    addr.sin_addr.s_addr = INADDR_ANY;  
    memset(&addr.sin_zero, 0,8);

    bind(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

    memset(&hints, 0, sizeof(hints) );
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    status = getaddrinfo(hostname, service, &hints, &remote_res);
    printf("Status : %d\n",status);

    struct arg_struct argumentsOutBound;
    argumentsOutBound.list = sharedListOUT;
    argumentsOutBound.socket = s;
    argumentsOutBound.remote = remote_res;

    struct arg_struct argumentsInBound;
    argumentsInBound.list = sharedListIN;
    argumentsInBound.socket = s;
    argumentsInBound.remote = remote_res;

    //Creating the threads
    result_code = pthread_create(&threads[0], NULL, inputFromKeyboard, sharedListOUT);
    assert(!result_code);
    result_code = pthread_create(&threads[1], NULL, outBound, &argumentsOutBound);
    assert(!result_code);
    result_code = pthread_create(&threads[2], NULL, inBound, &argumentsInBound);
    assert(!result_code);
    result_code = pthread_create(&threads[3], NULL, printCharacters, sharedListIN);
    assert(!result_code);

    for (int i = 0; i < NUM_THREADS; i++) 
    {
        result_code = pthread_join(threads[i], NULL);
        assert(!result_code);
    }
    printf("HAHA\n");
    close(s); //close socket
    freeaddrinfo(remote_res); //free getaddrinfo return
    return -1;
}