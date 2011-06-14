/* tcpserver.c */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#define PORT 3333

int main()
{
    int fd = open("server-safe.txt", O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IWOTH|S_IROTH);
    if (fd < 0) {
        fprintf(stderr, "failed to open server-safe.txt\n");
        exit(1);
    }

    /* first line */
    write(fd, "line-1\n", 7);

    int sock;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket");
        exit(1);
    }

    int true = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &true, sizeof(int)) == -1) {
        perror("Setsockopt");
        exit(1);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(server_addr.sin_zero), 8);

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        perror("Unable to bind");
        exit(1);
    }

    if (listen(sock, 5) == -1) {
        perror("Listen");
        exit(1);
    }

    while(1) {
        struct sockaddr_in client_addr;
        int sin_size = sizeof(struct sockaddr_in);
        int connected = accept(sock, (struct sockaddr *)&client_addr,&sin_size);

        char data[1024];
        int len = read(connected, data, sizeof(data));
        data[len] = '\0';

        /* check termniation condition */
        if (strstr(data , "done") == data) {
            close(connected);
            break;
        }

        /* logging */
        write(fd, data, strlen(data));
    }

    /* last line */
    write(fd, "line-2\n", 7);

    close(fd);
    close(sock);

    return 0;
}
