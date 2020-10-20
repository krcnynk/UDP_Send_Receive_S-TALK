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

#define MAXBUFLEN 512
#define NUM_THREADS 4

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cv1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t  cv2 = PTHREAD_COND_INITIALIZER;
pthread_cond_t  cv3 = PTHREAD_COND_INITIALIZER;
pthread_cond_t  cv4 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexClose = PTHREAD_MUTEX_INITIALIZER;
pthread_t threads[NUM_THREADS];

struct arg_struct {
    struct addrinfo* remote;
    List* list;
    int socket;
};

void* inputFromKeyboard(void* arg)
{
    List* sharedListOUT = arg;
    while(1)
    {
        int nread;
        char* line;
        char buffer[MAXBUFLEN];

        if((nread = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0)
        {
            if(nread == 2 && buffer[0] == '!')
            {
                for(int i = 1; i<=NUM_THREADS;i++)
                {
                    pthread_cancel(threads[i%4]);
                }
            }
            //printf("I just read (nread)from keyboard returns: %d %d\n",nread);
            buffer[nread-1] = '\0';
            //printf("%s BUFFER\n",buffer);
            line = malloc(nread);
            memcpy(line, buffer, nread);
            //printf("%s line\n",line);
            pthread_mutex_lock(&mutex1);
            List_last(sharedListOUT); 
            int returnVal = List_add(sharedListOUT, line);
            if(returnVal == -1) 
            {
                pthread_cond_wait(&cv1, &mutex1); 
                List_add(sharedListOUT, line);
            }
            pthread_cond_signal(&cv2);
            pthread_mutex_unlock(&mutex1);
        }
    }
}

void* outBound(void* arguments)
{
    int s = ((struct arg_struct*)arguments)->socket;
    List* sharedListOUT = ((struct arg_struct*)arguments)->list;
    struct addrinfo* remote = ((struct arg_struct*)arguments)->remote;
    int nsend;
    void* toBeFreed;

    while(1)
    {
        pthread_mutex_lock(&mutex1);
        if(List_count(sharedListOUT) == 0)
        {
            pthread_cond_wait(&cv2, &mutex1);
        }
        toBeFreed = List_first(sharedListOUT);
        List_remove(sharedListOUT);
        pthread_cond_signal(&cv1);
        pthread_mutex_unlock(&mutex1);

        //printf("%d , %s",strlen( (char*)toBeFreed),(char*)toBeFreed);
        int remainder = 0;
        while(remainder < strlen( (char*)toBeFreed))
        {
            nsend = sendto(s, toBeFreed, strlen( (char*)toBeFreed), 0, remote->ai_addr, remote->ai_addrlen);
            remainder += nsend;
        }
        //printf("I just send (nsend) to otherside returns: %d\n",nsend);
        free(toBeFreed);
    }
}

void* inBound(void* arguments)
{
    int s = ((struct arg_struct*)arguments)->socket;
    List* sharedListIN = ((struct arg_struct*)arguments)->list;
    struct addrinfo* remote = ((struct arg_struct*)arguments)->remote;

    while(1)
    { 
        int nread;
        char* line;
        char buffer[MAXBUFLEN];
        
        //printf("HOP%d\n",recvfrom(s, buffer, MAXBUFLEN, 0, (struct sockaddr *)(remote->ai_addr), remote->ai_addrlen));
        //printf("recvfrom() failed %s\n", strerror(errno));
        if( (nread = recvfrom(s, buffer, sizeof(buffer), 0, remote->ai_addr, &remote->ai_addrlen)) > 0)
        {   //printf("I just read from otherside returns: %d\n",nread);
            buffer[nread] = '\n';
            buffer[nread+1] = '\0';
            line = malloc(nread + 2);
            memcpy(line, buffer, nread + 2);

            pthread_mutex_lock(&mutex2);
            List_last(sharedListIN); 
            int returnVal = List_add(sharedListIN, line);
            if(returnVal == -1) 
            {
                pthread_cond_wait(&cv3, &mutex2); 
                List_add(sharedListIN, line);
            }
            pthread_cond_signal(&cv4);
            pthread_mutex_unlock(&mutex2);
        }
    }
}

void* printCharacters(void* arg)
{
    List* sharedListIN = arg;
    while(1)
    {
        pthread_mutex_lock(&mutex2);
        if(List_count(sharedListIN) == 0)
        {
            pthread_cond_wait(&cv4, &mutex2);
        }
        void* toBeFreed = List_first(sharedListIN);
        List_remove(sharedListIN);
        pthread_cond_signal(&cv3);
        pthread_mutex_unlock(&mutex2);

        //printf("%s %d\n",(char*)toBeFreed,strlen( (char*)toBeFreed) );
        int nwrite = write(STDOUT_FILENO, toBeFreed, strlen( (char*)toBeFreed));
        //printf("I just wrote to screen returns: %d\n",nwrite);
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

    //printf("%s %s %s\n",argv[1],argv[2],argv[3]);

    port = atoi(inport); 
    s = socket(AF_INET, SOCK_DGRAM, 0);

    addr.sin_family = AF_INET;  
    addr.sin_port = htons(port);  
    addr.sin_addr.s_addr = INADDR_ANY;  
    memset(&addr.sin_zero, 0,8);

    bind(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
    //printf("%s\n",inet_ntoa(addr.sin_addr));

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
    close(s);
    freeaddrinfo(remote_res);  
    return -1;
}

/*
    char ipstr[INET6_ADDRSTRLEN];
    struct addrinfo *p;
    for(p = remote_res;p != NULL; p = p->ai_next) {
        void *addr;
        char *ipver;

        if (p->ai_family == AF_INET) { 
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } else { 
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }

        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf("  %s: %s\n", ipver, ipstr);
    }
*/

 /*   while(true)
    {   //printf("DUDE\n");
        sendto(s, (void*)&msg1, sizeof(msg1), 0, (struct sockaddr *)remote_res->ai_addr, remote_res->ai_addrlen);
        //printf("DUD1E");
        recvfrom(s, (void*)&msg2, sizeof(msg2), 0, (struct sockaddr *)remote_res->ai_addr, remote_res->ai_addrlen);
        //printf("DU2DE");
        printf("%s\n",(char*)msg2);

    }
*/