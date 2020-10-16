#include "list.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h> 

#define MAXBUFLEN 100

void inputFromKeyboard(List* sharedListOUT, char* buffer)
{
    while(read(0, (void*) buffer, sizeof(buffer)) > 0)
    {
      List_add(sharedListOUT, buffer);
    }
}

void inBound()
{
  //  recvfrom(s, &msg, len, 0, (struct sockaddr *)&remote, sizeof(struct sockaddr_in));
}

void printCharacters()
{
    
}

void outBound()
{
//    sendto(s, &msg, len, 0, (struct sockaddr *)&remote, sizeof(struct sockaddr_in));  
}

int main(int argc, char **argv)
{  
    if(argc <3)
      return -1;

    char bufferIN[MAXBUFLEN];
    char bufferOUT[MAXBUFLEN];
    List* sharedListIN = List_create();
    List* sharedListOUT = List_create();

    char* inport = argv[1]; // in port
    char* service = argv[3]; // out port
    char* hostname = argv[2]; // name

    printf("%s %s %s\n",argv[1],argv[2],argv[3]);

    unsigned short int port = atoi(inport);
    struct sockaddr_in addr;  

    int s = socket(AF_INET, SOCK_DGRAM, 0);

    addr.sin_family = AF_INET;  
    addr.sin_port = htons(port);  
    addr.sin_addr.s_addr = INADDR_ANY;  
    memset(&addr.sin_zero, 0,8);
    
    bind(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
    printf("%s\n",inet_ntoa(addr.sin_addr));

    struct addrinfo hints,*remote_res;
    memset(&hints, 0, sizeof(hints) );
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    int status;
    status = getaddrinfo(hostname, service, &hints, &remote_res);
    printf("Status : %d",status);
////////////////////////////////////////////////
    char ipstr[INET6_ADDRSTRLEN];
    struct addrinfo *p;
    for(p = remote_res;p != NULL; p = p->ai_next) {
        void *addr;
        char *ipver;

        // get the pointer to the address itself,
        // different fields in IPv4 and IPv6:
        if (p->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } else { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }

        // convert the IP to a string and print it:
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf("  %s: %s\n", ipver, ipstr);
    }
/////////////////////////////////////////////////
    char msg1[MAXBUFLEN] = "KARDESIN";
    char msg2[MAXBUFLEN];

    while(true)
    {   //printf("DUDE\n");
        sendto(s, (void*)&msg1, sizeof(msg1), 0, (struct sockaddr *)remote_res->ai_addr, remote_res->ai_addrlen);
        //printf("DUD1E");
        recvfrom(s, (void*)&msg2, sizeof(msg2), 0, (struct sockaddr *)remote_res->ai_addr, remote_res->ai_addrlen);
        //printf("DU2DE");
        printf("%s\n",(char*)msg2);

    }

    return -1;
}
