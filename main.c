#include "list.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>

void inputFromKeyboard()
{

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
     List* sharedList = List_create();

    int port = argv[1];
    char* service = argv[3];
    char* hostname = argv[2];
    //[my port number] [remote machine name] [remote port number]
    struct sockaddr_in addr;  
    addr.sin_family = AF_INET;  
    addr.sin_port = htons(port);  
    addr.sin_addr.s_addr = INADDR_ANY;  
    memset(&addr.sin_zero, '\0',8);

    int s = socket(AF_INET, SOCK_DGRAM, 0);
    bind(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

    char msg1[100] = "KARDESIN";
    char msg2[100];
    int len = sizeof(msg1);
    int len2 = sizeof(msg2);

    struct sockaddr_in hints,*remote_res;
    memset(&hints, 0, sizeof(hints));
    //hints.sin_socktype = SOCK_STREAM;
    hints.sin_family = AF_INET;
    getaddrinfo(hostname, service, &hints, &remote_res);

    while(1)
    {
        sendto(s, (void*)&msg1, len, 0, (struct sockaddr *)&remote_res, sizeof(struct sockaddr_in));
        recvfrom(s, (void*)&msg2, len2, 0, (struct sockaddr *)&remote_res, sizeof(struct sockaddr_in));

        printf("%s\n",(char*)msg2);

    }
}
