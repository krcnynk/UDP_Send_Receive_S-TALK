#include <list.h>
#include <sys/types.h>
#include <sys/socket.h>

void inputFromKeyboard()
{

}

void inBound()
{
    recvfrom(s, &msg, len, 0, (struct sockaddr *)&remote, sizeof(struct sockaddr_in));
}

void printCharacters()
{
    
}

void outBound()
{
    sendto(s, &msg, len, 0, (struct sockaddr *)&remote, sizeof(struct sockaddr_in));  
}

int main(int argc, char **argv)
{  
     List* sharedList = List_create();

    int port = argv[1];
    char* service = argv[3];
    char* hostname = argv[2];
    
    //[my port number] [remote machine name] [remote port number]
    int port = 6060;
    struct sockaddr_in addr;  
    addr.sin_family = AF_INET;  
    sin_port = htons(port);  
    addr.sin_addr.s_addr = INADDR_ANY;  
    memset(&addr.sin_zero, ‘\0’, 8);

    int s = socket(AF_INET, SOCK_DGRAM, 0);
    bind(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

    void* msg1 = "KARDESIN";
    void* msg2;
    int len = sizeof(msg);

    struct sockaddr_in hints,*remote_res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;
    getaddrinfo(hostname, service, &hints, &remote_res);

    while(1)
    {
        sendto(s, &msg1, len, 0, (struct sockaddr *)&remote_res, sizeof(struct sockaddr_in));
        recvfrom(s, &msg2, len, 0, (struct sockaddr *)&remote_res, sizeof(struct sockaddr_in));

        printf("%s\n",(char*)msg2);

    }
}