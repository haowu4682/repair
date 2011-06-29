#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#define PORT 3333

int main(int argc, char * argv[])
{
    if (argc != 2) {
        printf("usage %s [IP]\n", argv[0]);
        exit(1);
    }

    FILE * fd = fopen("client-safe.txt", "a+");
    if (!fd) {
        perror("failed to open client-safe.txt\n");
        exit(1);
    }

    fprintf(fd, "attack\n");

    struct hostent * host = gethostbyname(argv[1]);

    int sock;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr = *((struct in_addr *)host->h_addr);
    bzero(&(server_addr.sin_zero),8);

    if (connect(sock, (struct sockaddr *)&server_addr,
                sizeof(struct sockaddr)) == -1) {
        perror("connect");
        exit(1);
    }

    write(sock, "attack\n", 7);
    
    close(sock);
    fclose(fd);
    
    return 0;
}
